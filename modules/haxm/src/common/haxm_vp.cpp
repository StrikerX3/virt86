/*
Implementation of the HAXM VirtualProcessor class.
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
#include "haxm_vp.hpp"
#include "haxm_vm.hpp"
#include "haxm_helpers.hpp"

#include "virt86/util/bytemanip.hpp"

#include <cassert>

namespace virt86::haxm {

HaxmVirtualProcessor::HaxmVirtualProcessor(HaxmVirtualMachine& vm, HaxmVirtualMachineSysImpl& vmSys, uint32_t id)
    : VirtualProcessor(vm)
    , m_vm(vm)
    , m_sys(std::make_unique<HaxmVirtualProcessorSysImpl>(vm, vmSys))
    , m_regs({ 0 })
    , m_fpuRegs({ 0 })
    , m_regsDirty(true)
    , m_regsChanged(false)
    , m_fpuRegsChanged(false)
    , m_tunnel(nullptr)
    , m_ioTunnel(nullptr)
    , m_debug({ 0 })
    , m_vcpuID(id)
    , m_useEferMSR(true)
{
    // Check if we need to use the EFER MSR instead of the register state by
    // enabling the LME bit and reading back. If the register value is changed,
    // we can use the EFER register value from m_regs, otherwise we'll have to
    // use the MSR. The register value is reverted after the test.
    // TODO: find a less invasive method to check for this
    if (m_sys->GetRegisters(&m_regs)) {
        uint32_t oldEfer = m_regs._efer;
        m_regs._efer ^= EFER_LME;
        if (m_sys->SetRegisters(&m_regs)) {
            if (m_sys->GetRegisters(&m_regs)) {
                uint32_t newEfer = m_regs._efer;
                if (oldEfer != newEfer) {
                    m_useEferMSR = false;
                }
            }
            m_regs._efer = oldEfer;
            m_sys->SetRegisters(&m_regs);
        }
    }
}

HaxmVirtualProcessor::~HaxmVirtualProcessor() noexcept {
}

bool HaxmVirtualProcessor::Initialize() noexcept {
    return m_sys->Initialize(m_vcpuID, &m_tunnel, &m_ioTunnel);
}

VPExecutionStatus HaxmVirtualProcessor::RunImpl() noexcept {
    UpdateRegisters();
    
    if (!m_sys->Run()) {
        return VPExecutionStatus::Failed;
    }

    return HandleExecResult();
}

VPExecutionStatus HaxmVirtualProcessor::StepImpl() noexcept {
    m_debug.control |= HAX_DEBUG_STEP;
    if (!SetDebug()) {
        return VPExecutionStatus::Failed;
    }

    const auto status = RunImpl();

    m_debug.control &= ~HAX_DEBUG_STEP;
    if (!SetDebug()) {
        return VPExecutionStatus::Failed;
    }

    if (status != VPExecutionStatus::OK) {
        return status;
    }

    const auto result = HandleExecResult();
    if (m_exitInfo.reason == VMExitReason::SoftwareBreakpoint) {
        m_exitInfo.reason = VMExitReason::Step;
    }
    return result;
}

bool HaxmVirtualProcessor::SetDebug() noexcept {
    const bool enable = (m_debug.control & ~HAX_DEBUG_ENABLE) != 0;
    if (enable) {
        m_debug.control |= HAX_DEBUG_ENABLE;
    }
    else {
        m_debug.control &= ~HAX_DEBUG_ENABLE;
    }

    return m_sys->SetDebug(&m_debug);
}

void HaxmVirtualProcessor::UpdateRegisters() noexcept {
    // Update CPU state if registers were modified
    if (m_regsChanged) {
        m_sys->SetRegisters(&m_regs);
        m_regsChanged = false;
    }
    if (m_fpuRegsChanged) {
        m_sys->SetFPURegisters(&m_fpuRegs);
        m_fpuRegsChanged = false;
    }
}

bool HaxmVirtualProcessor::RefreshRegisters() noexcept {
    // Retrieve registers from CPU state
    if (m_regsDirty) {
        if (!m_sys->GetRegisters(&m_regs)) {
            return false;
        }
        if (!m_sys->GetFPURegisters(&m_fpuRegs)) {
            return false;
        }
        m_regsDirty = false;
    }
    return true;
}

VPExecutionStatus HaxmVirtualProcessor::HandleExecResult() noexcept {
    // Mark registers as dirty
    m_regsDirty = true;

    // Handle exit status using tunnel
    switch (m_tunnel->_exit_status) {
    case HAX_EXIT_HLT:         m_exitInfo.reason = VMExitReason::HLT;       break;  // HLT instruction
    case HAX_EXIT_INTERRUPT:   m_exitInfo.reason = VMExitReason::Interrupt; break;  // Interrupt window
    case HAX_EXIT_PAUSED:      m_exitInfo.reason = VMExitReason::Normal;    break;  // Let HAXM handle this
    case HAX_EXIT_MMIO:        m_exitInfo.reason = VMExitReason::Error;     break;  // Regular MMIO (cannot be implemented)
    case HAX_EXIT_REALMODE:    m_exitInfo.reason = VMExitReason::Error;     break;  // Real mode is not supported
    case HAX_EXIT_UNKNOWN:     m_exitInfo.reason = VMExitReason::Error;     break;  // VM failed for an unknown reason
    case HAX_EXIT_STATECHANGE: m_exitInfo.reason = VMExitReason::Shutdown;  break;  // The VM is shutting down
    case HAX_EXIT_IO:          HandleIO();                                  break;  // I/O (in / out instructions)
    case HAX_EXIT_FAST_MMIO:   HandleFastMMIO();                            break;  // Fast MMIO
    case HAX_EXIT_DEBUG:                                                            // A breakpoint was hit
    {
        // Determine if it was a software or hardware breakpoint
        if (m_tunnel->debug.dr6 & 0xf) {
            m_exitInfo.reason = VMExitReason::HardwareBreakpoint;
        }
        else {
            m_exitInfo.reason = VMExitReason::SoftwareBreakpoint;
        }
        break;
    }
    default: m_exitInfo.reason = VMExitReason::Unhandled; break;   // Unknown reason (possibly new reason from a newer HAXM driver)
    }

    return VPExecutionStatus::OK;
}

void HaxmVirtualProcessor::HandleIO() noexcept {
    m_exitInfo.reason = VMExitReason::PIO;
    
    uint8_t *ptr = static_cast<uint8_t *>(m_ioTunnel);
    if (m_tunnel->io._df) {
        ptr += static_cast<uintptr_t>(m_tunnel->io._size) * (m_tunnel->io._count - 1);
    }

    for (uint16_t i = 0; i < m_tunnel->io._count; i++) {
        if (m_tunnel->io._direction == HAX_IO_OUT) {
            uint32_t value;
            switch (m_tunnel->io._size) {
            case 1: value = *ptr; break;
            case 2: value = *reinterpret_cast<uint16_t *>(ptr); break;
            case 4: value = *reinterpret_cast<uint32_t *>(ptr); break;
            default: assert(0); // should not happen
            }
            m_io.IOWrite(m_tunnel->io._port, m_tunnel->io._size, value);
        }
        else {
            uint32_t value = m_io.IORead(m_tunnel->io._port, m_tunnel->io._size);
            switch (m_tunnel->io._size) {
            case 1: *ptr = static_cast<uint8_t>(value); break;
            case 2: *reinterpret_cast<uint16_t *>(ptr) = static_cast<uint16_t>(value); break;
            case 4: *reinterpret_cast<uint32_t *>(ptr) = value; break;
            default: assert(0); // should not happen
            }
        }

        if (m_tunnel->io._df) {
            ptr -= m_tunnel->io._size;
        }
        else {
            ptr += m_tunnel->io._size;
        }
    }
}

void HaxmVirtualProcessor::HandleFastMMIO() noexcept {
    m_exitInfo.reason = VMExitReason::MMIO;

    hax_fastmmio *info = static_cast<hax_fastmmio *>(m_ioTunnel);

    if (info->direction < 2) {
        if (info->direction == HAX_IO_IN) {
            m_io.MMIOWrite(info->gpa, info->size, info->value);
        }
        else {
            info->value = m_io.MMIORead(info->gpa, info->size);
        }
    }
    else {
        // HAX API v4 supports transferring data between two MMIO addresses,
        // info->gpa and info->gpa2 (instructions such as MOVS require this):
        //  info->direction == 2: gpa ==> gpa2

        const uint64_t value = m_io.MMIORead(info->gpa, info->size);
        m_io.MMIOWrite(info->gpa2, info->size, value);
    }
}

bool HaxmVirtualProcessor::PrepareInterrupt(uint8_t vector) noexcept {
    return true;
}

VPOperationStatus HaxmVirtualProcessor::InjectInterrupt(uint8_t vector) noexcept {
    if (!m_sys->InjectInterrupt(vector)) {
        return VPOperationStatus::Failed;
    }

    m_tunnel->request_interrupt_window = 0;

    return VPOperationStatus::OK;
}

bool HaxmVirtualProcessor::CanInjectInterrupt() const noexcept {
    return m_tunnel->ready_for_interrupt_injection != 0;
}

void HaxmVirtualProcessor::RequestInterruptWindow() noexcept {
    m_tunnel->request_interrupt_window = 1;
}

// ----- Registers ------------------------------------------------------------

#define REFRESH_REGISTERS do { \
    const auto status = RefreshRegisters(); \
    if (!status) { \
        return VPOperationStatus::Failed; \
    } \
} while (0)

static inline uint64_t fixupFlags(uint64_t flags) noexcept {
    return (flags | 0x2) & ~0x8028;
}

VPOperationStatus HaxmVirtualProcessor::RegRead(const Reg reg, RegValue& value) noexcept {
    REFRESH_REGISTERS;
    return HaxmRegRead(reg, value);
}

VPOperationStatus HaxmVirtualProcessor::RegWrite(const Reg reg, const RegValue& value) noexcept {
    REFRESH_REGISTERS;
    return HaxmRegWrite(reg, value);
}

VPOperationStatus HaxmVirtualProcessor::RegRead(const Reg regs[], RegValue values[], const size_t numRegs) noexcept {
    if (regs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    REFRESH_REGISTERS;

    for (size_t i = 0; i < numRegs; i++) {
        const auto result = HaxmRegRead(regs[i], values[i]);
        if (result != VPOperationStatus::OK) {
            return result;
        }
    }

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) noexcept {
    if (regs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    REFRESH_REGISTERS;

    for (size_t i = 0; i < numRegs; i++) {
        const auto result = HaxmRegWrite(regs[i], values[i]);
        if (result != VPOperationStatus::OK) {
            return result;
        }
    }
  
    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::HaxmRegRead(const Reg reg, RegValue& value) noexcept {
    switch (reg) {
    case Reg::CS:   LoadSegment(value, &m_regs._cs);  break;
    case Reg::SS:   LoadSegment(value, &m_regs._ss);  break;
    case Reg::DS:   LoadSegment(value, &m_regs._ds);  break;
    case Reg::ES:   LoadSegment(value, &m_regs._es);  break;
    case Reg::FS:   LoadSegment(value, &m_regs._fs);  break;
    case Reg::GS:   LoadSegment(value, &m_regs._gs);  break;
    case Reg::LDTR: LoadSegment(value, &m_regs._ldt); break;
    case Reg::TR:   LoadSegment(value, &m_regs._tr);  break;

    case Reg::GDTR: LoadTable(value, &m_regs._gdt);  break;
    case Reg::IDTR: LoadTable(value, &m_regs._idt);  break;

    case Reg::AH:   value.u8 =                      m_regs._ah;      break;
    case Reg::AL:   value.u8 =                      m_regs._al;      break;
    case Reg::CH:   value.u8 =                      m_regs._ch;      break;
    case Reg::CL:   value.u8 =                      m_regs._cl;      break;
    case Reg::DH:   value.u8 =                      m_regs._dh;      break;
    case Reg::DL:   value.u8 =                      m_regs._dl;      break;
    case Reg::BH:   value.u8 =                      m_regs._bh;      break;
    case Reg::BL:   value.u8 =                      m_regs._bl;      break;
    case Reg::SPL:  value.u8 = static_cast<uint8_t>(m_regs._sp);     break;
    case Reg::BPL:  value.u8 = static_cast<uint8_t>(m_regs._bp);     break;
    case Reg::SIL:  value.u8 = static_cast<uint8_t>(m_regs._si);     break;
    case Reg::DIL:  value.u8 = static_cast<uint8_t>(m_regs._di);     break;
    case Reg::R8B:  value.u8 = static_cast<uint8_t>(m_regs._r8);     break;
    case Reg::R9B:  value.u8 = static_cast<uint8_t>(m_regs._r9);     break;
    case Reg::R10B: value.u8 = static_cast<uint8_t>(m_regs._r10);    break;
    case Reg::R11B: value.u8 = static_cast<uint8_t>(m_regs._r11);    break;
    case Reg::R12B: value.u8 = static_cast<uint8_t>(m_regs._r12);    break;
    case Reg::R13B: value.u8 = static_cast<uint8_t>(m_regs._r13);    break;
    case Reg::R14B: value.u8 = static_cast<uint8_t>(m_regs._r14);    break;
    case Reg::R15B: value.u8 = static_cast<uint8_t>(m_regs._r15);    break;

    case Reg::AX:    value.u16 =                       m_regs._ax;      break;
    case Reg::CX:    value.u16 =                       m_regs._cx;      break;
    case Reg::DX:    value.u16 =                       m_regs._dx;      break;
    case Reg::BX:    value.u16 =                       m_regs._bx;      break;
    case Reg::SI:    value.u16 =                       m_regs._si;      break;
    case Reg::DI:    value.u16 =                       m_regs._di;      break;
    case Reg::SP:    value.u16 =                       m_regs._sp;      break;
    case Reg::BP:    value.u16 =                       m_regs._bp;      break;
    case Reg::R8W:   value.u16 = static_cast<uint16_t>(m_regs._r8);     break;
    case Reg::R9W:   value.u16 = static_cast<uint16_t>(m_regs._r9);     break;
    case Reg::R10W:  value.u16 = static_cast<uint16_t>(m_regs._r10);    break;
    case Reg::R11W:  value.u16 = static_cast<uint16_t>(m_regs._r11);    break;
    case Reg::R12W:  value.u16 = static_cast<uint16_t>(m_regs._r12);    break;
    case Reg::R13W:  value.u16 = static_cast<uint16_t>(m_regs._r13);    break;
    case Reg::R14W:  value.u16 = static_cast<uint16_t>(m_regs._r14);    break;
    case Reg::R15W:  value.u16 = static_cast<uint16_t>(m_regs._r15);    break;
    case Reg::IP:    value.u16 = static_cast<uint16_t>(m_regs._eip);    break;
    case Reg::FLAGS: value.u16 = static_cast<uint16_t>(m_regs._eflags); break;

    case Reg::EAX:    value.u32 =                       m_regs._eax;     break;
    case Reg::ECX:    value.u32 =                       m_regs._ecx;     break;
    case Reg::EDX:    value.u32 =                       m_regs._edx;     break;
    case Reg::EBX:    value.u32 =                       m_regs._ebx;     break;
    case Reg::ESI:    value.u32 =                       m_regs._esi;     break;
    case Reg::EDI:    value.u32 =                       m_regs._edi;     break;
    case Reg::ESP:    value.u32 =                       m_regs._esp;     break;
    case Reg::EBP:    value.u32 =                       m_regs._ebp;     break;
    case Reg::R8D:    value.u32 = static_cast<uint32_t>(m_regs._r8);     break;
    case Reg::R9D:    value.u32 = static_cast<uint32_t>(m_regs._r9);     break;
    case Reg::R10D:   value.u32 = static_cast<uint32_t>(m_regs._r10);    break;
    case Reg::R11D:   value.u32 = static_cast<uint32_t>(m_regs._r11);    break;
    case Reg::R12D:   value.u32 = static_cast<uint32_t>(m_regs._r12);    break;
    case Reg::R13D:   value.u32 = static_cast<uint32_t>(m_regs._r13);    break;
    case Reg::R14D:   value.u32 = static_cast<uint32_t>(m_regs._r14);    break;
    case Reg::R15D:   value.u32 = static_cast<uint32_t>(m_regs._r15);    break;
    case Reg::EIP:    value.u32 = m_regs._eip;    break;
    case Reg::EFLAGS: value.u32 = m_regs._eflags; break;

    case Reg::RAX:    value.u64 = m_regs._rax;    break;
    case Reg::RCX:    value.u64 = m_regs._rcx;    break;
    case Reg::RDX:    value.u64 = m_regs._rdx;    break;
    case Reg::RBX:    value.u64 = m_regs._rbx;    break;
    case Reg::RSI:    value.u64 = m_regs._rsi;    break;
    case Reg::RDI:    value.u64 = m_regs._rdi;    break;
    case Reg::RSP:    value.u64 = m_regs._rsp;    break;
    case Reg::RBP:    value.u64 = m_regs._rbp;    break;
    case Reg::R8:     value.u64 = m_regs._r8;     break;
    case Reg::R9:     value.u64 = m_regs._r9;     break;
    case Reg::R10:    value.u64 = m_regs._r10;    break;
    case Reg::R11:    value.u64 = m_regs._r11;    break;
    case Reg::R12:    value.u64 = m_regs._r12;    break;
    case Reg::R13:    value.u64 = m_regs._r13;    break;
    case Reg::R14:    value.u64 = m_regs._r14;    break;
    case Reg::R15:    value.u64 = m_regs._r15;    break;
    case Reg::RIP:    value.u64 = m_regs._rip;    break;
    case Reg::RFLAGS: value.u64 = m_regs._rflags; break;
    case Reg::CR0:    value.u64 = m_regs._cr0;    break;
    case Reg::CR2:    value.u64 = m_regs._cr2;    break;
    case Reg::CR3:    value.u64 = m_regs._cr3;    break;
    case Reg::CR4:    value.u64 = m_regs._cr4;    break;
    case Reg::DR0:    value.u64 = m_regs._dr0;    break;
    case Reg::DR1:    value.u64 = m_regs._dr1;    break;
    case Reg::DR2:    value.u64 = m_regs._dr2;    break;
    case Reg::DR3:    value.u64 = m_regs._dr3;    break;
    case Reg::DR6:    value.u64 = m_regs._dr6;    break;
    case Reg::DR7:    value.u64 = m_regs._dr7;    break;
    case Reg::EFER:
        if (m_useEferMSR) {
            GetMSR(0xC0000080, value.u64);
        }
        else {
            value.u64 = m_regs._efer;
        }
        break;


    case Reg::ST0: case Reg::ST1: case Reg::ST2: case Reg::ST3:
    case Reg::ST4: case Reg::ST5: case Reg::ST6: case Reg::ST7:
        LoadSTRegister(value, RegOffset<uint8_t>(Reg::ST0, reg), m_fpuRegs);
        break;

    case Reg::MM0: case Reg::MM1: case Reg::MM2: case Reg::MM3:
    case Reg::MM4: case Reg::MM5: case Reg::MM6: case Reg::MM7:
        LoadMMRegister(value, RegOffset<uint8_t>(Reg::MM0, reg), m_fpuRegs);
        break;

    case Reg::XMM0: case Reg::XMM1: case Reg::XMM2: case Reg::XMM3:
    case Reg::XMM4: case Reg::XMM5: case Reg::XMM6: case Reg::XMM7:
    case Reg::XMM8: case Reg::XMM9: case Reg::XMM10: case Reg::XMM11:
    case Reg::XMM12: case Reg::XMM13: case Reg::XMM14: case Reg::XMM15:
        LoadXMMRegister(value, RegOffset<uint8_t>(Reg::XMM0, reg), m_fpuRegs);
        break;

    default: return VPOperationStatus::Unsupported;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::HaxmRegWrite(const Reg reg, const RegValue& value) noexcept {
    switch (reg) {
    case Reg::CS:   StoreSegment(value, &m_regs._cs);          m_regsChanged = true;  break;
    case Reg::SS:   StoreSegment(value, &m_regs._ss);          m_regsChanged = true;  break;
    case Reg::DS:   StoreSegment(value, &m_regs._ds);          m_regsChanged = true;  break;
    case Reg::ES:   StoreSegment(value, &m_regs._es);          m_regsChanged = true;  break;
    case Reg::FS:   StoreSegment(value, &m_regs._fs);          m_regsChanged = true;  break;
    case Reg::GS:   StoreSegment(value, &m_regs._gs);          m_regsChanged = true;  break;
    case Reg::LDTR: StoreSegment(value, &m_regs._ldt);         m_regsChanged = true;  break;
    case Reg::TR:   StoreSegment(value, &m_regs._tr);          m_regsChanged = true;  break;

    case Reg::GDTR: StoreTable(value, &m_regs._gdt);           m_regsChanged = true;  break;
    case Reg::IDTR: StoreTable(value, &m_regs._idt);           m_regsChanged = true;  break;

    case Reg::AH:   m_regs._ah = value.u8;                     m_regsChanged = true;  break;
    case Reg::AL:   m_regs._al = value.u8;                     m_regsChanged = true;  break;
    case Reg::CH:   m_regs._ch = value.u8;                     m_regsChanged = true;  break;
    case Reg::CL:   m_regs._cl = value.u8;                     m_regsChanged = true;  break;
    case Reg::DH:   m_regs._dh = value.u8;                     m_regsChanged = true;  break;
    case Reg::DL:   m_regs._dl = value.u8;                     m_regsChanged = true;  break;
    case Reg::BH:   m_regs._bh = value.u8;                     m_regsChanged = true;  break;
    case Reg::BL:   m_regs._bl = value.u8;                     m_regsChanged = true;  break;
    case Reg::SPL:  m_regs._sp = value.u8;                     m_regsChanged = true;  break;
    case Reg::BPL:  m_regs._bp = value.u8;                     m_regsChanged = true;  break;
    case Reg::SIL:  m_regs._si = value.u8;                     m_regsChanged = true;  break;
    case Reg::DIL:  m_regs._di = value.u8;                     m_regsChanged = true;  break;
    case Reg::R8B:  m_regs._r8 = SetLowByte(m_regs._r8, value.u8);   m_regsChanged = true;  break;
    case Reg::R9B:  m_regs._r9 = SetLowByte(m_regs._r9, value.u8);   m_regsChanged = true;  break;
    case Reg::R10B: m_regs._r10 = SetLowByte(m_regs._r10, value.u8); m_regsChanged = true;  break;
    case Reg::R11B: m_regs._r11 = SetLowByte(m_regs._r11, value.u8); m_regsChanged = true;  break;
    case Reg::R12B: m_regs._r12 = SetLowByte(m_regs._r12, value.u8); m_regsChanged = true;  break;
    case Reg::R13B: m_regs._r13 = SetLowByte(m_regs._r13, value.u8); m_regsChanged = true;  break;
    case Reg::R14B: m_regs._r14 = SetLowByte(m_regs._r14, value.u8); m_regsChanged = true;  break;
    case Reg::R15B: m_regs._r15 = SetLowByte(m_regs._r15, value.u8); m_regsChanged = true;  break;

    case Reg::AX:    m_regs._ax = value.u16;                   m_regsChanged = true;  break;
    case Reg::CX:    m_regs._cx = value.u16;                   m_regsChanged = true;  break;
    case Reg::DX:    m_regs._dx = value.u16;                   m_regsChanged = true;  break;
    case Reg::BX:    m_regs._bx = value.u16;                   m_regsChanged = true;  break;
    case Reg::SI:    m_regs._si = value.u16;                   m_regsChanged = true;  break;
    case Reg::DI:    m_regs._di = value.u16;                   m_regsChanged = true;  break;
    case Reg::SP:    m_regs._sp = value.u16;                   m_regsChanged = true;  break;
    case Reg::BP:    m_regs._bp = value.u16;                   m_regsChanged = true;  break;
    case Reg::R8W:   m_regs._r8 = SetLowWord(m_regs._r8, value.u16);    m_regsChanged = true;  break;
    case Reg::R9W:   m_regs._r9 = SetLowWord(m_regs._r9, value.u16);    m_regsChanged = true;  break;
    case Reg::R10W:  m_regs._r10 = SetLowWord(m_regs._r10, value.u16);  m_regsChanged = true;  break;
    case Reg::R11W:  m_regs._r11 = SetLowWord(m_regs._r11, value.u16);  m_regsChanged = true;  break;
    case Reg::R12W:  m_regs._r12 = SetLowWord(m_regs._r12, value.u16);  m_regsChanged = true;  break;
    case Reg::R13W:  m_regs._r13 = SetLowWord(m_regs._r13, value.u16);  m_regsChanged = true;  break;
    case Reg::R14W:  m_regs._r14 = SetLowWord(m_regs._r14, value.u16);  m_regsChanged = true;  break;
    case Reg::R15W:  m_regs._r15 = SetLowWord(m_regs._r15, value.u16);  m_regsChanged = true;  break;
    case Reg::IP:    m_regs._eip = value.u16;                  m_regsChanged = true;  break;
    case Reg::FLAGS: m_regs._eflags = static_cast<uint32_t>(fixupFlags(value.u16)); m_regsChanged = true;  break;

    case Reg::EAX:    m_regs._rax = value.u32;                 m_regsChanged = true;  break;
    case Reg::ECX:    m_regs._rcx = value.u32;                 m_regsChanged = true;  break;
    case Reg::EDX:    m_regs._rdx = value.u32;                 m_regsChanged = true;  break;
    case Reg::EBX:    m_regs._rbx = value.u32;                 m_regsChanged = true;  break;
    case Reg::ESI:    m_regs._rsi = value.u32;                 m_regsChanged = true;  break;
    case Reg::EDI:    m_regs._rdi = value.u32;                 m_regsChanged = true;  break;
    case Reg::ESP:    m_regs._rsp = value.u32;                 m_regsChanged = true;  break;
    case Reg::EBP:    m_regs._rbp = value.u32;                 m_regsChanged = true;  break;
    case Reg::R8D:    m_regs._r8 = value.u32;                  m_regsChanged = true;  break;
    case Reg::R9D:    m_regs._r9 = value.u32;                  m_regsChanged = true;  break;
    case Reg::R10D:   m_regs._r10 = value.u32;                 m_regsChanged = true;  break;
    case Reg::R11D:   m_regs._r11 = value.u32;                 m_regsChanged = true;  break;
    case Reg::R12D:   m_regs._r12 = value.u32;                 m_regsChanged = true;  break;
    case Reg::R13D:   m_regs._r13 = value.u32;                 m_regsChanged = true;  break;
    case Reg::R14D:   m_regs._r14 = value.u32;                 m_regsChanged = true;  break;
    case Reg::R15D:   m_regs._r15 = value.u32;                 m_regsChanged = true;  break;
    case Reg::EIP:    m_regs._eip = value.u32;                 m_regsChanged = true;  break;
    case Reg::EFLAGS: m_regs._eflags = static_cast<uint32_t>(fixupFlags(value.u32)); m_regsChanged = true;  break;
    case Reg::EFER:
        if (m_useEferMSR) {
            SetMSR(0xC0000080, value.u64);
        }
        else {
            m_regs._efer = value.u32;
            m_regsChanged = true;
        }
        break;

    case Reg::RAX:    m_regs._rax = value.u64;                 m_regsChanged = true;  break;
    case Reg::RCX:    m_regs._rcx = value.u64;                 m_regsChanged = true;  break;
    case Reg::RDX:    m_regs._rdx = value.u64;                 m_regsChanged = true;  break;
    case Reg::RBX:    m_regs._rbx = value.u64;                 m_regsChanged = true;  break;
    case Reg::RSI:    m_regs._rsi = value.u64;                 m_regsChanged = true;  break;
    case Reg::RDI:    m_regs._rdi = value.u64;                 m_regsChanged = true;  break;
    case Reg::RSP:    m_regs._rsp = value.u64;                 m_regsChanged = true;  break;
    case Reg::RBP:    m_regs._rbp = value.u64;                 m_regsChanged = true;  break;
    case Reg::R8:     m_regs._r8 = value.u64;                  m_regsChanged = true;  break;
    case Reg::R9:     m_regs._r9 = value.u64;                  m_regsChanged = true;  break;
    case Reg::R10:    m_regs._r10 = value.u64;                 m_regsChanged = true;  break;
    case Reg::R11:    m_regs._r11 = value.u64;                 m_regsChanged = true;  break;
    case Reg::R12:    m_regs._r12 = value.u64;                 m_regsChanged = true;  break;
    case Reg::R13:    m_regs._r13 = value.u64;                 m_regsChanged = true;  break;
    case Reg::R14:    m_regs._r14 = value.u64;                 m_regsChanged = true;  break;
    case Reg::R15:    m_regs._r15 = value.u64;                 m_regsChanged = true;  break;
    case Reg::RIP:    m_regs._rip = value.u64;                 m_regsChanged = true;  break;
    case Reg::RFLAGS: m_regs._rflags = fixupFlags(value.u64);  m_regsChanged = true;  break;
    case Reg::CR0:    m_regs._cr0 = value.u64;                 m_regsChanged = true;  break;
    case Reg::CR2:    m_regs._cr2 = value.u64;                 m_regsChanged = true;  break;
    case Reg::CR3:    m_regs._cr3 = value.u64;                 m_regsChanged = true;  break;
    case Reg::CR4:    m_regs._cr4 = value.u64;                 m_regsChanged = true;  break;
    case Reg::DR0:    m_regs._dr0 = value.u64;                 m_regsChanged = true;  break;
    case Reg::DR1:    m_regs._dr1 = value.u64;                 m_regsChanged = true;  break;
    case Reg::DR2:    m_regs._dr2 = value.u64;                 m_regsChanged = true;  break;
    case Reg::DR3:    m_regs._dr3 = value.u64;                 m_regsChanged = true;  break;
    case Reg::DR6:    m_regs._dr6 = value.u64;                 m_regsChanged = true;  break;
    case Reg::DR7:    m_regs._dr7 = value.u64;                 m_regsChanged = true;  break;

    case Reg::ST0: case Reg::ST1: case Reg::ST2: case Reg::ST3:
    case Reg::ST4: case Reg::ST5: case Reg::ST6: case Reg::ST7:
        StoreSTRegister(value, RegOffset<uint8_t>(Reg::ST0, reg), m_fpuRegs);
        m_fpuRegsChanged = true;
        break;

    case Reg::MM0: case Reg::MM1: case Reg::MM2: case Reg::MM3:
    case Reg::MM4: case Reg::MM5: case Reg::MM6: case Reg::MM7:
        StoreMMRegister(value, RegOffset<uint8_t>(Reg::MM0, reg), m_fpuRegs);
        m_fpuRegsChanged = true;
        break;

    case Reg::XMM0: case Reg::XMM1: case Reg::XMM2: case Reg::XMM3:
    case Reg::XMM4: case Reg::XMM5: case Reg::XMM6: case Reg::XMM7:
    case Reg::XMM8: case Reg::XMM9: case Reg::XMM10: case Reg::XMM11:
    case Reg::XMM12: case Reg::XMM13: case Reg::XMM14: case Reg::XMM15:
        StoreXMMRegister(value, RegOffset<uint8_t>(Reg::XMM0, reg), m_fpuRegs);
        m_fpuRegsChanged = true;
        break;

    default: return VPOperationStatus::Unsupported;
    }

    return VPOperationStatus::OK;
}

// ----- Floating point control registers -------------------------------------

VPOperationStatus HaxmVirtualProcessor::GetFPUControl(FPUControl& value) noexcept {
    REFRESH_REGISTERS;

    value.cw = m_fpuRegs.fcw;
    value.sw = m_fpuRegs.fsw;
    value.tw = m_fpuRegs.ftw;
    value.op = m_fpuRegs.fop;
    value.cs = m_fpuRegs.fcs;
    value.rip = m_fpuRegs.fip;
    value.ds = m_fpuRegs.fds;
    value.rdp = m_fpuRegs.fdp;

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::SetFPUControl(const FPUControl& value) noexcept {
    REFRESH_REGISTERS;

    m_fpuRegs.fcw = value.cw;
    m_fpuRegs.fsw = value.sw;
    m_fpuRegs.ftw = static_cast<uint8_t>(value.tw);
    m_fpuRegs.fop = value.op;
    m_fpuRegs.fcs = value.cs;
    m_fpuRegs.fip = value.ip;
    m_fpuRegs.fds = value.ds;
    m_fpuRegs.fdp = value.dp;

    m_fpuRegsChanged = true;

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::GetMXCSR(MXCSR& value) noexcept {
    REFRESH_REGISTERS;
    value.u32 = m_fpuRegs.mxcsr;
    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::SetMXCSR(const MXCSR& value) noexcept {
    REFRESH_REGISTERS;
    m_fpuRegs.mxcsr = value.u32;
    m_fpuRegsChanged = true;
    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::GetMXCSRMask(MXCSR& value) noexcept {
    REFRESH_REGISTERS;
    value.u32 = m_fpuRegs.mxcsr_mask;
    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::SetMXCSRMask(const MXCSR& value) noexcept {
    REFRESH_REGISTERS;
    m_fpuRegs.mxcsr_mask = value.u32;
    m_fpuRegsChanged = true;
    return VPOperationStatus::OK;
}
// ----- Model specific registers ---------------------------------------------

VPOperationStatus HaxmVirtualProcessor::GetMSR(const uint64_t msr, uint64_t& value) noexcept {
    hax_msr_data msrData = { 0 };
    msrData.entries[0].entry = msr;
    msrData.nr_msr = 1;

    if (!m_sys->GetMSRData(&msrData)) {
        return VPOperationStatus::Failed;
    }

    value = msrData.entries[0].value;
    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::SetMSR(const uint64_t msr, const uint64_t value) noexcept {
    hax_msr_data msrData = { 0 };
    msrData.entries[0].entry = msr;
    msrData.entries[0].value = value;
    msrData.nr_msr = 1;

    if (!m_sys->SetMSRData(&msrData)) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) noexcept {
    if (msrs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    size_t index = 0;
    while (index < numRegs) {
        hax_msr_data msrData = { 0 };
        size_t msrCount = 0;
        const size_t prevIndex = index;
        for (size_t i = 0; i < numRegs && i < HAX_MAX_MSR_ARRAY; i++) {
            msrData.entries[i].entry = msrs[index];
            index++;
            msrCount++;
        }
        msrData.nr_msr = static_cast<uint16_t>(msrCount);

        if (!m_sys->GetMSRData(&msrData)) {
            return VPOperationStatus::Failed;
        }

        index = prevIndex;
        for (size_t i = 0; i < numRegs && i < HAX_MAX_MSR_ARRAY; i++) {
            values[index] = msrData.entries[i].value;
            index++;
            msrCount++;
        }
    }

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) noexcept {
    if (msrs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    size_t index = 0;
    while (index < numRegs) {
        hax_msr_data msrData = { 0 };
        size_t msrCount = 0;
        for (size_t i = 0; i < numRegs && i < HAX_MAX_MSR_ARRAY; i++) {
            msrData.entries[i].entry = msrs[index];
            msrData.entries[i].value = values[index];
            index++;
            msrCount++;
        }
        msrData.nr_msr = static_cast<uint16_t>(msrCount);

        if (!m_sys->SetMSRData(&msrData)) {
            return VPOperationStatus::Failed;
        }
    }

    return VPOperationStatus::OK;
}

// ----- Breakpoints ----------------------------------------------------------

VPOperationStatus HaxmVirtualProcessor::EnableSoftwareBreakpoints(bool enable) noexcept {
    if (enable) {
        m_debug.control |= HAX_DEBUG_USE_SW_BP;
    }
    else {
        m_debug.control &= ~HAX_DEBUG_USE_SW_BP;
    }

    if (!SetDebug()) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::SetHardwareBreakpoints(HardwareBreakpoints breakpoints) noexcept {
    bool anyEnabled = false;
    for (int i = 0; i < 4; i++) {
        if (breakpoints.bp[i].localEnable || breakpoints.bp[i].globalEnable) {
            anyEnabled = true;
            break;
        }
    }
    if (!anyEnabled) {
        return ClearHardwareBreakpoints();
    }

    m_debug.control |= HAX_DEBUG_USE_HW_BP;
    for (int i = 0; i < 4; i++) {
        m_debug.dr[i] = breakpoints.bp[i].address;
        m_debug.dr[7] |= static_cast<uint64_t>(breakpoints.bp[i].localEnable ? 1 : 0) << (0 + i * 2);
        m_debug.dr[7] |= static_cast<uint64_t>(breakpoints.bp[i].globalEnable ? 1 : 0) << (1 + i * 2);
        m_debug.dr[7] |= static_cast<uint64_t>(static_cast<uint8_t>(breakpoints.bp[i].trigger) & 0x3) << (16 + i * 4);
        m_debug.dr[7] |= static_cast<uint64_t>(static_cast<uint8_t>(breakpoints.bp[i].length) & 0x3) << (18 + i * 4);
    }

    if (!SetDebug()) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::ClearHardwareBreakpoints() noexcept {
    m_debug.control &= ~HAX_DEBUG_USE_HW_BP;
    memset(m_debug.dr, 0, 8 * sizeof(uint64_t));
    if (!SetDebug()) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus HaxmVirtualProcessor::GetBreakpointAddress(uint64_t *address) const noexcept {
    if (address == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    auto debugTunnel = &m_tunnel->debug;
    const uint8_t hardwareBP = static_cast<uint8_t>(debugTunnel->dr6 & 0xF);

    // No breakpoints were hit
    if (hardwareBP == 0 && debugTunnel->rip == 0) {
        return VPOperationStatus::BreakpointNeverHit;
    }

    // Get the appropriate breakpoint address
    /**/ if (hardwareBP & (1 << 0)) *address = m_debug.dr[0];
    else if (hardwareBP & (1 << 1)) *address = m_debug.dr[1];
    else if (hardwareBP & (1 << 2)) *address = m_debug.dr[2];
    else if (hardwareBP & (1 << 3)) *address = m_debug.dr[3];
    else /************************/ *address = debugTunnel->rip;

    return VPOperationStatus::OK;
}

#undef REFRESH_REGISTERS
#undef REFRESH_FP_REGISTERS

}
