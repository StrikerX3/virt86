/*
Implementation of the KVM VirtualMachine class.
-------------------------------------------------------------------------------
MIT License

Copyright (c) 2019 Ivan Roberto de Oliveira

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "kvm_vm.hpp"
#include "kvm_vp.hpp"
#include "kvm_helpers.hpp"

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <algorithm>
#include <functional>
#include <memory>

namespace virt86::kvm {

// Checks if the memory region exactly matches the given memory address range
static bool regionEquals(kvm_userspace_memory_region& rgn, const uint64_t baseAddress, const uint64_t size) noexcept {
    return rgn.guest_phys_addr == baseAddress && rgn.memory_size == size;
}

KvmVirtualMachine::KvmVirtualMachine(KvmPlatform& platform, const VMSpecifications& specifications, int fdKVM) noexcept
    : VirtualMachine(platform, specifications)
    , m_platform(platform)
    , m_fdKVM(fdKVM)
    , m_fd(-1)
    , m_memSlot(0)
{
}

KvmVirtualMachine::~KvmVirtualMachine() noexcept {
    DestroyVPs();
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

bool KvmVirtualMachine::Initialize() {
    // Create the VM
    m_fd = ioctl(m_fdKVM, KVM_CREATE_VM, 0);
    if (m_fd < 0) {
        return false;
    }

    // Move the identity map to the specified address.
    uint64_t identityMapAddr = m_specifications.kvm.identityMapPageAddress;
    if (ioctl(m_fd, KVM_SET_IDENTITY_MAP_ADDR, &identityMapAddr) < 0) {
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // Setup TSC scaling if requested and available
    if (m_platform.GetFeatures().guestTSCScaling && m_specifications.guestTSCFrequency != 0) {
        uint32_t guestFreqKHz = m_specifications.guestTSCFrequency / 1000;
        if (ioctl(m_fd, KVM_SET_TSC_KHZ, &guestFreqKHz) < 0) {
            close(m_fd);
            m_fd = -1;
            return false;
        }
    }

    // Create virtual processors
    for (uint32_t id = 0; id < m_specifications.numProcessors; id++) {
        auto vp = std::make_unique<KvmVirtualProcessor>(*this, id);
        if (!vp->Initialize()) {
            return false;
        }
        RegisterVP(std::move(vp));
    }

    return true;
}

MemoryMappingStatus KvmVirtualMachine::MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept {
    kvm_userspace_memory_region& memoryRegion = m_memoryRegions.emplace_back();
    memoryRegion.guest_phys_addr = baseAddress;
    memoryRegion.memory_size = size;
    memoryRegion.userspace_addr = (uint64_t)memory;
    memoryRegion.slot = m_memSlot++;
    memoryRegion.flags = 0;
    auto flagsBM = BitmaskEnum(flags);
    if (flagsBM.NoneOf(MemoryFlags::Write)) memoryRegion.flags |= KVM_MEM_READONLY;
    if (flagsBM.AnyOf(MemoryFlags::DirtyPageTracking)) memoryRegion.flags |= KVM_MEM_LOG_DIRTY_PAGES;

    if (ioctl(m_fd, KVM_SET_USER_MEMORY_REGION, &memoryRegion) < 0) {
        for (auto it = m_memoryRegions.begin(); it != m_memoryRegions.end(); it++) {
            if (it->slot == memoryRegion.slot) {
                m_memoryRegions.erase(it);
                break;
            }
        }
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus KvmVirtualMachine::SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept {
    // Find memory range that corresponds to the specified address range
    auto memoryRegion = std::find_if(m_memoryRegions.begin(), m_memoryRegions.end(),
        [=](kvm_userspace_memory_region& memRgn) { return regionEquals(memRgn, baseAddress, size); });
    if (memoryRegion == m_memoryRegions.end()) {
        return MemoryMappingStatus::InvalidRange;
    }

    // Update flags
    memoryRegion->flags = 0;
    auto flagsBM = BitmaskEnum(flags);
    if (flagsBM.NoneOf(MemoryFlags::Write)) memoryRegion->flags |= KVM_MEM_READONLY;
    if (flagsBM.AnyOf(MemoryFlags::DirtyPageTracking)) memoryRegion->flags |= KVM_MEM_LOG_DIRTY_PAGES;
    if (ioctl(m_fd, KVM_SET_USER_MEMORY_REGION, &memoryRegion) < 0) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

DirtyPageTrackingStatus KvmVirtualMachine::QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept {
    // Find memory range that corresponds to the specified address range
    auto memoryRegion = std::find_if(m_memoryRegions.begin(), m_memoryRegions.end(),
        [=](kvm_userspace_memory_region& memRgn) { return regionEquals(memRgn, baseAddress, size); });
    if (memoryRegion == m_memoryRegions.end()) {
        return DirtyPageTrackingStatus::InvalidRange;
    }

    // Get dirty bitmap for the entire slot
    kvm_dirty_log dirtyLog = { 0 };
    dirtyLog.slot = memoryRegion->slot;
    dirtyLog.dirty_bitmap = bitmap;
    if (ioctl(m_fd, KVM_GET_DIRTY_LOG, &dirtyLog) < 0) {
        return DirtyPageTrackingStatus::Failed;
    }

    return DirtyPageTrackingStatus::OK;
}

DirtyPageTrackingStatus KvmVirtualMachine::ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) noexcept {
    // Delegate to QueryDirtyPagesImpl as it will clear the dirty bitmap for
    // the memory slot
    const size_t bitmapSize = (size / PAGE_SIZE + sizeof(uint64_t) - 1) / sizeof(uint64_t);
    auto bitmap = new uint64_t[bitmapSize];
    auto status = QueryDirtyPagesImpl(baseAddress, size, bitmap, bitmapSize);
    delete[] bitmap;
    return status;
}

}
