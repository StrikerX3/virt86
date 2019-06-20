/*
Implementation of the KVM VirtualProcessor class.
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
#include "kvm_vp.hpp"
#include "kvm_vm.hpp"

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <malloc.h>
#include <linux/kvm.h>
#include <assert.h>
#include <memory>

#include "virt86/util/bytemanip.hpp"

namespace virt86::kvm {

KvmVirtualProcessor::KvmVirtualProcessor(KvmVirtualMachine& vm, uint32_t vcpuID)
    : VirtualProcessor(vm)
    , m_vm(vm)
    , m_vcpuID(vcpuID)
    , m_fd(-1)
    , m_regsDirty(false)
    , m_regsChanged(false)
    , m_sregsChanged(false)
    , m_debugRegsChanged(false)
    , m_fpuRegsChanged(false)
    , m_regs({ 0 })
    , m_sregs({ 0 })
    , m_fpuRegs({ 0 })
    , m_kvmRun(nullptr)
    , m_kvmRunMmapSize(0)
    , m_debug({ 0 })
{
}

KvmVirtualProcessor::~KvmVirtualProcessor() noexcept {
    if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }
}

bool KvmVirtualProcessor::Initialize() noexcept {
    // Create the VCPU
    m_fd = ioctl(m_vm.FileDescriptor(), KVM_CREATE_VCPU, m_vcpuID);
    if (m_fd < 0) {
        return false;
    }

    // Get the kvmRun struct size
    m_kvmRunMmapSize = ioctl(m_vm.KVMFileDescriptor(), KVM_GET_VCPU_MMAP_SIZE, 0);
    if (m_kvmRunMmapSize < 0) {
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // mmap kvmRun to the VCPU file
    m_kvmRun = (struct kvm_run*)mmap(nullptr, (size_t)m_kvmRunMmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if (m_kvmRun < 0) {
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // Configure the custom CPUIDs if supported
    if (m_vm.GetPlatform().GetFeatures().customCPUIDs) {
        auto& cpuids = m_vm.GetSpecifications().CPUIDResults;
        if (!cpuids.empty()) {
            size_t count = cpuids.size();
            auto cpuid = allocVarEntry<kvm_cpuid2, kvm_cpuid_entry2>(count);
            cpuid->nent = static_cast<__u32>(count);
            for (size_t i = 0; i < count; i++) {
                auto &entry = cpuids[i];
                cpuid->entries[i].function = entry.function;
                cpuid->entries[i].index = static_cast<__u32>(i);
                cpuid->entries[i].flags = 0;
                cpuid->entries[i].eax = entry.eax;
                cpuid->entries[i].ebx = entry.ebx;
                cpuid->entries[i].ecx = entry.ecx;
                cpuid->entries[i].edx = entry.edx;
            }

            int result = ioctl(m_fd, KVM_SET_CPUID2, cpuid);
            if (result < 0) {
                close(m_fd);
                m_fd = -1;
                free(cpuid);
                return false;
            }
            free(cpuid);
        }
    }

    return true;

}

VPExecutionStatus KvmVirtualProcessor::RunImpl() noexcept {
    if (!UpdateRegisters()) {
        return VPExecutionStatus::Failed;
    }

    if (ioctl(m_fd, KVM_RUN, 0) < 0) {
        return VPExecutionStatus::Failed;
    }

    return HandleExecResult();
}

VPExecutionStatus KvmVirtualProcessor::StepImpl() noexcept {
    m_debug.control |= KVM_GUESTDBG_SINGLESTEP;
    if (!SetDebug()) {
        return VPExecutionStatus::Failed;
    }

    auto status = RunImpl();

    m_debug.control &= ~KVM_GUESTDBG_SINGLESTEP;
    if (!SetDebug()) {
        return VPExecutionStatus::Failed;
    }

    if (status != VPExecutionStatus::OK) {
        return status;
    }

    auto result = HandleExecResult();
    if (m_exitInfo.reason == VMExitReason::SoftwareBreakpoint) {
        m_exitInfo.reason = VMExitReason::Step;
    }
    return result;
}

bool KvmVirtualProcessor::SetDebug() noexcept {
    bool enable = (m_debug.control & ~KVM_GUESTDBG_ENABLE) != 0;
    if (enable) {
        m_debug.control |= KVM_GUESTDBG_ENABLE;
    }
    else {
        m_debug.control &= ~KVM_GUESTDBG_ENABLE;
    }

    return ioctl(m_fd, KVM_SET_GUEST_DEBUG, &m_debug) >= 0;
}

bool KvmVirtualProcessor::UpdateRegisters() noexcept {
    // Update CPU state if registers were modified
    if (m_regsChanged) {
        if (ioctl(m_fd, KVM_SET_REGS, &m_regs) < 0) {
            return false;
        }
        m_regsChanged = false;
    }
    if (m_sregsChanged) {
        if (ioctl(m_fd, KVM_SET_SREGS, &m_sregs) < 0) {
            return false;
        }
        m_sregsChanged = false;
    }
    if (m_debugRegsChanged) {
        if (ioctl(m_fd, KVM_SET_DEBUGREGS, &m_debugRegs) < 0) {
            return false;
        }
        m_debugRegsChanged = false;
    }
    if (m_fpuRegsChanged) {
        if (ioctl(m_fd, KVM_SET_FPU, &m_fpuRegs) < 0) {
            return false;
        }
        m_fpuRegsChanged = false;
    }
    return true;
}

bool KvmVirtualProcessor::RefreshRegisters() noexcept {
    // Retrieve registers from CPU state
    if (m_regsDirty) {
        if (ioctl(m_fd, KVM_GET_REGS, &m_regs) < 0) {
            return false;
        }
        if (ioctl(m_fd, KVM_GET_SREGS, &m_sregs) < 0) {
            return false;
        }
        if (ioctl(m_fd, KVM_GET_DEBUGREGS, &m_debugRegs) < 0) {
            return false;
        }
        if (ioctl(m_fd, KVM_GET_FPU, &m_fpuRegs) < 0) {
            return false;
        }
        m_regsDirty = false;
    }
    return true;
}

VPExecutionStatus KvmVirtualProcessor::HandleExecResult() noexcept {
    // Mark registers as dirty
    m_regsDirty = true;

    // Handle exit status using tunnel
    switch (m_kvmRun->exit_reason) {
        case KVM_EXIT_HLT:             m_exitInfo.reason = VMExitReason::HLT;       break;  // HLT instruction
        case KVM_EXIT_INTR:            m_exitInfo.reason = VMExitReason::Normal;    break;  // Let KVM handle this
        case KVM_EXIT_IRQ_WINDOW_OPEN: m_exitInfo.reason = VMExitReason::Interrupt; break;  // Interrupt window
        case KVM_EXIT_EXCEPTION:       HandleException();                           break;  // CPU exception raised
        case KVM_EXIT_SHUTDOWN:        m_exitInfo.reason = VMExitReason::Shutdown;  break;  // The VM is shutting down
        case KVM_EXIT_UNKNOWN:         m_exitInfo.reason = VMExitReason::Error;     break;  // VM exited for an unknown reason
        case KVM_EXIT_IO:              HandleIO();                                  break;  // I/O (in / out instructions)
        case KVM_EXIT_MMIO:            HandleMMIO();                                break;  // MMIO
        case KVM_EXIT_DEBUG:                                                                // A breakpoint was hit
        {
            // Determine if it was a software or hardware breakpoint
            if (m_kvmRun->debug.arch.dr6 & 0xf) {
                m_exitInfo.reason = VMExitReason::HardwareBreakpoint;
            }
            else {
                m_exitInfo.reason = VMExitReason::SoftwareBreakpoint;
            }
            break;
        }
        default: m_exitInfo.reason = VMExitReason::Unhandled; break;   // Unknown reason (possibly new reason from a newer KVM)
    }

    return VPExecutionStatus::OK;
}

void KvmVirtualProcessor::HandleException() noexcept {
    m_exitInfo.reason = VMExitReason::Exception;
    m_exitInfo.exceptionCode = m_kvmRun->ex.exception;
}

void KvmVirtualProcessor::HandleIO() noexcept {
    m_exitInfo.reason = VMExitReason::PIO;

    uint8_t *ptr = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(m_kvmRun) + m_kvmRun->io.data_offset);

    for (uint16_t i = 0; i < m_kvmRun->io.count; i++) {
        if (m_kvmRun->io.direction == KVM_EXIT_IO_OUT) {
            uint32_t value;
            switch (m_kvmRun->io.size) {
                case 1: value = *ptr; break;
                case 2: value = *reinterpret_cast<uint16_t *>(ptr); break;
                case 4: value = *reinterpret_cast<uint32_t *>(ptr); break;
                default: assert(0); // should not happen
            }
            m_io.IOWrite(m_kvmRun->io.port, m_kvmRun->io.size, value);
        }
        else {
            uint32_t value = m_io.IORead(m_kvmRun->io.port, m_kvmRun->io.size);
            switch (m_kvmRun->io.size) {
                case 1: *ptr = static_cast<uint8_t>(value); break;
                case 2: *reinterpret_cast<uint16_t *>(ptr) = static_cast<uint16_t>(value); break;
                case 4: *reinterpret_cast<uint32_t *>(ptr) = value; break;
                default: assert(0); // should not happen
            }
        }

        ptr += m_kvmRun->io.size;
    }
}

void KvmVirtualProcessor::HandleMMIO() noexcept {
    m_exitInfo.reason = VMExitReason::MMIO;

    if (m_kvmRun->mmio.is_write) {
        m_io.MMIOWrite(m_kvmRun->mmio.phys_addr, m_kvmRun->mmio.len, *reinterpret_cast<uint64_t*>(m_kvmRun->mmio.data));
    } else {
        uint64_t value = m_io.MMIORead(m_kvmRun->mmio.phys_addr, m_kvmRun->mmio.len);
        switch (m_kvmRun->mmio.len) {
            case 1: *reinterpret_cast<uint8_t *>(m_kvmRun->mmio.data) = static_cast<uint8_t>(value); break;
            case 2: *reinterpret_cast<uint16_t *>(m_kvmRun->mmio.data) = static_cast<uint16_t>(value); break;
            case 4: *reinterpret_cast<uint32_t *>(m_kvmRun->mmio.data) = static_cast<uint32_t>(value); break;
            case 8: *reinterpret_cast<uint64_t *>(m_kvmRun->mmio.data) = value; break;
            default: assert(0); // should not happen
        }
    }
}

bool KvmVirtualProcessor::PrepareInterrupt(uint8_t vector) noexcept {
    return true;
}

VPOperationStatus KvmVirtualProcessor::InjectInterrupt(uint8_t vector) noexcept {
    struct kvm_interrupt kvmInterrupt;
    kvmInterrupt.irq = (uint32_t)vector;
    if (ioctl(m_fd, KVM_INTERRUPT, &kvmInterrupt) < 0) {
        return VPOperationStatus::Failed;
    }
    m_kvmRun->request_interrupt_window = 0;
    return VPOperationStatus::OK;
}

bool KvmVirtualProcessor::CanInjectInterrupt() const noexcept {
    return m_kvmRun->ready_for_interrupt_injection != 0;
}

void KvmVirtualProcessor::RequestInterruptWindow() noexcept {
    m_kvmRun->request_interrupt_window = 1;
}

// ----- Registers ------------------------------------------------------------

#define REFRESH_REGISTERS do { \
    const auto status = RefreshRegisters(); \
    if (!status) { \
        return VPOperationStatus::Failed; \
    } \
} while (0)

VPOperationStatus KvmVirtualProcessor::RegRead(const Reg reg, RegValue& value) noexcept {
    REFRESH_REGISTERS;
    return KvmRegRead(reg, value);
}

VPOperationStatus KvmVirtualProcessor::RegWrite(const Reg reg, const RegValue& value) noexcept {
    REFRESH_REGISTERS;
    return KvmRegWrite(reg, value);
}

VPOperationStatus KvmVirtualProcessor::RegRead(const Reg regs[], RegValue values[], const size_t numRegs) noexcept {
    REFRESH_REGISTERS;

    for (size_t i = 0; i < numRegs; i++) {
        auto result = KvmRegRead(regs[i], values[i]);
        if (result != VPOperationStatus::OK) {
            return result;
        }
    }

    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) noexcept {
    REFRESH_REGISTERS;

    for (size_t i = 0; i < numRegs; i++) {
        auto result = KvmRegWrite(regs[i], values[i]);
        if (result != VPOperationStatus::OK) {
            return result;
        }
    }

    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::KvmRegRead(const Reg reg, RegValue& value) noexcept {
    switch (reg) {
        case Reg::CS:   LoadSegment(value, &m_sregs.cs);  break;
        case Reg::SS:   LoadSegment(value, &m_sregs.ss);  break;
        case Reg::DS:   LoadSegment(value, &m_sregs.ds);  break;
        case Reg::ES:   LoadSegment(value, &m_sregs.es);  break;
        case Reg::FS:   LoadSegment(value, &m_sregs.fs);  break;
        case Reg::GS:   LoadSegment(value, &m_sregs.gs);  break;
        case Reg::LDTR: LoadSegment(value, &m_sregs.ldt); break;
        case Reg::TR:   LoadSegment(value, &m_sregs.tr);  break;

        case Reg::GDTR: LoadTable(value, &m_sregs.gdt);  break;
        case Reg::IDTR: LoadTable(value, &m_sregs.idt);  break;

        case Reg::AH:   value.u8 = static_cast<uint8_t>(m_regs.rax >> 8ull); break;
        case Reg::AL:   value.u8 = static_cast<uint8_t>(m_regs.rax);         break;
        case Reg::CH:   value.u8 = static_cast<uint8_t>(m_regs.rcx >> 8ull); break;
        case Reg::CL:   value.u8 = static_cast<uint8_t>(m_regs.rcx);         break;
        case Reg::DH:   value.u8 = static_cast<uint8_t>(m_regs.rdx >> 8ull); break;
        case Reg::DL:   value.u8 = static_cast<uint8_t>(m_regs.rdx);         break;
        case Reg::BH:   value.u8 = static_cast<uint8_t>(m_regs.rbx >> 8ull); break;
        case Reg::BL:   value.u8 = static_cast<uint8_t>(m_regs.rbx);         break;
        case Reg::SPL:  value.u8 = static_cast<uint8_t>(m_regs.rsp);         break;
        case Reg::BPL:  value.u8 = static_cast<uint8_t>(m_regs.rbp);         break;
        case Reg::SIL:  value.u8 = static_cast<uint8_t>(m_regs.rsi);         break;
        case Reg::DIL:  value.u8 = static_cast<uint8_t>(m_regs.rdi);         break;
        case Reg::R8B:  value.u8 = static_cast<uint8_t>(m_regs.r8);          break;
        case Reg::R9B:  value.u8 = static_cast<uint8_t>(m_regs.r9);          break;
        case Reg::R10B: value.u8 = static_cast<uint8_t>(m_regs.r10);         break;
        case Reg::R11B: value.u8 = static_cast<uint8_t>(m_regs.r11);         break;
        case Reg::R12B: value.u8 = static_cast<uint8_t>(m_regs.r12);         break;
        case Reg::R13B: value.u8 = static_cast<uint8_t>(m_regs.r13);         break;
        case Reg::R14B: value.u8 = static_cast<uint8_t>(m_regs.r14);         break;
        case Reg::R15B: value.u8 = static_cast<uint8_t>(m_regs.r15);         break;

        case Reg::AX:    value.u16 = static_cast<uint16_t>(m_regs.rax);    break;
        case Reg::CX:    value.u16 = static_cast<uint16_t>(m_regs.rcx);    break;
        case Reg::DX:    value.u16 = static_cast<uint16_t>(m_regs.rdx);    break;
        case Reg::BX:    value.u16 = static_cast<uint16_t>(m_regs.rbx);    break;
        case Reg::SI:    value.u16 = static_cast<uint16_t>(m_regs.rsi);    break;
        case Reg::DI:    value.u16 = static_cast<uint16_t>(m_regs.rdi);    break;
        case Reg::SP:    value.u16 = static_cast<uint16_t>(m_regs.rsp);    break;
        case Reg::BP:    value.u16 = static_cast<uint16_t>(m_regs.rbp);    break;
        case Reg::R8W:   value.u16 = static_cast<uint16_t>(m_regs.r8);     break;
        case Reg::R9W:   value.u16 = static_cast<uint16_t>(m_regs.r9);     break;
        case Reg::R10W:  value.u16 = static_cast<uint16_t>(m_regs.r10);    break;
        case Reg::R11W:  value.u16 = static_cast<uint16_t>(m_regs.r11);    break;
        case Reg::R12W:  value.u16 = static_cast<uint16_t>(m_regs.r12);    break;
        case Reg::R13W:  value.u16 = static_cast<uint16_t>(m_regs.r13);    break;
        case Reg::R14W:  value.u16 = static_cast<uint16_t>(m_regs.r14);    break;
        case Reg::R15W:  value.u16 = static_cast<uint16_t>(m_regs.r15);    break;
        case Reg::IP:    value.u16 = static_cast<uint16_t>(m_regs.rip);    break;
        case Reg::FLAGS: value.u16 = static_cast<uint16_t>(m_regs.rflags); break;

        case Reg::EAX:    value.u32 = static_cast<uint32_t>(m_regs.rax);    break;
        case Reg::ECX:    value.u32 = static_cast<uint32_t>(m_regs.rcx);    break;
        case Reg::EDX:    value.u32 = static_cast<uint32_t>(m_regs.rdx);    break;
        case Reg::EBX:    value.u32 = static_cast<uint32_t>(m_regs.rbx);    break;
        case Reg::ESI:    value.u32 = static_cast<uint32_t>(m_regs.rsi);    break;
        case Reg::EDI:    value.u32 = static_cast<uint32_t>(m_regs.rdi);    break;
        case Reg::ESP:    value.u32 = static_cast<uint32_t>(m_regs.rsp);    break;
        case Reg::EBP:    value.u32 = static_cast<uint32_t>(m_regs.rbp);    break;
        case Reg::R8D:    value.u32 = static_cast<uint32_t>(m_regs.r8);     break;
        case Reg::R9D:    value.u32 = static_cast<uint32_t>(m_regs.r9);     break;
        case Reg::R10D:   value.u32 = static_cast<uint32_t>(m_regs.r10);    break;
        case Reg::R11D:   value.u32 = static_cast<uint32_t>(m_regs.r11);    break;
        case Reg::R12D:   value.u32 = static_cast<uint32_t>(m_regs.r12);    break;
        case Reg::R13D:   value.u32 = static_cast<uint32_t>(m_regs.r13);    break;
        case Reg::R14D:   value.u32 = static_cast<uint32_t>(m_regs.r14);    break;
        case Reg::R15D:   value.u32 = static_cast<uint32_t>(m_regs.r15);    break;
        case Reg::EIP:    value.u32 = static_cast<uint32_t>(m_regs.rip);    break;
        case Reg::EFLAGS: value.u32 = static_cast<uint32_t>(m_regs.rflags); break;

        case Reg::RAX:    value.u64 = m_regs.rax;         break;
        case Reg::RCX:    value.u64 = m_regs.rcx;         break;
        case Reg::RDX:    value.u64 = m_regs.rdx;         break;
        case Reg::RBX:    value.u64 = m_regs.rbx;         break;
        case Reg::RSI:    value.u64 = m_regs.rsi;         break;
        case Reg::RDI:    value.u64 = m_regs.rdi;         break;
        case Reg::RSP:    value.u64 = m_regs.rsp;         break;
        case Reg::RBP:    value.u64 = m_regs.rbp;         break;
        case Reg::R8:     value.u64 = m_regs.r8;          break;
        case Reg::R9:     value.u64 = m_regs.r9;          break;
        case Reg::R10:    value.u64 = m_regs.r10;         break;
        case Reg::R11:    value.u64 = m_regs.r11;         break;
        case Reg::R12:    value.u64 = m_regs.r12;         break;
        case Reg::R13:    value.u64 = m_regs.r13;         break;
        case Reg::R14:    value.u64 = m_regs.r14;         break;
        case Reg::R15:    value.u64 = m_regs.r15;         break;
        case Reg::RIP:    value.u64 = m_regs.rip;         break;
        case Reg::RFLAGS: value.u64 = m_regs.rflags;      break;
        case Reg::CR0:    value.u64 = m_sregs.cr0;        break;
        case Reg::CR2:    value.u64 = m_sregs.cr2;        break;
        case Reg::CR3:    value.u64 = m_sregs.cr3;        break;
        case Reg::CR4:    value.u64 = m_sregs.cr4;        break;
        case Reg::DR0:    value.u64 = m_debugRegs.db[0];  break;
        case Reg::DR1:    value.u64 = m_debugRegs.db[1];  break;
        case Reg::DR2:    value.u64 = m_debugRegs.db[2];  break;
        case Reg::DR3:    value.u64 = m_debugRegs.db[3];  break;
        case Reg::DR6:    value.u64 = m_debugRegs.dr6;    break;
        case Reg::DR7:    value.u64 = m_debugRegs.dr7;    break;
        case Reg::EFER:   value.u64 = m_sregs.efer;       break;

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

VPOperationStatus KvmVirtualProcessor::KvmRegWrite(const Reg reg, const RegValue& value) noexcept {
    switch (reg) {
        case Reg::CS:   StoreSegment(value, &m_sregs.cs);  m_sregsChanged = true;  break;
        case Reg::SS:   StoreSegment(value, &m_sregs.ss);  m_sregsChanged = true;  break;
        case Reg::DS:   StoreSegment(value, &m_sregs.ds);  m_sregsChanged = true;  break;
        case Reg::ES:   StoreSegment(value, &m_sregs.es);  m_sregsChanged = true;  break;
        case Reg::FS:   StoreSegment(value, &m_sregs.fs);  m_sregsChanged = true;  break;
        case Reg::GS:   StoreSegment(value, &m_sregs.gs);  m_sregsChanged = true;  break;
        case Reg::LDTR: StoreSegment(value, &m_sregs.ldt); m_sregsChanged = true;  break;
        case Reg::TR:   StoreSegment(value, &m_sregs.tr);  m_sregsChanged = true;  break;

        case Reg::GDTR: StoreTable(value, &m_sregs.gdt);  m_sregsChanged = true;  break;
        case Reg::IDTR: StoreTable(value, &m_sregs.idt);  m_sregsChanged = true;  break;

        case Reg::AH:   m_regs.rax = SetHighByte(m_regs.rax, value.u8);   m_regsChanged = true;  break;
        case Reg::AL:   m_regs.rax = SetLowByte(m_regs.rax, value.u8);  m_regsChanged = true;  break;
        case Reg::CH:   m_regs.rcx = SetHighByte(m_regs.rcx, value.u8);   m_regsChanged = true;  break;
        case Reg::CL:   m_regs.rcx = SetLowByte(m_regs.rcx, value.u8);  m_regsChanged = true;  break;
        case Reg::DH:   m_regs.rdx = SetHighByte(m_regs.rdx, value.u8);   m_regsChanged = true;  break;
        case Reg::DL:   m_regs.rdx = SetLowByte(m_regs.rdx, value.u8);  m_regsChanged = true;  break;
        case Reg::BH:   m_regs.rbx = SetHighByte(m_regs.rbx, value.u8);   m_regsChanged = true;  break;
        case Reg::BL:   m_regs.rbx = SetLowByte(m_regs.rbx, value.u8);  m_regsChanged = true;  break;
        case Reg::SPL:  m_regs.rsp = SetLowByte(m_regs.rsp, value.u8);   m_regsChanged = true;  break;
        case Reg::BPL:  m_regs.rbp = SetLowByte(m_regs.rbp, value.u8);   m_regsChanged = true;  break;
        case Reg::SIL:  m_regs.rsi = SetLowByte(m_regs.rsi, value.u8);   m_regsChanged = true;  break;
        case Reg::DIL:  m_regs.rdi = SetLowByte(m_regs.rdi, value.u8);   m_regsChanged = true;  break;
        case Reg::R8B:  m_regs.r8 = SetLowByte(m_regs.r8, value.u8);     m_regsChanged = true;  break;
        case Reg::R9B:  m_regs.r9 = SetLowByte(m_regs.r9, value.u8);     m_regsChanged = true;  break;
        case Reg::R10B: m_regs.r10 = SetLowByte(m_regs.r10, value.u8);   m_regsChanged = true;  break;
        case Reg::R11B: m_regs.r11 = SetLowByte(m_regs.r11, value.u8);   m_regsChanged = true;  break;
        case Reg::R12B: m_regs.r12 = SetLowByte(m_regs.r12, value.u8);   m_regsChanged = true;  break;
        case Reg::R13B: m_regs.r13 = SetLowByte(m_regs.r13, value.u8);   m_regsChanged = true;  break;
        case Reg::R14B: m_regs.r14 = SetLowByte(m_regs.r14, value.u8);   m_regsChanged = true;  break;
        case Reg::R15B: m_regs.r15 = SetLowByte(m_regs.r15, value.u8);   m_regsChanged = true;  break;

        case Reg::AX:    m_regs.rax = SetLowWord(m_regs.rax, value.u16);  m_regsChanged = true;  break;
        case Reg::CX:    m_regs.rcx = SetLowWord(m_regs.rcx, value.u16);  m_regsChanged = true;  break;
        case Reg::DX:    m_regs.rdx = SetLowWord(m_regs.rdx, value.u16);  m_regsChanged = true;  break;
        case Reg::BX:    m_regs.rbx = SetLowWord(m_regs.rbx, value.u16);  m_regsChanged = true;  break;
        case Reg::SI:    m_regs.rsi = SetLowWord(m_regs.rsi, value.u16);  m_regsChanged = true;  break;
        case Reg::DI:    m_regs.rdi = SetLowWord(m_regs.rdi, value.u16);  m_regsChanged = true;  break;
        case Reg::SP:    m_regs.rsp = SetLowWord(m_regs.rsp, value.u16);  m_regsChanged = true;  break;
        case Reg::BP:    m_regs.rbp = SetLowWord(m_regs.rbp, value.u16);  m_regsChanged = true;  break;
        case Reg::R8W:   m_regs.r8 = SetLowWord(m_regs.r8, value.u16);    m_regsChanged = true;  break;
        case Reg::R9W:   m_regs.r9 = SetLowWord(m_regs.r9, value.u16);    m_regsChanged = true;  break;
        case Reg::R10W:  m_regs.r10 = SetLowWord(m_regs.r10, value.u16);  m_regsChanged = true;  break;
        case Reg::R11W:  m_regs.r11 = SetLowWord(m_regs.r11, value.u16);  m_regsChanged = true;  break;
        case Reg::R12W:  m_regs.r12 = SetLowWord(m_regs.r12, value.u16);  m_regsChanged = true;  break;
        case Reg::R13W:  m_regs.r13 = SetLowWord(m_regs.r13, value.u16);  m_regsChanged = true;  break;
        case Reg::R14W:  m_regs.r14 = SetLowWord(m_regs.r14, value.u16);  m_regsChanged = true;  break;
        case Reg::R15W:  m_regs.r15 = SetLowWord(m_regs.r15, value.u16);  m_regsChanged = true;  break;
        case Reg::IP:    m_regs.rip = value.u16;                          m_regsChanged = true;  break;
        case Reg::FLAGS: m_regs.rflags = value.u16;                       m_regsChanged = true;  break;

        case Reg::EAX:    m_regs.rax = value.u32;     m_regsChanged = true;  break;
        case Reg::ECX:    m_regs.rcx = value.u32;     m_regsChanged = true;  break;
        case Reg::EDX:    m_regs.rdx = value.u32;     m_regsChanged = true;  break;
        case Reg::EBX:    m_regs.rbx = value.u32;     m_regsChanged = true;  break;
        case Reg::ESI:    m_regs.rsi = value.u32;     m_regsChanged = true;  break;
        case Reg::EDI:    m_regs.rdi = value.u32;     m_regsChanged = true;  break;
        case Reg::ESP:    m_regs.rsp = value.u32;     m_regsChanged = true;  break;
        case Reg::EBP:    m_regs.rbp = value.u32;     m_regsChanged = true;  break;
        case Reg::R8D:    m_regs.r8 = value.u32;      m_regsChanged = true;  break;
        case Reg::R9D:    m_regs.r9 = value.u32;      m_regsChanged = true;  break;
        case Reg::R10D:   m_regs.r10 = value.u32;     m_regsChanged = true;  break;
        case Reg::R11D:   m_regs.r11 = value.u32;     m_regsChanged = true;  break;
        case Reg::R12D:   m_regs.r12 = value.u32;     m_regsChanged = true;  break;
        case Reg::R13D:   m_regs.r13 = value.u32;     m_regsChanged = true;  break;
        case Reg::R14D:   m_regs.r14 = value.u32;     m_regsChanged = true;  break;
        case Reg::R15D:   m_regs.r15 = value.u32;     m_regsChanged = true;  break;
        case Reg::EIP:    m_regs.rip = value.u32;     m_regsChanged = true;  break;
        case Reg::EFLAGS: m_regs.rflags = value.u32;  m_regsChanged = true;  break;
        case Reg::EFER:   m_sregs.efer = value.u32;   m_sregsChanged = true; break;

        case Reg::RAX:    m_regs.rax = value.u64;         m_regsChanged = true;  break;
        case Reg::RCX:    m_regs.rcx = value.u64;         m_regsChanged = true;  break;
        case Reg::RDX:    m_regs.rdx = value.u64;         m_regsChanged = true;  break;
        case Reg::RBX:    m_regs.rbx = value.u64;         m_regsChanged = true;  break;
        case Reg::RSI:    m_regs.rsi = value.u64;         m_regsChanged = true;  break;
        case Reg::RDI:    m_regs.rdi = value.u64;         m_regsChanged = true;  break;
        case Reg::RSP:    m_regs.rsp = value.u64;         m_regsChanged = true;  break;
        case Reg::RBP:    m_regs.rbp = value.u64;         m_regsChanged = true;  break;
        case Reg::R8:     m_regs.r8 = value.u64;          m_regsChanged = true;  break;
        case Reg::R9:     m_regs.r9 = value.u64;          m_regsChanged = true;  break;
        case Reg::R10:    m_regs.r10 = value.u64;         m_regsChanged = true;  break;
        case Reg::R11:    m_regs.r11 = value.u64;         m_regsChanged = true;  break;
        case Reg::R12:    m_regs.r12 = value.u64;         m_regsChanged = true;  break;
        case Reg::R13:    m_regs.r13 = value.u64;         m_regsChanged = true;  break;
        case Reg::R14:    m_regs.r14 = value.u64;         m_regsChanged = true;  break;
        case Reg::R15:    m_regs.r15 = value.u64;         m_regsChanged = true;  break;
        case Reg::RIP:    m_regs.rip = value.u64;         m_regsChanged = true;  break;
        case Reg::RFLAGS: m_regs.rflags = value.u64;      m_regsChanged = true;  break;
        case Reg::CR0:    m_sregs.cr0 = value.u64;        m_sregsChanged = true;  break;
        case Reg::CR2:    m_sregs.cr2 = value.u64;        m_sregsChanged = true;  break;
        case Reg::CR3:    m_sregs.cr3 = value.u64;        m_sregsChanged = true;  break;
        case Reg::CR4:    m_sregs.cr4 = value.u64;        m_sregsChanged = true;  break;
        case Reg::DR0:    m_debugRegs.db[0] = value.u64;  m_debugRegsChanged = true;  break;
        case Reg::DR1:    m_debugRegs.db[1] = value.u64;  m_debugRegsChanged = true;  break;
        case Reg::DR2:    m_debugRegs.db[2] = value.u64;  m_debugRegsChanged = true;  break;
        case Reg::DR3:    m_debugRegs.db[3] = value.u64;  m_debugRegsChanged = true;  break;
        case Reg::DR6:    m_debugRegs.dr6 = value.u64;    m_debugRegsChanged = true;  break;
        case Reg::DR7:    m_debugRegs.dr7 = value.u64;    m_debugRegsChanged = true;  break;

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

VPOperationStatus KvmVirtualProcessor::GetFPUControl(FPUControl& value) noexcept {
    REFRESH_REGISTERS;

    value.cw = m_fpuRegs.fcw;
    value.sw = m_fpuRegs.fsw;
    value.tw = m_fpuRegs.ftwx; // Incompatible format?
    value.op = m_fpuRegs.last_opcode;
    value.cs = 0; // KVM is missing FPU CS
    value.rip = m_fpuRegs.last_ip;
    value.ds = 0; // KVM is missing FPU DS
    value.rdp = m_fpuRegs.last_dp;

    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::SetFPUControl(const FPUControl& value) noexcept {
    REFRESH_REGISTERS;

    m_fpuRegs.fcw = value.cw;
    m_fpuRegs.fsw = value.sw;
    m_fpuRegs.ftwx = static_cast<__u8>(value.tw); // Incompatible format?
    m_fpuRegs.last_opcode = value.op;
    m_fpuRegs.last_ip = value.rip;
    m_fpuRegs.last_dp = value.rdp;

    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::GetMXCSR(MXCSR& value) noexcept {
    REFRESH_REGISTERS;
    value.u32 = m_fpuRegs.mxcsr;
    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::SetMXCSR(const MXCSR& value) noexcept {
    REFRESH_REGISTERS;
    m_fpuRegs.mxcsr = value.u32;
    m_fpuRegsChanged = true;
    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::GetMXCSRMask(MXCSR& value) noexcept {
    return VPOperationStatus::Unsupported;
}

VPOperationStatus KvmVirtualProcessor::SetMXCSRMask(const MXCSR& value) noexcept {
    return VPOperationStatus::Unsupported;
}
// ----- Model specific registers ---------------------------------------------

VPOperationStatus KvmVirtualProcessor::GetMSR(const uint64_t msr, uint64_t& value) noexcept {
    kvm_msrs *msrData = (kvm_msrs *)malloc(sizeof(kvm_msrs) + 1 * sizeof(kvm_msr_entry));
    memset(msrData, 0, sizeof(kvm_msrs) + 1 * sizeof(kvm_msr_entry));
    msrData->entries[0].index = (__u32) msr;
    msrData->nmsrs = 1;

    if (ioctl(m_fd, KVM_GET_MSRS, msrData) < 0) {
        free(msrData);
        return VPOperationStatus::Failed;
    }

    value = msrData->entries[0].data;
    free(msrData);
    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::SetMSR(const uint64_t msr, const uint64_t value) noexcept {
    kvm_msrs *msrData = (kvm_msrs *)malloc(sizeof(kvm_msrs) + 1 * sizeof(kvm_msr_entry));
    memset(msrData, 0, sizeof(kvm_msrs) + 1 * sizeof(kvm_msr_entry));
    msrData->entries[0].index = (__u32) msr;
    msrData->entries[0].data = value;
    msrData->nmsrs = 1;

    if (ioctl(m_fd, KVM_SET_MSRS, msrData) < 0) {
        free(msrData);
        return VPOperationStatus::Failed;
    }

    free(msrData);
    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) noexcept {
    kvm_msrs *msrData = (kvm_msrs *)malloc(sizeof(kvm_msrs) + numRegs * sizeof(kvm_msr_entry));
    memset(msrData, 0, sizeof(kvm_msrs) + numRegs * sizeof(kvm_msr_entry));
    for (size_t i = 0; i < numRegs; i++) {
        msrData->entries[i].index = (__u32) msrs[i];
    }
    msrData->nmsrs = (__u32) numRegs;

    if (ioctl(m_fd, KVM_GET_MSRS, msrData) < 0) {
        free(msrData);
        return VPOperationStatus::Failed;
    }

    for (size_t i = 0; i < numRegs; i++) {
        values[i] = msrData->entries[i].data;
    }
    free(msrData);
    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) noexcept {
    kvm_msrs *msrData = (kvm_msrs *)malloc(sizeof(kvm_msrs) + numRegs * sizeof(kvm_msr_entry));
    memset(msrData, 0, sizeof(kvm_msrs) + numRegs * sizeof(kvm_msr_entry));
    for (size_t i = 0; i < numRegs; i++) {
        msrData->entries[i].index = (__u32) msrs[i];
        msrData->entries[i].data = values[i];
    }
    msrData->nmsrs = (__u32) numRegs;

    if (ioctl(m_fd, KVM_SET_MSRS, msrData) < 0) {
        free(msrData);
        return VPOperationStatus::Failed;
    }

    free(msrData);
    return VPOperationStatus::OK;
}

// ----- Breakpoints ----------------------------------------------------------

VPOperationStatus KvmVirtualProcessor::EnableSoftwareBreakpoints(bool enable) noexcept {
    if (enable) {
        m_debug.control |= KVM_GUESTDBG_USE_SW_BP;
    }
    else {
        m_debug.control &= ~KVM_GUESTDBG_USE_SW_BP;
    }

    if (!SetDebug()) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::SetHardwareBreakpoints(HardwareBreakpoints breakpoints) noexcept {
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

    m_debug.control |= KVM_GUESTDBG_USE_HW_BP;
    for (int i = 0; i < 4; i++) {
        m_debug.arch.debugreg[i] = breakpoints.bp[i].address;
        m_debug.arch.debugreg[7] |= (uint64_t)(breakpoints.bp[i].localEnable ? 1 : 0) << (0 + i * 2);
        m_debug.arch.debugreg[7] |= (uint64_t)(breakpoints.bp[i].globalEnable ? 1 : 0) << (1 + i * 2);
        m_debug.arch.debugreg[7] |= (uint64_t)(static_cast<uint8_t>(breakpoints.bp[i].trigger) & 0x3) << (16 + i * 4);
        m_debug.arch.debugreg[7] |= (uint64_t)(static_cast<uint8_t>(breakpoints.bp[i].length) & 0x3) << (18 + i * 4);
    }

    if (!SetDebug()) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::ClearHardwareBreakpoints() noexcept {
    m_debug.control &= ~KVM_GUESTDBG_USE_HW_BP;
    memset(m_debug.arch.debugreg, 0, 8 * sizeof(uint64_t));
    if (!SetDebug()) {
        return VPOperationStatus::Failed;
    }

    return VPOperationStatus::OK;
}

VPOperationStatus KvmVirtualProcessor::GetBreakpointAddress(uint64_t *address) const noexcept {
    char hardwareBP = static_cast<char>(m_kvmRun->debug.arch.dr6 & 0xF);

    // No breakpoints were hit
    if (hardwareBP == 0 && m_kvmRun->debug.arch.pc == 0) {
        return VPOperationStatus::BreakpointNeverHit;
    }

    // Get the appropriate breakpoint address
    /**/ if (hardwareBP & (1 << 0)) *address = m_debug.arch.debugreg[0];
    else if (hardwareBP & (1 << 1)) *address = m_debug.arch.debugreg[1];
    else if (hardwareBP & (1 << 2)) *address = m_debug.arch.debugreg[2];
    else if (hardwareBP & (1 << 3)) *address = m_debug.arch.debugreg[3];
    else /************************/ *address = m_kvmRun->debug.arch.pc;

    return VPOperationStatus::OK;
}

}
