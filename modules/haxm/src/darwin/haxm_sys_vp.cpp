/*
HAXM system-based virtual processor implementation for macOS.
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

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

namespace virt86::haxm {

HaxmVirtualProcessorSysImpl::HaxmVirtualProcessorSysImpl(HaxmVirtualMachine& vm, HaxmVirtualMachineSysImpl& vmSys) noexcept
    : m_vm(vm)
    , m_vmSys(vmSys)
    , m_fdVM(vmSys.FileDescriptor())
    , m_fd(-1)
{
}

HaxmVirtualProcessorSysImpl::~HaxmVirtualProcessorSysImpl() noexcept {
    Destroy();
}

bool HaxmVirtualProcessorSysImpl::Initialize(uint32_t vcpuID, hax_tunnel** out_tunnel, void** out_ioTunnel) noexcept {
    // Create virtual processor
    int result = ioctl(m_fdVM, HAX_VM_IOCTL_VCPU_CREATE, &vcpuID);
    if (result < 0) {
        return false;
    }

    // Open virtual processor object
    char vcpuName[21];
    sprintf(vcpuName, "/dev/hax_vm%02d/vcpu%02d", m_vm.ID(), vcpuID);
    m_fd = open(vcpuName, O_RDWR);
    if (m_fd < 0) {
        return false;
    }

    // Setup tunnel
    hax_tunnel_info tunnelInfo;
    result = ioctl(m_fd, HAX_VCPU_IOCTL_SETUP_TUNNEL, &tunnelInfo);
    if (result < 0) {
        close(m_fd);
        m_fd = -1;
        return false;
    }

    *out_tunnel = (hax_tunnel *)(intptr_t)tunnelInfo.va;
    *out_ioTunnel = (void *)(intptr_t)tunnelInfo.io_va;

    return true;
}

void HaxmVirtualProcessorSysImpl::Destroy() noexcept {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

bool HaxmVirtualProcessorSysImpl::Run() noexcept {
    return ioctl(m_fd, HAX_VCPU_IOCTL_RUN) >= 0;
}

bool HaxmVirtualProcessorSysImpl::InjectInterrupt(uint8_t vector) noexcept {
    uint32_t vector32 = static_cast<uint32_t>(vector);
    return ioctl(m_fd, HAX_VCPU_IOCTL_INTERRUPT, &vector32) >= 0;
}

bool HaxmVirtualProcessorSysImpl::GetRegisters(vcpu_state_t* registers) noexcept {
    return ioctl(m_fd, HAX_VCPU_GET_REGS, registers) >= 0;
}

bool HaxmVirtualProcessorSysImpl::SetRegisters(vcpu_state_t* registers) noexcept {
    return ioctl(m_fd, HAX_VCPU_SET_REGS, registers) >= 0;
}

bool HaxmVirtualProcessorSysImpl::GetFPURegisters(fx_layout* registers) noexcept {
    return ioctl(m_fd, HAX_VCPU_IOCTL_GET_FPU, registers) >= 0;
}

bool HaxmVirtualProcessorSysImpl::SetFPURegisters(fx_layout* registers) noexcept {
    return ioctl(m_fd, HAX_VCPU_IOCTL_SET_FPU, registers) >= 0;
}

bool HaxmVirtualProcessorSysImpl::GetMSRData(hax_msr_data* msrData) noexcept {
    return ioctl(m_fd, HAX_VCPU_IOCTL_GET_MSRS, msrData) >= 0;
}

bool HaxmVirtualProcessorSysImpl::SetMSRData(hax_msr_data* msrData) noexcept {
    return ioctl(m_fd, HAX_VCPU_IOCTL_SET_MSRS, msrData) >= 0;
}

bool HaxmVirtualProcessorSysImpl::SetDebug(hax_debug_t* debug) noexcept {
    return ioctl(m_fd, HAX_IOCTL_VCPU_DEBUG, debug) >= 0;
}

}

