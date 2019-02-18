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

#include <Windows.h>

#define WHV_IO_IN  0
#define WHV_IO_OUT 1

namespace virt86::whpx {

WhpxVirtualProcessor::WhpxVirtualProcessor(WhpxVirtualMachine& vm, uint32_t id)
    : VirtualProcessor(vm)
    , m_vm(vm)
    , m_id(id)
    , m_emuHandle(INVALID_HANDLE_VALUE)
{
    ZeroMemory(&m_exitContext, sizeof(m_exitContext));
}

WhpxVirtualProcessor::~WhpxVirtualProcessor() {
    if (m_emuHandle != INVALID_HANDLE_VALUE) {
        g_dispatch.WHvEmulatorDestroyEmulator(m_emuHandle);
        m_emuHandle = INVALID_HANDLE_VALUE;
    }
    g_dispatch.WHvDeleteVirtualProcessor(m_vm.Handle(), m_id);
}

bool WhpxVirtualProcessor::Initialize() {
    HRESULT hr = g_dispatch.WHvCreateVirtualProcessor(m_vm.Handle(), m_id, 0);
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
    hr = g_dispatch.WHvEmulatorCreateEmulator(&callbacks, &m_emuHandle);
    if (S_OK != hr) {
        g_dispatch.WHvDeleteVirtualProcessor(m_vm.Handle(), m_id);
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
    WHV_REGISTER_NAME regs[] = { WHvX64RegisterCs, WHvX64RegisterRdx };
    WHV_REGISTER_VALUE values[ARRAYSIZE(regs)];
    values[0].Segment.Base = 0xffff0000;
    values[0].Segment.Limit = 0xffff;
    values[0].Segment.Selector = 0xf000;
    values[0].Segment.Attributes = 0x0093;
    values[1].Reg64 = 0;
    hr = g_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, regs, ARRAYSIZE(regs), values);
    if (S_OK != hr) {
        g_dispatch.WHvEmulatorDestroyEmulator(m_emuHandle);
        m_emuHandle = INVALID_HANDLE_VALUE;
        g_dispatch.WHvDeleteVirtualProcessor(m_vm.Handle(), m_id);
        return false;
    }

    return true;
}

VPExecutionStatus WhpxVirtualProcessor::RunImpl() {
    HRESULT hr = g_dispatch.WHvRunVirtualProcessor(m_vm.Handle(), m_id, &m_exitContext, sizeof(m_exitContext));
    if (S_OK != hr) {
        return VPExecutionStatus::Failed;
    }
    switch (m_exitContext.ExitReason) {
    case WHvRunVpExitReasonX64Halt:                  m_exitInfo.reason = VMExitReason::HLT;       break;  // HLT instruction
    case WHvRunVpExitReasonX64IoPortAccess:          return HandleIO();                                   // I/O (IN / OUT instructions)
    case WHvRunVpExitReasonMemoryAccess:             return HandleMMIO();                                 // MMIO
    case WHvRunVpExitReasonX64InterruptWindow:       m_exitInfo.reason = VMExitReason::Interrupt; break;  // Interrupt window
    case WHvRunVpExitReasonCanceled:                 m_exitInfo.reason = VMExitReason::Cancelled; break;  // Execution cancelled
    case WHvRunVpExitReasonNone:                     m_exitInfo.reason = VMExitReason::Normal;    break;  // VM exited for no reason
    case WHvRunVpExitReasonX64MsrAccess:             m_exitInfo.reason = VMExitReason::MSRAccess; break;  // MSR access
    case WHvRunVpExitReasonX64Cpuid:                 m_exitInfo.reason = VMExitReason::CPUID;     break;  // CPUID instruction
    case WHvRunVpExitReasonException:                m_exitInfo.reason = VMExitReason::Exception; break;  // CPU raised an exception
    case WHvRunVpExitReasonUnsupportedFeature:       m_exitInfo.reason = VMExitReason::Error;     break;  // Host CPU does not support a feature needed by the hypervisor
    case WHvRunVpExitReasonInvalidVpRegisterValue:   m_exitInfo.reason = VMExitReason::Error;     break;  // VCPU has an invalid register
    case WHvRunVpExitReasonUnrecoverableException:   m_exitInfo.reason = VMExitReason::Error;     break;  // Unrecoverable exception
    default:                                         m_exitInfo.reason = VMExitReason::Unhandled; break;  // Unknown reason (possibly new reason from a newer WHPX)
    }

    return VPExecutionStatus::OK;
}

VPExecutionStatus WhpxVirtualProcessor::HandleIO() {
    WHV_EMULATOR_STATUS emuStatus;

    HRESULT hr = g_dispatch.WHvEmulatorTryIoEmulation(m_emuHandle, this, &m_exitContext.VpContext, &m_exitContext.IoPortAccess, &emuStatus);
    if (S_OK != hr) {
        return VPExecutionStatus::Failed;
    }
    if (!emuStatus.EmulationSuccessful) {
        return VPExecutionStatus::Failed;
    }

    m_exitInfo.reason = VMExitReason::PIO;
    
    return VPExecutionStatus::OK;
}

VPExecutionStatus WhpxVirtualProcessor::HandleMMIO() {
    WHV_EMULATOR_STATUS emuStatus;

    HRESULT hr = g_dispatch.WHvEmulatorTryMmioEmulation(m_emuHandle, this, &m_exitContext.VpContext, &m_exitContext.MemoryAccess, &emuStatus);
    if (S_OK != hr) {
        return VPExecutionStatus::Failed;
    }
    if (!emuStatus.EmulationSuccessful) {
        return VPExecutionStatus::Failed;
    }

    m_exitInfo.reason = VMExitReason::MMIO;

    return VPExecutionStatus::OK;
}

bool WhpxVirtualProcessor::PrepareInterrupt(uint8_t vector) {
    HRESULT hr = g_dispatch.WHvCancelRunVirtualProcessor(m_vm.Handle(), m_id, 0);
    if (S_OK != hr) {
        return false;
    }

    return true;
}

VPOperationStatus WhpxVirtualProcessor::InjectInterrupt(uint8_t vector) {
    WHV_REGISTER_NAME reg[] = { WHvRegisterPendingInterruption };
    WHV_REGISTER_VALUE val[1];
    HRESULT hr = g_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    // Setup the interrupt
    val[0].PendingInterruption.InterruptionPending = TRUE;
    val[0].PendingInterruption.InterruptionType = WHvX64PendingInterrupt;
    val[0].PendingInterruption.InterruptionVector = vector;

    hr = g_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

bool WhpxVirtualProcessor::CanInjectInterrupt() const {
    // TODO: how to check for this?
    return true;
}

void WhpxVirtualProcessor::RequestInterruptWindow() {
    // TODO: how to do this?
}

// ----- Registers ------------------------------------------------------------

VPOperationStatus WhpxVirtualProcessor::RegRead(const Reg reg, RegValue& value) {
    return RegRead(&reg, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::RegWrite(const Reg reg, const RegValue& value) {
    return RegWrite(&reg, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::RegRead(const Reg regs[], RegValue values[], const size_t numRegs) {
    WHV_REGISTER_NAME *whvRegs = new WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *whvVals = new WHV_REGISTER_VALUE[numRegs];

    for (int i = 0; i < numRegs; i++) {
        auto xlatResult = TranslateRegisterName(regs[i], whvRegs[i]);
        if (xlatResult != VPOperationStatus::OK) {
            return xlatResult;
        }
    }

    HRESULT hr = g_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, whvRegs, numRegs, whvVals);
    VPOperationStatus result;
    if (S_OK != hr) {
        result = VPOperationStatus::Failed;
    }
    else {
        result = VPOperationStatus::OK;
    }

    for (int i = 0; i < numRegs; i++) {
        values[i] = TranslateRegisterValue(regs[i], whvVals[i]);
    }

    delete[] whvVals;
    delete[] whvRegs;
    return result;
}

VPOperationStatus WhpxVirtualProcessor::RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) {
    WHV_REGISTER_NAME *whvRegs = new WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *whvVals = new WHV_REGISTER_VALUE[numRegs];

    for (int i = 0; i < numRegs; i++) {
        auto xlatResult = TranslateRegisterName(regs[i], whvRegs[i]);
        if (xlatResult != VPOperationStatus::OK) {
            return xlatResult;
        }
        TranslateRegisterValue(regs[i], values[i], whvVals[i]);
    }

    HRESULT hr = g_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, whvRegs, numRegs, whvVals);
    VPOperationStatus result;
    if (S_OK != hr) {
        result = VPOperationStatus::Failed;
    }
    else {
        result = VPOperationStatus::OK;
    }

    delete[] whvVals;
    delete[] whvRegs;
    return result;
}

// ----- Floating point control registers -------------------------------------

VPOperationStatus WhpxVirtualProcessor::GetFPUControl(FPUControl& value) {
    WHV_REGISTER_NAME reg[2] = { WHvX64RegisterFpControlStatus, WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[2];

    HRESULT hr = g_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 2, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    auto& fp = val[0].FpControlStatus;
    auto& mm = val[1].XmmControlStatus;
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

VPOperationStatus WhpxVirtualProcessor::SetFPUControl(const FPUControl& value) {
    WHV_REGISTER_NAME reg[2] = { WHvX64RegisterFpControlStatus, WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[2];

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

    HRESULT hr = g_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 2, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::GetMXCSR(MXCSR& value) {
    WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1];

    HRESULT hr = g_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    auto& mm = val[0].XmmControlStatus;
    value.u32 = mm.XmmStatusControl;

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::SetMXCSR(const MXCSR& value) {
    WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1];

    auto& mm = val[0].XmmControlStatus;
    mm.XmmStatusControl = value.u32;

    HRESULT hr = g_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::GetMXCSRMask(MXCSR& value) {
    WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1];

    HRESULT hr = g_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    auto& mm = val[0].XmmControlStatus;
    value.u32 = mm.XmmStatusControlMask;

    return VPOperationStatus::OK;
}

VPOperationStatus WhpxVirtualProcessor::SetMXCSRMask(const MXCSR& value) {
    WHV_REGISTER_NAME reg[1] = { WHvX64RegisterXmmControlStatus };
    WHV_REGISTER_VALUE val[1];

    auto& mm = val[0].XmmControlStatus;
    mm.XmmStatusControlMask = value.u32;

    HRESULT hr = g_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, reg, 1, val);
    if (S_OK != hr) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

// ----- Model specific registers ---------------------------------------------

VPOperationStatus WhpxVirtualProcessor::GetMSR(const uint64_t msr, uint64_t& value) {
    return GetMSRs(&msr, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::SetMSR(const uint64_t msr, const uint64_t value) {
    return SetMSRs(&msr, &value, 1);
}

VPOperationStatus WhpxVirtualProcessor::GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) {
    WHV_REGISTER_NAME *regs = new WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *vals = new WHV_REGISTER_VALUE[numRegs];

    for (size_t i = 0; i < numRegs; i++) {
        if (!TranslateMSR(msrs[i], regs[i])) {
            delete[] regs;
            delete[] vals;
            return VPOperationStatus::InvalidRegister;
        }
    }

    HRESULT hr = g_dispatch.WHvGetVirtualProcessorRegisters(m_vm.Handle(), m_id, regs, numRegs, vals);
    VPOperationStatus result;
    if (S_OK == hr) {
        for (size_t i = 0; i < numRegs; i++) {
            values[i] = vals[i].Reg64;
        }
        result = VPOperationStatus::OK;
    }
    else {
        result = VPOperationStatus::Failed;
    }

    delete[] regs;
    delete[] vals;
    return result;
}

VPOperationStatus WhpxVirtualProcessor::SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) {
    WHV_REGISTER_NAME *regs = new WHV_REGISTER_NAME[numRegs];
    WHV_REGISTER_VALUE *vals = new WHV_REGISTER_VALUE[numRegs];

    for (size_t i = 0; i < numRegs; i++) {
        if (!TranslateMSR(msrs[i], regs[i])) {
            delete[] regs;
            delete[] vals;
            return VPOperationStatus::InvalidRegister;
        }
        vals[i].Reg64 = values[i];
    }

    HRESULT hr = g_dispatch.WHvSetVirtualProcessorRegisters(m_vm.Handle(), m_id, regs, numRegs, vals);
    VPOperationStatus result;
    if (S_OK == hr) {
        result = VPOperationStatus::OK;
    }
    else {
        result = VPOperationStatus::Failed;
    }

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
) {
    WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return g_dispatch.WHvGetVirtualProcessorRegisters(vp->m_vm.Handle(), vp->m_id, RegisterNames, RegisterCount, RegisterValues);
}

HRESULT WhpxVirtualProcessor::SetVirtualProcessorRegistersCallback(
    VOID* Context,
    const WHV_REGISTER_NAME* RegisterNames,
    UINT32 RegisterCount,
    const WHV_REGISTER_VALUE* RegisterValues
) {
    WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return g_dispatch.WHvSetVirtualProcessorRegisters(vp->m_vm.Handle(), vp->m_id, RegisterNames, RegisterCount, RegisterValues);
}

HRESULT WhpxVirtualProcessor::TranslateGvaPageCallback(VOID* Context, WHV_GUEST_VIRTUAL_ADDRESS Gva, WHV_TRANSLATE_GVA_FLAGS TranslateFlags, WHV_TRANSLATE_GVA_RESULT_CODE* TranslationResult, WHV_GUEST_PHYSICAL_ADDRESS* Gpa) {
    WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    WHV_TRANSLATE_GVA_RESULT result;
    HRESULT hr = g_dispatch.WHvTranslateGva(vp->m_vm.Handle(), vp->m_id, Gva, TranslateFlags, &result, Gpa);
    if (S_OK == hr) {
        *TranslationResult = result.ResultCode;
    }
    return hr;
}

HRESULT WhpxVirtualProcessor::IoPortCallback(VOID *Context, WHV_EMULATOR_IO_ACCESS_INFO *IoAccess) {
    WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return vp->HandleIO(IoAccess);
}

HRESULT WhpxVirtualProcessor::MemoryCallback(VOID *Context, WHV_EMULATOR_MEMORY_ACCESS_INFO *MemoryAccess) {
    WhpxVirtualProcessor *vp = (WhpxVirtualProcessor *)Context;
    return vp->HandleMMIO(MemoryAccess);
}

HRESULT WhpxVirtualProcessor::HandleIO(WHV_EMULATOR_IO_ACCESS_INFO *IoAccess) {
    if (IoAccess->Direction == WHV_IO_IN) {
        IoAccess->Data = m_io.IORead(IoAccess->Port, IoAccess->AccessSize);
    }
    else {
        m_io.IOWrite(IoAccess->Port, IoAccess->AccessSize, IoAccess->Data);
    }

    return S_OK;
}

HRESULT WhpxVirtualProcessor::HandleMMIO(WHV_EMULATOR_MEMORY_ACCESS_INFO *MemoryAccess) {
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
