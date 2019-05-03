/*
HAXM system-based virtual machine implementation for Windows.
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

namespace virt86::haxm {

HaxmVirtualMachineSysImpl::HaxmVirtualMachineSysImpl(HaxmPlatformImpl& platformImpl) noexcept
    : m_platformImpl(platformImpl)
    , m_handle(INVALID_HANDLE_VALUE)
    , m_hHAXM(platformImpl.m_sys->Handle())
{
}

HaxmVirtualMachineSysImpl::~HaxmVirtualMachineSysImpl() noexcept {
    Destroy();
}

bool HaxmVirtualMachineSysImpl::Initialize(const size_t numProcessors, uint32_t *out_vmID) noexcept {
    if (out_vmID == nullptr) {
        return false;
    }

    DWORD returnSize;
    BOOLEAN bResult;

    // Ask HAXM to create a VM
    uint32_t vmID;
    bResult = DeviceIoControl(m_hHAXM,
        HAX_IOCTL_CREATE_VM,
        NULL, 0,
        &vmID, sizeof(uint32_t),
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        return false;
    }

    // VM created successfully; open the object
    wchar_t vmName[13];
    swprintf_s(vmName, L"\\\\.\\hax_vm%02u", vmID);
    m_handle = CreateFileW(vmName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (m_handle == INVALID_HANDLE_VALUE) {
        *out_vmID = 0xFFFFFFFF;
        return false;
    }

    *out_vmID = vmID;

    return true;
}

void HaxmVirtualMachineSysImpl::Destroy() noexcept {
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool HaxmVirtualMachineSysImpl::ReportQEMUVersion(hax_qemu_version& version) noexcept {
    DWORD returnSize;
    
    return DeviceIoControl(m_handle,
        HAX_VM_IOCTL_NOTIFY_QEMU_VERSION,
        &version, sizeof(hax_qemu_version),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapHostMemory(void *memory, const uint32_t size) noexcept {
    DWORD returnSize;
    BOOLEAN bResult;

    hax_alloc_ram_info memInfo;
    memInfo.va = (uint64_t)memory;
    memInfo.size = size;
    bResult = DeviceIoControl(m_handle,
        HAX_VM_IOCTL_ALLOC_RAM,
        &memInfo, sizeof(memInfo),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        return MemoryMappingStatus::AlreadyAllocated;
    }
    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapGuestMemory(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags, void *memory) noexcept {
    DWORD returnSize;
    BOOLEAN bResult;

    hax_set_ram_info setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = (uint64_t)memory;
    setMemInfo.size = size;
    setMemInfo.flags = BitmaskEnum(flags).AnyOf(MemoryFlags::Write) ? 0 : HAX_RAM_INFO_ROM;

    bResult = DeviceIoControl(m_handle,
        HAX_VM_IOCTL_SET_RAM,
        &setMemInfo, sizeof(setMemInfo),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

bool HaxmVirtualMachineSysImpl::UnmapGuestMemory(const uint64_t baseAddress, const uint32_t size) noexcept {
    DWORD returnSize;

    hax_set_ram_info setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = 0;
    setMemInfo.size = size;
    setMemInfo.flags = HAX_RAM_INFO_INVALID;

    return DeviceIoControl(m_handle,
        HAX_VM_IOCTL_SET_RAM,
        &setMemInfo, sizeof(setMemInfo),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapHostMemoryLarge(void *memory, const uint64_t size) noexcept {
    DWORD returnSize;
    BOOLEAN bResult;

    hax_ramblock_info memInfo;
    memInfo.start_va = (uint64_t)memory;
    memInfo.size = size;
    memInfo.reserved = 0;
    bResult = DeviceIoControl(m_handle,
        HAX_VM_IOCTL_ADD_RAMBLOCK,
        &memInfo, sizeof(memInfo),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        return MemoryMappingStatus::AlreadyAllocated;
    }
    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HaxmVirtualMachineSysImpl::MapGuestMemoryLarge(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept {
    DWORD returnSize;
    BOOLEAN bResult;

    hax_set_ram_info2 setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = (uint64_t)memory;
    setMemInfo.size = size;
    setMemInfo.reserved1 = 0;
    setMemInfo.reserved2 = 0;
    setMemInfo.flags = BitmaskEnum(flags).AnyOf(MemoryFlags::Write) ? 0 : HAX_RAM_INFO_ROM;

    bResult = DeviceIoControl(m_handle,
        HAX_VM_IOCTL_SET_RAM2,
        &setMemInfo, sizeof(setMemInfo),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

bool HaxmVirtualMachineSysImpl::UnmapGuestMemoryLarge(const uint64_t baseAddress, const uint64_t size) noexcept {
    DWORD returnSize;

    hax_set_ram_info2 setMemInfo;
    setMemInfo.pa_start = baseAddress;
    setMemInfo.va = 0;
    setMemInfo.size = size;
    setMemInfo.reserved1 = 0;
    setMemInfo.reserved2 = 0;
    setMemInfo.flags = HAX_RAM_INFO_INVALID;

    return DeviceIoControl(m_handle,
        HAX_VM_IOCTL_SET_RAM2,
        &setMemInfo, sizeof(setMemInfo),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualMachineSysImpl::SetGuestMemoryFlagsLarge(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept {
    DWORD returnSize;

    hax_protect_ram_info protectInfo = { 0 };
    protectInfo.pa_start = baseAddress;
    protectInfo.size = size;
    protectInfo.flags = static_cast<uint32_t>(flags);

    return DeviceIoControl(m_handle,
        HAX_VM_IOCTL_PROTECT_RAM,
        &protectInfo, sizeof(protectInfo),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

}
