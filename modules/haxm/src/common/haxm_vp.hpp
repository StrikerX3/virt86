/*
Declares the VirtualProcessor implementation class for HAXM.
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

#include "virt86/vp/vp.hpp"
#include "haxm_sys_vm.hpp"
#include "haxm_sys_vp.hpp"

#include "interface/hax_interface.hpp"

#include <memory>

namespace virt86::haxm {

class HaxmVirtualMachine;

class HaxmVirtualProcessor : public VirtualProcessor {
public:
    VPExecutionStatus RunImpl() override;
    VPExecutionStatus StepImpl() override;

    bool PrepareInterrupt(uint8_t vector) override;
    VPOperationStatus InjectInterrupt(uint8_t vector) override;
    bool CanInjectInterrupt() const override;
    void RequestInterruptWindow() override;

    VPOperationStatus RegRead(const Reg reg, RegValue& value) override;
    VPOperationStatus RegWrite(const Reg reg, const RegValue& value) override;
    VPOperationStatus RegRead(const Reg regs[], RegValue values[], const size_t numRegs) override;
    VPOperationStatus RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) override;

    VPOperationStatus GetFPUControl(FPUControl& value) override;
    VPOperationStatus SetFPUControl(const FPUControl& value) override;

    VPOperationStatus GetMXCSR(MXCSR& value) override;
    VPOperationStatus SetMXCSR(const MXCSR& value) override;

    VPOperationStatus GetMXCSRMask(MXCSR& value) override;
    VPOperationStatus SetMXCSRMask(const MXCSR& value) override;

    VPOperationStatus GetMSR(const uint64_t msr, uint64_t& value) override;
    VPOperationStatus SetMSR(const uint64_t msr, const uint64_t value) override;
    VPOperationStatus GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) override;
    VPOperationStatus SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) override;

    VPOperationStatus EnableSoftwareBreakpoints(bool enable) override;
    VPOperationStatus SetHardwareBreakpoints(HardwareBreakpoints breakpoints) override;
    VPOperationStatus ClearHardwareBreakpoints() override;
    VPOperationStatus GetBreakpointAddress(uint64_t *address) const override;

    hax_tunnel* Tunnel() const { return m_tunnel; }
    void* IOTunnel() const { return m_ioTunnel; }
    const hax_debug_t* Debug() const { return &m_debug; }

    const uint32_t ID() const { return m_vcpuID; }

protected:
    HaxmVirtualProcessor(HaxmVirtualMachine& vm, HaxmVirtualMachineSysImpl& vmSys, uint32_t id);
    ~HaxmVirtualProcessor() override;

private:
    HaxmVirtualMachine& m_vm;
    std::unique_ptr<HaxmVirtualProcessorSysImpl> m_sys;
 
    vcpu_state_t m_regs;
    fx_layout m_fpuRegs;
    bool m_regsDirty;        // Set to true on VM exit to indicate that registers need to be refreshed
    bool m_regsChanged;      // Set to true when general registers are modified by the host
    bool m_fpuRegsChanged;   // Set to true when floating point registers are modified by the host

    hax_tunnel *m_tunnel;
    void *m_ioTunnel;
    hax_debug_t m_debug;

    uint32_t m_vcpuID;

    bool Initialize();

    // Allow HaxmVirtualMachine to access the constructor and Initialize()
    friend class HaxmVirtualMachine;

    // ----- Helper methods ---------------------------------------------------

    bool SetDebug();

    void UpdateRegisters();
    bool RefreshRegisters();
    VPExecutionStatus HandleExecResult();

    void HandleIO();
    void HandleFastMMIO();

    VPOperationStatus HaxmRegRead(const Reg reg, RegValue& value);
    VPOperationStatus HaxmRegWrite(const Reg reg, const RegValue& value);
};

}
