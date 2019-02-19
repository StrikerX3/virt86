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

#include <fcntl.h>
#include <linux/kvm.h>
#include <algorithm>
#include <functional>
#include <memory>

namespace virt86::kvm {

KvmVirtualMachine::KvmVirtualMachine(KvmPlatform& platform, const VMSpecifications& specifications, int fdKVM)
    : VirtualMachine(platform, specifications)
    , m_platform(platform)
    , m_fdKVM(fdKVM)
    , m_fd(-1)
    , m_memSlot(0)
{
}

KvmVirtualMachine::~KvmVirtualMachine() {
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


    // Configure the custom CPUIDs if supported
    if (m_platform.GetFeatures().customCPUIDs && m_specifications.CPUIDResults.size() > 0) {
        size_t count = m_specifications.CPUIDResults.size();
        kvm_cpuid2 *cpuid = (kvm_cpuid2 *)malloc(sizeof(kvm_cpuid2) + count * sizeof(kvm_cpuid_entry2));
        cpuid->nent = static_cast<__u32>(count);
        for (size_t i = 0; i < count; i++) {
            auto& entry = m_specifications.CPUIDResults[i];
            cpuid->entries[i].function = entry.function;
            cpuid->entries[i].index = static_cast<__u32>(i);
            cpuid->entries[i].flags = 0;
            cpuid->entries[i].eax = entry.eax;
            cpuid->entries[i].ebx = entry.ebx;
            cpuid->entries[i].ecx = entry.ecx;
            cpuid->entries[i].edx = entry.edx;
        }

        if (ioctl(m_fd, KVM_SET_CPUID2, cpuid) < 0) {
            close(m_fd);
            m_fd = -1;
            free(cpuid);
            return false;
        }
        free(cpuid);
    }

    // Create virtual processors
    for (uint32_t id = 0; id < m_specifications.numProcessors; id++) {
        auto vp = new KvmVirtualProcessor(*this, id);
        if (!vp->Initialize()) {
            delete vp;
            return false;
        }
        RegisterVP(vp);
    }

    return true;
}

MemoryMappingStatus KvmVirtualMachine::MapGuestMemoryImpl(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags, void *memory) {
    return MapGuestMemoryLargeImpl(baseAddress, size, flags, memory);
}

MemoryMappingStatus KvmVirtualMachine::MapGuestMemoryLargeImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) {
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
        m_memoryRegions.erase(std::find_if(m_memoryRegions.begin(), m_memoryRegions.end(), std::bind([](kvm_userspace_memory_region& rgn1, kvm_userspace_memory_region& rgn2) -> bool {
            return rgn1.slot == rgn2.slot;
        }, memoryRegion, std::placeholders::_1)));
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus KvmVirtualMachine::SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags) {
    return SetGuestMemoryFlagsLargeImpl(baseAddress, size, flags);
}

MemoryMappingStatus KvmVirtualMachine::SetGuestMemoryFlagsLargeImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) {
    // Define function that checks if the memory region exactly matches the
    // specified memory address range
    auto equals = std::bind([](kvm_userspace_memory_region& rgn, const uint64_t baseAddress, const uint64_t size) -> bool {
        return rgn.guest_phys_addr == baseAddress && rgn.memory_size == size;
    }, std::placeholders::_1, baseAddress, size);

    // Find memory range that corresponds to the specified address range
    auto memoryRegion = std::find_if(m_memoryRegions.begin(), m_memoryRegions.end(), equals);
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

}
