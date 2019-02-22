/*
Implementation of the NetBSD Virtual Machine Monitor VirtualProcessor class.
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
#include "nvmm_vp.hpp"
#include "nvmm_vm.hpp"

namespace virt86::nvmm {

NVMMVirtualProcessor::NVMMVirtualProcessor(NVMMVirtualMachine& vm, uint32_t vcpuID)
    : VirtualProcessor(vm)
    , m_vm(vm)
    , m_vcpuID(vcpuID)
{
}

NVMMVirtualProcessor::~NVMMVirtualProcessor() {
    // TODO: Close/release/free VCPU
}

bool NVMMVirtualProcessor::Initialize() {
    // TODO: Create the VCPU
    // TODO: Initialize any additional features available to the platform

    return true;

}

VPExecutionStatus NVMMVirtualProcessor::RunImpl() {
    // TODO: Request VCPU to run
    // If registers are cached in this class, update the VCPU state before running
    // (see HAXM or KVM for examples of this)

    return HandleExecResult();
}

VPExecutionStatus NVMMVirtualProcessor::StepImpl() {
    // TODO: If NVMM does not support guest debugging or single stepping, this
    // function should return VPExecutionStatus::Unsupported.
    // Otherwise, implement as follows:

    // TODO: Enable single stepping

    auto status = RunImpl();

    // TODO: Disable single stepping

    if (status != VPExecutionStatus::OK) {
        return status;
    }

    auto result = HandleExecResult();

    // TODO: When single stepping is enabled, most hypervisors will cause the
    // VM to exit due to a software breakpoint. If that's the case, change the
    // exit reason to Step here. If NVMM has a special VM exit for single
    // stepping, this is not needed here
    if (m_exitInfo.reason == VMExitReason::SoftwareBreakpoint) {
        m_exitInfo.reason = VMExitReason::Step;
    }
    return result;
}

VPExecutionStatus NVMMVirtualProcessor::HandleExecResult() {
    // TODO: If registers are cached in this class, mark registers as dirty
    // (typically by setting a bool flag to true)

    // TODO: Handle exit status (typically a big switch block)

    return VPExecutionStatus::OK;
}

bool NVMMVirtualProcessor::PrepareInterrupt(uint8_t vector) {
    // TODO: Some hypervisors (e.g. WHPX) won't stop running the VCPU if no
    // exit conditions are met. This method gives a chance to cancel, pause
    // or otherwise interrupt execution so that an interrupt can be injected.
    // If that's the case with Hypervisor.Platform, then do it here.
    // Otherwise, just return true.
    return true;
}

VPOperationStatus NVMMVirtualProcessor::InjectInterrupt(uint8_t vector) {
    // TODO: Inject interrupt
    return VPOperationStatus::OK;
}

bool NVMMVirtualProcessor::CanInjectInterrupt() const {
    // TODO: If NVMM provides a flag indicating that there is an open window
    // for interrupt injection, return that here, otherwise just return true.
    return true;
}

void NVMMVirtualProcessor::RequestInterruptWindow() {
    // TODO: If NVMM requires a window for interrupt injection to be opened, do
    // it here
}

// ----- Registers ------------------------------------------------------------

VPOperationStatus NVMMVirtualProcessor::RegRead(const Reg reg, RegValue& value) {
    // TODO: Read the specified register into the value. Be sure to zero extend
    // smaller registers, since client code might access a value type larger
    // than the register.

    // If registers are cached, refresh them before reading.
    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::RegWrite(const Reg reg, const RegValue& value) {
    // TODO: Write the specified register to the VCPU state.
    // If NVMM provides a way to access smaller registers such as AH, AL, AX
    // and EAX directly, use them as the hypervisor will likely perform zero
    // extensions correctly. Otherwise, make sure to zero extend and/or
    // preserve bits according to the x86 rules. Use http://sandpile.org/
    // for quick reference.

    // Additionally, if you're caching registers, set the dirty flag now.
    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::RegRead(const Reg regs[], RegValue values[], const size_t numRegs) {
    // TODO: Read the specified registers into the values.
    // If NVMM provides a more efficient way of reading registers in bulk, use
    // that here. Otherwise, this method can be removed from this subclass, as
    // the default implementation will read registers using a for loop and
    // RegRead with individual registers and values

    // If registers are cached, refresh them before reading.
    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) {
    // TODO: Write the specified registers to the VCPU state.
    // If NVMM provides a more efficient way of writing registers in bulk, use
    // that here. Otherwise, this method can be removed from this subclass, as
    // the default implementation will write registers using a for loop and
    // RegWrite with individual registers and values

    // Additionally, if you're caching registers, set the dirty flags now.
    return VPOperationStatus::OK;
}

// ----- Floating point control registers -------------------------------------

VPOperationStatus NVMMVirtualProcessor::GetFPUControl(FPUControl& value) {
    // TODO: Read the FPU control registers.
    // If they're cached, refresh them

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::SetFPUControl(const FPUControl& value) {
    // TODO: Write the FPU control registers.
    // If they're cached, mark them dirty.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::GetMXCSR(MXCSR& value) {
    // TODO: Read the MXCSR register.
    // Refresh registers if cached.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::SetMXCSR(const MXCSR& value) {
    // TODO: Write the MXCSR register.
    // Mark as dirty if cached.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::GetMXCSRMask(MXCSR& value) {
    // TODO: Read the MXCSR_MASK register if supported.
    // Refresh registers if cached.
    // Return VPOperationStatus::Unsupported if MXCSR_MASK is unavailable.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::SetMXCSRMask(const MXCSR& value) {
    // TODO: Write the MXCSR_MASK register if supported.
    // Mark as dirty if cached.
    // Return VPOperationStatus::Unsupported if MXCSR_MASK is unavailable.

    return VPOperationStatus::OK;
}
// ----- Model specific registers ---------------------------------------------

VPOperationStatus NVMMVirtualProcessor::GetMSR(const uint64_t msr, uint64_t& value) {
    // TODO: Read the specified MSR.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::SetMSR(const uint64_t msr, const uint64_t value) {
    // TODO: Write the specified MSR.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) {
    // TODO: Read the specified MSRs.
    // If NVMM provides an efficient way to read MSRs in bulk, use that.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) {
    // TODO: Write the specified MSRs.
    // If NVMM provides an efficient way to write MSRs in bulk, use that.

    return VPOperationStatus::OK;
}

// ----- Breakpoints ----------------------------------------------------------

VPOperationStatus NVMMVirtualProcessor::EnableSoftwareBreakpoints(bool enable) {
    // TODO: Enable software breakpoints.
    // If NVMM doesn't support guest debugging, remove this method from this
    // subclass as the default implementation returns
    // VPOperationStatus::Unsupported.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::SetHardwareBreakpoints(HardwareBreakpoints breakpoints) {
    // TODO: Configure hardware breakpoints.
    // If NVMM doesn't support guest debugging, remove this method from this
    // subclass as the default implementation returns
    // VPOperationStatus::Unsupported.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::ClearHardwareBreakpoints() {
    // TODO: Reset hardware breakpoints.
    // If NVMM doesn't support guest debugging, remove this method from this
    // subclass as the default implementation returns
    // VPOperationStatus::Unsupported.

    return VPOperationStatus::OK;
}

VPOperationStatus NVMMVirtualProcessor::GetBreakpointAddress(uint64_t *address) const {
    // TODO: Determine breakpoint address by checking the debug registers.
    // If NVMM doesn't support guest debugging, remove this method from this
    // subclass as the default implementation returns
    // VPOperationStatus::Unsupported.

    return VPOperationStatus::OK;
}

}
