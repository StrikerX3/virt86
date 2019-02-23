/*
HAXM system-based virtual processor implementation for Mac OS X.
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
#pragma once

#include "virt86/platform/platform.hpp"
#include "haxm_vm.hpp"
#include "haxm_platform_impl.hpp"
#include "haxm_sys_platform.hpp"

#include "interface/hax_interface.hpp"

#include <sys/ioctl.h>

namespace virt86::haxm {

class HaxmVirtualProcessorSysImpl {
public:
    HaxmVirtualProcessorSysImpl(HaxmVirtualMachine& vm, HaxmVirtualMachineSysImpl& vmSys);
    ~HaxmVirtualProcessorSysImpl();

    bool Initialize(uint32_t vcpuID, hax_tunnel **out_tunnel, void **out_ioTunnel);
    void Destroy();

    bool Run();

    bool InjectInterrupt(uint8_t vector);

    bool GetRegisters(vcpu_state_t *registers);
    bool SetRegisters(vcpu_state_t *registers);

    bool GetFPURegisters(fx_layout *registers);
    bool SetFPURegisters(fx_layout *registers);

    bool GetMSRData(hax_msr_data *msrData);
    bool SetMSRData(hax_msr_data *msrData);

    bool SetDebug(hax_debug_t *debug);

private:
    HaxmVirtualMachine& m_vm;
    HaxmVirtualMachineSysImpl& m_vmSys;

    int m_fdVM;
    int m_fd;
};

}
