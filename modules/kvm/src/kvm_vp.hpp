/*
Declares the VirtualProcessor implementation class for the KVM platform
adapter.
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
#include "kvm_helpers.hpp"

#include <linux/kvm.h>

namespace virt86::kvm {

class KvmVirtualMachine;

class KvmVirtualProcessor : public VirtualProcessor {
public:
    KvmVirtualProcessor(KvmVirtualMachine& vm, uint32_t vcpuID);
    ~KvmVirtualProcessor() noexcept final;

    // Prevent copy construction and copy assignment
    KvmVirtualProcessor(const KvmVirtualProcessor&) = delete;
    KvmVirtualProcessor& operator=(const KvmVirtualProcessor&) = delete;

    // Prevent move construction and move assignment
    KvmVirtualProcessor(KvmVirtualProcessor&&) = delete;
    KvmVirtualProcessor&& operator=(KvmVirtualProcessor&&) = delete;

    // Disallow taking the address
    KvmVirtualProcessor *operator&() = delete;

    VPExecutionStatus RunImpl() noexcept override;
    VPExecutionStatus StepImpl() noexcept override;

    bool PrepareInterrupt(uint8_t vector) noexcept override;
    VPOperationStatus InjectInterrupt(uint8_t vector) noexcept override;
    bool CanInjectInterrupt() const noexcept override;
    void RequestInterruptWindow() noexcept override;

    VPOperationStatus RegRead(const Reg reg, RegValue& value) noexcept override;
    VPOperationStatus RegWrite(const Reg reg, const RegValue& value) noexcept override;
    VPOperationStatus RegRead(const Reg regs[], RegValue values[], const size_t numRegs) noexcept override;
    VPOperationStatus RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) noexcept override;

    VPOperationStatus GetFPUControl(FPUControl& value) noexcept override;
    VPOperationStatus SetFPUControl(const FPUControl& value) noexcept override;

    VPOperationStatus GetMXCSR(MXCSR& value) noexcept override;
    VPOperationStatus SetMXCSR(const MXCSR& value) noexcept override;

    VPOperationStatus GetMXCSRMask(MXCSR& value) noexcept override;
    VPOperationStatus SetMXCSRMask(const MXCSR& value) noexcept override;

    VPOperationStatus GetMSR(const uint64_t msr, uint64_t& value) noexcept override;
    VPOperationStatus SetMSR(const uint64_t msr, const uint64_t value) noexcept override;
    VPOperationStatus GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) noexcept override;
    VPOperationStatus SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) noexcept override;

    VPOperationStatus EnableSoftwareBreakpoints(bool enable) noexcept override;
    VPOperationStatus SetHardwareBreakpoints(HardwareBreakpoints breakpoints) noexcept override;
    VPOperationStatus ClearHardwareBreakpoints() noexcept override;
    VPOperationStatus GetBreakpointAddress(uint64_t *address) const noexcept override;

private:
    KvmVirtualMachine& m_vm;
    uint32_t m_vcpuID;

    int m_fd;

    bool m_regsDirty;
    bool m_regsChanged;
    bool m_sregsChanged;
    bool m_debugRegsChanged;
    bool m_fpuRegsChanged;

    struct kvm_regs m_regs;
    struct kvm_sregs m_sregs;
    struct kvm_debugregs m_debugRegs;
    struct kvm_fpu m_fpuRegs;

    struct kvm_run* m_kvmRun;
    int m_kvmRunMmapSize;

    struct kvm_guest_debug m_debug;

    bool Initialize() noexcept;

    bool UpdateRegisters() noexcept;
    bool SetDebug() noexcept;
    bool RefreshRegisters() noexcept;

    VPExecutionStatus HandleExecResult() noexcept;
    void HandleException() noexcept;
    void HandleIO() noexcept;
    void HandleMMIO() noexcept;

    VPOperationStatus KvmRegRead(const Reg reg, RegValue& value) noexcept;
    VPOperationStatus KvmRegWrite(const Reg reg, const RegValue& value) noexcept;

    // Allow KvmVirtualMachine to access the constructor and Initialize()
    friend class KvmVirtualMachine;
};

}
