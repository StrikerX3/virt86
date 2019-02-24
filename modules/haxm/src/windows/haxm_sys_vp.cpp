/*
HAXM system-based virtual processor implmentation for Windows.
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
#include "haxm_sys_vp.hpp"

namespace virt86::haxm {

HaxmVirtualProcessorSysImpl::HaxmVirtualProcessorSysImpl(HaxmVirtualMachine& vm, HaxmVirtualMachineSysImpl& vmSys) noexcept
    : m_vm(vm)
    , m_vmSys(vmSys)
    , m_hVM(vmSys.Handle())
    , m_handle(INVALID_HANDLE_VALUE)
{
}

HaxmVirtualProcessorSysImpl::~HaxmVirtualProcessorSysImpl() noexcept {
    Destroy();
}

bool HaxmVirtualProcessorSysImpl::Initialize(uint32_t vcpuID, hax_tunnel** out_tunnel, void** out_ioTunnel) noexcept {
    if (out_tunnel == nullptr || out_ioTunnel == nullptr) {
        return false;
    }

    DWORD returnSize;
    BOOLEAN bResult;

    // Create virtual processor
    bResult = DeviceIoControl(m_hVM,
        HAX_VM_IOCTL_VCPU_CREATE,
        &vcpuID, sizeof(vcpuID),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        return false;
    }

    // Open virtual processor object
    wchar_t vcpuName[20];
    swprintf_s(vcpuName, L"\\\\.\\hax_vm%02u_vcpu%02u", m_vm.ID(), vcpuID);
    m_handle = CreateFileW(vcpuName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (m_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Setup tunnel
    hax_tunnel_info tunnelInfo;
    bResult = DeviceIoControl(m_handle,
        HAX_VCPU_IOCTL_SETUP_TUNNEL,
        NULL, 0,
        &tunnelInfo, sizeof(tunnelInfo),
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    *out_tunnel = (hax_tunnel *)(intptr_t)tunnelInfo.va;
    *out_ioTunnel = (void *)(intptr_t)tunnelInfo.io_va;

    return true;
}

void HaxmVirtualProcessorSysImpl::Destroy() noexcept {
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool HaxmVirtualProcessorSysImpl::Run() noexcept {
    DWORD returnSize;

    return DeviceIoControl(m_handle,
        HAX_VCPU_IOCTL_RUN,
        NULL, 0,
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::InjectInterrupt(uint8_t vector) noexcept {
    DWORD returnSize;

    uint32_t vec32 = vector;
    return DeviceIoControl(m_handle,
        HAX_VCPU_IOCTL_INTERRUPT,
        &vec32, sizeof(vec32),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::GetRegisters(vcpu_state_t* registers) noexcept {
    DWORD returnSize;

    return DeviceIoControl(m_handle,
        HAX_VCPU_GET_REGS,
        NULL, 0,
        registers, sizeof(vcpu_state_t),
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::SetRegisters(vcpu_state_t* registers) noexcept {
    DWORD returnSize;

    return DeviceIoControl(m_handle,
        HAX_VCPU_SET_REGS,
        registers, sizeof(vcpu_state_t),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::GetFPURegisters(fx_layout* registers) noexcept {
    DWORD returnSize;

    return DeviceIoControl(m_handle,
        HAX_VCPU_IOCTL_GET_FPU,
        NULL, 0,
        registers, sizeof(fx_layout),
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::SetFPURegisters(fx_layout* registers) noexcept {
    DWORD returnSize;

    return DeviceIoControl(m_handle,
        HAX_VCPU_IOCTL_SET_FPU,
        registers, sizeof(fx_layout),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::GetMSRData(hax_msr_data* msrData) noexcept {
    DWORD returnSize;

    return DeviceIoControl(m_handle,
        HAX_VCPU_IOCTL_GET_MSRS,
        msrData, sizeof(hax_msr_data),
        msrData, sizeof(hax_msr_data),
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::SetMSRData(hax_msr_data* msrData) noexcept {
    DWORD returnSize;

    return DeviceIoControl(m_handle,
        HAX_VCPU_IOCTL_SET_MSRS,
        msrData, sizeof(hax_msr_data),
        msrData, sizeof(hax_msr_data),
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

bool HaxmVirtualProcessorSysImpl::SetDebug(hax_debug_t* debug) noexcept {
    DWORD returnSize;
 
    return DeviceIoControl(m_handle,
        HAX_IOCTL_VCPU_DEBUG,
        debug, sizeof(hax_debug_t),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

}
