/*
Implementation of the Windows Hypervisor Platform VirtualProcessor class.
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
#include "whpx_vp.hpp"
#include "whpx_vm.hpp"
#include "whpx_regs.hpp"
#include "whpx_dispatch.hpp"

#include <cassert>
#include <new>

#include <Windows.h>

#define WHV_IO_IN  0
#define WHV_IO_OUT 1

namespace virt86::whpx {

WhpxVirtualProcessor::WhpxVirtualProcessor(WhpxVirtualMachine& vm, const WhpxDispatch& dispatch, uint32_t id)
    : VirtualProcessor(vm)
    , m_vm(vm)
    , m_dispatch(dispatch)
    , m_id(id)
    , m_emuHandle(INVALID_HANDLE_VALUE)
{
    ZeroMemory(&m_exitContext, sizeof(m_exitContext));
}

WhpxVirtualProcessor::~WhpxVirtualProcessor() noexcept {
    if (m_emuHandle != INVALID_HANDLE_VALUE) {
        m_dispatch.WHvEmulatorDestroyEmulator(m_emuHandle);
        m_emuHandle = INVALID_HANDLE_VALUE;
    }
    m_dispatch.WHvDeleteVirtualProcessor(m_vm.Handle(), m_id);
}

bool WhpxVirtualProcessor::Initialize() noexcept {
    HRESULT hr = m_dispatch.WHvCreateVirtualProcessor(m_vm.Handle(), m_id, 0);
    if (S_OK != hr) {
        return false;
    }

    WHV_EMULATOR_CALLBACKS callbacks;
    callbacks.Size = sizeof(WHV_EMULATOR_CALLBACKS);
    callbacks.Reserved = 0;
    callbacks.WHvEmulatorGetVirtualProcessorRegisters = GetVirtualProcessorRegistersCallback;
    callbacks.WHvEmulatorSetVirtualProcessorRegisters = SetVirtualProcessorRegistersCallback;
    callbacks.WHvEmulatorTranslateGvaPage = TranslateGvaPageCallback;
    callbacks.WHvEmulatorIoPortCallback = IoPortCallback;
    callbacks.WHvEmulatorMemoryCallback = MemoryCallback;
    hr = m_dispatch.WHvEmulatorCreateEmulator(&callbacks, &m_emuHandle);
    if (S_OK != hr) {
        m_dispatch.WHvDeleteVirtualProcessor(m_vm.Handle(), m_id);
        return false;
    }

    // WHPX incorrectly initializes the CS with the base address set to 000F0000H.
    // According to Intel® 64 and IA-32 Architectures Software Developer’s Manual,
    // Volume 3B, section 9.1.4 (available at https://software.intel.com/sites/default/files/managed/a4/60/325384-sdm-vol-3abcd.pdf):
    //
    //   "The first instruction that is fetched and executed following a hardware
    //    reset is located at physical address FFFFFFF0H.
    //
    //    [...]
    //
    //   "[D]uring a hardware reset, the segment selector in the CS register is
    //    loaded with F000H and the base address is loaded with FFFF0000H. The
    //    starting address is thus formed by adding the base address to the
    //    value in the EIP register (that is, FFFF0000 + FFF0H = FFFFFFF0H)."
    //
    // Additionally, RDX is initialized to a non-zero value. Let's rectify that
    // while we're at it.
    const WHV_REGISTER_NAME regs[] = { WHvX64RegisterCs, WHvX64RegisterRdx };
    WHV_REGISTER_VALUE values[ARRAYSIZE(regs)];
    values[0].Segment.Base = 0xffff0000;
    values[0].Segment.Limit = 0xffff;
    values[0].Segment.Selector = 0xf000;
    values[0].Segment.Attributes = 0x0093;
    values[1].Reg64 = 0;
    hr = m_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, regs, ARRAYSIZE(regs), values);
    if (S_OK != hr) {
        m_dispatch.WHvEmulatorDestroyEmulator(m_emuHandle);
        m_emuHandle = INVALID_HANDLE_VALUE;
        m_dispatch.WHvDeleteVirtualProcessor(m_vm.Handle(), m_id);
        return false;
    }

    return true;
}

VPExecutionStatus WhpxVirtualProcessor::RunImpl() noexcept {
    const HRESULT hr = m_dispatch.WHvRunVirtualProcessor(m_vm.Handle(), m_id, &m_exitContext, sizeof(m_exitContext));
    if (S_OK != hr) {
        return VPExecutionStatus::Failed;
    }
    switch (m_exitContext.ExitReason) {
    case WHvRunVpExitReasonX64Halt:                  m_exitInfo.reason = VMExitReason::HLT;       break;  // HLT instruction
    case WHvRunVpExitReasonX64MsrAccess:             HandleMSRAccess();                           break;  // MSR access
    case WHvRunVpExitReasonX64Cpuid:                 HandleCPUIDAccess();                         break;  // CPUID instruction
    case WHvRunVpExitReasonX64IoPortAccess:          return HandleIO();                                   // I/O (IN / OUT instructions)
    case WHvRunVpExitReasonMemoryAccess:             return HandleMMIO();                                 // MMIO
    case WHvRunVpExitReasonX64InterruptWindow:       m_exitInfo.reason = VMExitReason::Interrupt; break;  // Interrupt window
    case WHvRunVpExitReasonCanceled:                 m_exitInfo.reason = VMExitReason::Cancelled; break;  // Execution cancelled
    case WHvRunVpExitReasonNone:                     m_exitInfo.reason = VMExitReason::Normal;    break;  // VM exited for no reason
    case WHvRunVpExitReasonException:                m_exitInfo.reason = VMExitReason::Exception; break;  // CPU raised an exception
    case WHvRunVpExitReasonUnsupportedFeature:       m_exitInfo.reason = VMExitReason::Error;     break;  // Host CPU does not support a feature needed by the hypervisor
    case WHvRunVpExitReasonInvalidVpRegisterValue:   m_exitInfo.reason = VMExitReason::Error;     break;  // VCPU has an invalid register
    case WHvRunVpExitReasonUnrecoverableException:   m_exitInfo.reason = VMExitReason::Error;     break;  // Unrecoverable exception
    default:                                         m_exitInfo.reason = VMExitReason::Unhandled; break;  // Unknown reason (possibly new reason from a newer WHPX)
    }

    return VPExecutionStatus::OK;
}

VPExecutionStatus WhpxVirtualProcessor::HandleIO() noexcept {
    WHV_EMULATOR_STATUS emuStatus;

    const HRESULT hr = m_dispatch.WHvEmulatorTryIoEmulation(m_emuHandle, this, &m_exitContext.VpContext, &m_exitContext.IoPortAccess, &emuStatus);
    if (S_OK != hr) {
        return VPExecutionStatus::Failed;
    }
    if (!emuStatus.EmulationSuccessful) {
        return VPExecutionStatus::Failed;
    }

    m_exitInfo.reason = VMExitReason::PIO;
    
    return VPExecutionStatus::OK;
}

VPExecutionStatus WhpxVirtualProcessor::HandleMMIO() noexcept {
    WHV_EMULATOR_STATUS emuStatus;

    const HRESULT hr = m_dispatch.WHvEmulatorTryMmioEmulation(m_emuHandle, this, &m_exitContext.VpContext, &m_exitContext.MemoryAccess, &emuStatus);
    if (S_OK != hr) {
        return VPExecutionStatus::Failed;
    }
    if (!emuStatus.EmulationSuccessful) {
        return VPExecutionStatus::Failed;
    }

    m_exitInfo.reason = VMExitReason::MMIO;

    return VPExecutionStatus::OK;
}

void WhpxVirtualProcessor::HandleMSRAccess() noexcept {
    m_exitInfo.reason = VMExitReason::MSRAccess;
    m_exitInfo.msr.isWrite = m_exitContext.MsrAccess.AccessInfo.IsWrite == TRUE;
    m_exitInfo.msr.msrNumber = m_exitContext.MsrAccess.MsrNumber;
    m_exitInfo.msr.rax = m_exitContext.MsrAccess.Rax;
    m_exitInfo.msr.rdx = m_exitContext.MsrAccess.Rdx;
}

void WhpxVirtualProcessor::HandleCPUIDAccess() noexcept {
    m_exitInfo.reason = VMExitReason::CPUID;
    m_exitInfo.cpuid.rax = m_exitContext.CpuidAccess.Rax;
    m_exitInfo.cpuid.rcx = m_exitContext.CpuidAccess.Rcx;
    m_exitInfo.cpuid.rdx = m_exitContext.CpuidAccess.Rdx;
    m_exitInfo.cpuid.rbx = m_exitContext.CpuidAccess.Rbx;
    m_exitInfo.cpuid.defaultRax = m_exitContext.CpuidAccess.DefaultResultRax;
    m_exitInfo.cpuid.defaultRcx = m_exitContext.CpuidAccess.DefaultResultRcx;
    m_exitInfo.cpuid.defaultRdx = m_exitContext.CpuidAccess.DefaultResultRdx;
    m_exitInfo.cpuid.defaultRbx = m_exitContext.CpuidAccess.DefaultResultRbx;
}

bool WhpxVirtualProcessor::PrepareInterrupt(uint8_t vector) noexcept {
    const HRESULT hr = m_dispatch.WHvCancelRunVirtualProcessor(m_vm.Handle(), m_id, 0);
    if (S_OK != hr) {
        return false;
    }

    return true;
}

VPOperationStatus WhpxVirtualProcessor::InjectInterrupt(uint8_t vector) noexcept {
    const WHV_REGISTER_NAME reg[] = { WHvRegisterPendingInterruption };
    WHV_REGISTER_VALUE val[1];
    HRESULT hr = m_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    // Setup the interrupt
    val[0].PendingInterruption.InterruptionPending = TRUE;
    val[0].PendingInterruption.InterruptionType = WHvX64PendingInterrupt;
    val[0].PendingInterruption.InterruptionVector = vector;

    hr = m_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

bool WhpxVirtualProcessor::CanInjectInterrupt() const noexcept {
    // TODO: how to check for this?
    return true;
}

void WhpxVirtualProcessor::RequestInterruptWindow() noexcept {
    // TODO: how to do this?
}

// ----- Registers ------------------------------------------------------------

VPOperationStatus WhpxVirtualProcessor::RegRead(const Reg reg, RegValue& value) noexcept {
    return RegRead(&reg, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::RegWrite(const Reg reg, const RegValue& value) noexcept {
    return RegWrite(&reg, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::RegRead(const Reg regs[], RegValue values[], const size_t numRegs) noexcept {
    if (regs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    WHV_REGISTER_NAME *whvRegs = new(std::nothrow) WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *whvVals = new(std::nothrow) WHV_REGISTER_VALUE[numRegs];

    if (whvRegs == nullptr || whvVals == nullptr) {
        delete[] whvRegs;
        delete[] whvVals;
        return VPOperationStatus::Failed;
    }

    for (int i = 0; i < numRegs; i++) {
        const auto xlatResult = TranslateRegisterName(regs[i], whvRegs[i]);
        if (xlatResult != VPOperationStatus::OK) {
            delete[] whvRegs;
            delete[] whvVals;
            return xlatResult;
        }
    }

    if (S_OK != m_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, whvRegs, numRegs, whvVals)) {
        delete[] whvVals;
        delete[] whvRegs;
        return VPOperationStatus::Failed;
    }

    for (int i = 0; i < numRegs; i++) {
        values[i] = TranslateRegisterValue(regs[i], whvVals[i]);
    }

    delete[] whvVals;
    delete[] whvRegs;
    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) noexcept {
    if (regs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    WHV_REGISTER_NAME *whvRegs = new(std::nothrow) WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *whvVals = new(std::nothrow) WHV_REGISTER_VALUE[numRegs];

    if (whvRegs == nullptr || whvVals == nullptr) {
        delete[] whvRegs;
        delete[] whvVals;
        return VPOperationStatus::Failed;
    }

    for (int i = 0; i < numRegs; i++) {
        const auto xlatResult = TranslateRegisterName(regs[i], whvRegs[i]);
        if (xlatResult != VPOperationStatus::OK) {
            delete[] whvVals;
            delete[] whvRegs;
            return xlatResult;
        }
    }

    // Get current register values in order to preserve or zero-extend values
    if (S_OK != m_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, whvRegs, numRegs, whvVals)) {
        delete[] whvVals;
        delete[] whvRegs;
        return VPOperationStatus::Failed;
    }

    for (int i = 0; i < numRegs; i++) {
        TranslateRegisterValue(regs[i], values[i], whvVals[i]);
    }

    const HRESULT hr = m_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, whvRegs, numRegs, whvVals);
    const VPOperationStatus result = (S_OK == hr) ? VPOperationStatus::OK : VPOperationStatus::Failed;

    delete[] whvVals;
    delete[] whvRegs;
    return result;
}

// ----- Floating point control registers -------------------------------------

VPOperationStatus WhpxVirtualProcessor::GetFPUControl(FPUControl& value) noexcept {
    const WHV_REGISTER_NAME reg[2] = { WHvX64RegisterFpControlStatus, WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[2];

    const HRESULT hr = m_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 2, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    const auto& fp = val[0].FpControlStatus;
    const auto& mm = val[1].XmmControlStatus;
    value.cw = fp.FpControl;
    value.sw = fp.FpStatus;
    value.tw = fp.FpTag;
    value.op = fp.LastFpOp;
    value.cs = fp.LastFpCs;
    value.rip = fp.LastFpRip;
    value.ds = mm.LastFpDs;
    value.rdp = mm.LastFpRdp;

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::SetFPUControl(const FPUControl& value) noexcept {
    const WHV_REGISTER_NAME reg[2] = { WHvX64RegisterFpControlStatus, WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[2] = { 0 };

    auto& fp = val[0].FpControlStatus;
    auto& mm = val[1].XmmControlStatus;
    fp.FpControl = value.cw;
    fp.FpStatus = value.sw;
    fp.FpTag = value.tw;
    fp.LastFpOp = value.op;
    fp.LastFpCs = value.cs;
    fp.LastFpRip = value.rip;
    mm.LastFpDs = value.ds;
    mm.LastFpRdp = value.rdp;

    const HRESULT hr = m_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 2, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::GetMXCSR(MXCSR& value) noexcept {
    const WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1];

    const HRESULT hr = m_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    const auto& mm = val[0].XmmControlStatus;
    value.u32 = mm.XmmStatusControl;

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::SetMXCSR(const MXCSR& value) noexcept {
    const WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1] = { 0 };

    auto& mm = val[0].XmmControlStatus;
    mm.XmmStatusControl = value.u32;

    const HRESULT hr = m_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::GetMXCSRMask(MXCSR& value) noexcept {
    const WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1];

    const HRESULT hr = m_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    const auto& mm = val[0].XmmControlStatus;
    value.u32 = mm.XmmStatusControlMask;

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::SetMXCSRMask(const MXCSR& value) noexcept {
    const WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1] = { 0 };

    auto& mm = val[0].XmmControlStatus;
    mm.XmmStatusControlMask = value.u32;

    const HRESULT hr = m_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

// ----- Model specific registers ---------------------------------------------

VPOperationStatus WhpxVirtualProcessor::GetMSR(const uint64_t msr, uint64_t& value) noexcept {
    return GetMSRs(&msr, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::SetMSR(const uint64_t msr, const uint64_t value) noexcept {
    return SetMSRs(&msr, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) noexcept {
    if (msrs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    WHV_REGISTER_NAME *regs = new(std::nothrow) WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *vals = new(std::nothrow) WHV_REGISTER_VALUE[numRegs];

    if (regs == nullptr || vals == nullptr) {
        delete[] regs;
        delete[] vals;
        return VPOperationStatus::Failed;
    }

    for (size_t i = 0; i < numRegs; i++) {
        if (!TranslateMSR(msrs[i], regs[i])) {
            delete[] regs;
            delete[] vals;
            return VPOperationStatus::InvalidRegister;
        }
    }

    const HRESULT hr = m_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, regs, numRegs, vals);
    const VPOperationStatus result = (S_OK == hr) ? VPOperationStatus::OK : VPOperationStatus::Failed;
    if (S_OK == hr) {
        for (size_t i = 0; i < numRegs; i++) {
            values[i] = vals[i].Reg64;
        }
    }

    delete[] regs;
    delete[] vals;
    return result;
}

VPOperationStatus WhpxVirtualProcessor::SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) noexcept {
    if (msrs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    WHV_REGISTER_NAME *regs = new(std::nothrow) WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *vals = new(std::nothrow) WHV_REGISTER_VALUE[numRegs];

    if (regs == nullptr || vals == nullptr) {
        delete[] regs;
        delete[] vals;
        return VPOperationStatus::Failed;
    }

    for (size_t i = 0; i < numRegs; i++) {
        if (!TranslateMSR(msrs[i], regs[i])) {
            delete[] regs;
            delete[] vals;
            return VPOperationStatus::InvalidRegister;
        }
        vals[i].Reg64 = values[i];
    }

    const HRESULT hr = m_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, regs, numRegs, vals);
    const VPOperationStatus result = (S_OK == hr) ? VPOperationStatus::OK : VPOperationStatus::Failed;

    delete[] regs;
    delete[] vals;
    return VPOperationStatus::OK;
}

// ----- WHPX instruction emulator callbacks ----------------------------------

HRESULT WhpxVirtualProcessor::GetVirtualProcessorRegistersCallback(
    VOID* Context,
    const WHV_REGISTER_NAME* RegisterNames,
    UINT32 RegisterCount,
    WHV_REGISTER_VALUE* RegisterValues
) noexcept {
    const WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return vp->m_dispatch.WHvGetVirtualProcessorRegisters(vp->m_vm.Handle(), vp->m_id, RegisterNames, RegisterCount, RegisterValues);
}

HRESULT WhpxVirtualProcessor::SetVirtualProcessorRegistersCallback(
    VOID* Context,
    const WHV_REGISTER_NAME* RegisterNames,
    UINT32 RegisterCount,
    const WHV_REGISTER_VALUE* RegisterValues
) noexcept {
    const WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return vp->m_dispatch.WHvSetVirtualProcessorRegisters(vp->m_vm.Handle(), vp->m_id, RegisterNames, RegisterCount, RegisterValues);
}

HRESULT WhpxVirtualProcessor::TranslateGvaPageCallback(VOID* Context, WHV_GUEST_VIRTUAL_ADDRESS Gva, WHV_TRANSLATE_GVA_FLAGS TranslateFlags, WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult, WHV_GUEST_PHYSICAL_ADDRESS* Gpa) noexcept {
    const WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    WHV_TRANSLATE_GVA_RESULT result;
    const HRESULT hr = vp->m_dispatch.WHvTranslateGva(vp->m_vm.Handle(), vp->m_id, Gva, TranslateFlags, &result, Gpa);
    if (S_OK == hr) {
        *TranslationResult = result.ResultCode;
    }
    return hr;
}

HRESULT WhpxVirtualProcessor::IoPortCallback(VOID *Context, WHV_EMULATOR_IO_ACCESS_INFO *IoAccess) noexcept {
    WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return vp->HandleIO(IoAccess);
}

HRESULT WhpxVirtualProcessor::MemoryCallback(VOID *Context, WHV_EMULATOR_MEMORY_ACCESS_INFO *MemoryAccess) noexcept {
    WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return vp->HandleMMIO(MemoryAccess);
}

HRESULT WhpxVirtualProcessor::HandleIO(WHV_EMULATOR_IO_ACCESS_INFO *IoAccess) noexcept {
    if (IoAccess->Direction == WHV_IO_IN) {
        IoAccess->Data = m_io.IORead(IoAccess->Port, IoAccess->AccessSize);
    }
    else {
        m_io.IOWrite(IoAccess->Port, IoAccess->AccessSize, IoAccess->Data);
    }

    return S_OK;
}

HRESULT WhpxVirtualProcessor::HandleMMIO(WHV_EMULATOR_MEMORY_ACCESS_INFO *MemoryAccess) noexcept {
    if (MemoryAccess->Direction == WHV_IO_IN) {
        uint64_t value = m_io.MMIORead(MemoryAccess->GpaAddress, MemoryAccess->AccessSize);
        switch (MemoryAccess->AccessSize) {
        case 1: *MemoryAccess->Data = value; break;
        case 2: *reinterpret_cast<uint16_t *>(MemoryAccess->Data) = value; break;
        case 4: *reinterpret_cast<uint32_t *>(MemoryAccess->Data) = value; break;
        case 8: *reinterpret_cast<uint64_t *>(MemoryAccess->Data) = value; break;
        default: assert(0);  // should never happen
        }
    }
    else {
        switch (MemoryAccess->AccessSize) {
        case 1: m_io.MMIOWrite(MemoryAccess->GpaAddress, MemoryAccess->AccessSize, *MemoryAccess->Data); break;
        case 2: m_io.MMIOWrite(MemoryAccess->GpaAddress, MemoryAccess->AccessSize, *reinterpret_cast<uint16_t *>(MemoryAccess->Data)); break;
        case 4: m_io.MMIOWrite(MemoryAccess->GpaAddress, MemoryAccess->AccessSize, *reinterpret_cast<uint32_t *>(MemoryAccess->Data)); break;
        case 8: m_io.MMIOWrite(MemoryAccess->GpaAddress, MemoryAccess->AccessSize, *reinterpret_cast<uint64_t *>(MemoryAccess->Data)); break;
        default: assert(0);  // should never happen
        }
    }
    
    return S_OK;
}

}
