/*
HAXM system-based virtual machine implementation for Linux.
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
#include "haxm_sys_vm.hpp"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

namespace virt86::haxm {

HaxmVirtualMachineSysImpl::HaxmVirtualMachineSysImpl(HaxmPlatformImpl& platformImpl) noexcept
    : m_platformImpl(platformImpl)
    , m_fd(-1)
    , m_fdHAXM(platformImpl.m_sys->FileDescriptor())
{
}

HaxmVirtualMachineSysImpl::~HaxmVirtualMachineSysImpl() noexcept {
    Destroy();
}

bool HaxmVirtualMachineSysImpl::Initialize(const size_t numProcessors, uint32_t *out_vmID) noexcept {
    // Ask HAXM to create a VM
    uint32_t vmID;
    int result = ioctl(m_fdHAXM, HAX_IOCTL_CREATE_VM, &vmID);
    if (result == -1) {
        return false;
    }

    // VM created successfully; open the object
    char vmName[17];
    sprintf(vmName, "/dev/hax_vm/vm%02d", vmID);
    m_fd = open(vmName, O_RDWR);
    if (m_fd == -1) {
        *out_vmID = 0xFFFFFFFF;
        return false;
    }

    *out_vmID = vmID;

    return true;
}

void HaxmVirtualMachineSysImpl::Destroy() noexcept {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

bool HaxmVirtualMachineSysImpl::ReportQEMUVersion(hax_qemu_version& version) noexcept {
    return ioctl(m_fd, HAX_VM_IOCTL_NOTIFY_QEMU_VERSION, &version) >= 0;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapHostMemory(void *memory, const uint32_t size) noexcept {
    hax_alloc_ram_info memInfo;
    memInfo.va = (uint64_t)memory;
    memInfo.size = size;
    int result = ioctl(m_fd, HAX_VM_IOCTL_ALLOC_RAM, &memInfo);
    if (result < 0) {
        return MemoryMappingStatus::AlreadyAllocated;
    }
    
    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapGuestMemory(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags, void *memory) noexcept {
    hax_set_ram_info setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = (uint64_t)memory;
    setMemInfo.size = size;
    setMemInfo.flags = BitmaskEnum(flags).AnyOf(MemoryFlags::Write) ? 0 : HAX_RAM_INFO_ROM;
    int result = ioctl(m_fd, HAX_VM_IOCTL_SET_RAM, &setMemInfo);
    if (result < 0) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

bool HaxmVirtualMachineSysImpl::UnmapGuestMemory(const uint64_t baseAddress, const uint32_t size) noexcept {
    hax_set_ram_info setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = 0;
    setMemInfo.size = size;
    setMemInfo.flags = HAX_RAM_INFO_INVALID;
    return ioctl(m_fd, HAX_VM_IOCTL_SET_RAM, &setMemInfo) >= 0;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapHostMemoryLarge(void *memory, const uint64_t size) noexcept {
    hax_ramblock_info memInfo;
    memInfo.start_va = (uint64_t)memory;
    memInfo.size = size;
    memInfo.reserved = 0;
    int result = ioctl(m_fd, HAX_VM_IOCTL_ADD_RAMBLOCK, &memInfo);
    if (result < 0) {
        return MemoryMappingStatus::AlreadyAllocated;
    }

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapGuestMemoryLarge(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept {
    hax_set_ram_info2 setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = (uint64_t)memory;
    setMemInfo.size = size;
    setMemInfo.reserved1 = 0;
    setMemInfo.reserved2 = 0;
    setMemInfo.flags = BitmaskEnum(flags).AnyOf(MemoryFlags::Write) ? 0 : HAX_RAM_INFO_ROM;
    int result = ioctl(m_fd, HAX_VM_IOCTL_SET_RAM2, &setMemInfo);
    if (result < 0) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

bool HaxmVirtualMachineSysImpl::UnmapGuestMemoryLarge(const uint64_t baseAddress, const uint64_t size) noexcept {
    hax_set_ram_info2 setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = 0;
    setMemInfo.size = size;
    setMemInfo.reserved1 = 0;
    setMemInfo.reserved2 = 0;
    setMemInfo.flags = HAX_RAM_INFO_INVALID;
    return ioctl(m_fd, HAX_VM_IOCTL_SET_RAM2, &setMemInfo) >= 0;
}

bool HaxmVirtualMachineSysImpl::SetGuestMemoryFlagsLarge(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept {
    hax_protect_ram_info protectInfo = { 0 };
    protectInfo.pa_start = baseAddress;
    protectInfo.size = size;
    protectInfo.flags = static_cast<uint32_t>(flags);
    return ioctl(m_fd, HAX_VM_IOCTL_PROTECT_RAM, &protectInfo) >= 0;
}

}

