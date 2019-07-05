/*
Defines the interface for virtual processors inside a virtual machine.

The main purpose a virtual processor is to run virtualized code. A virtual
processor enables this through two methods:
- Run(), which runs the virtual processor until a condition causes it to exit
- Step(), which runs a single instruction

Single stepping is only supported on platforms that expose the guestDebugging
feature.

It's also possible to read from and write to physical or linear memory
addresses with MemRead, MemWrite, LMemRead and LMemWrite methods.
Linear address translation will take into account the current VCPU paging mode.
You can also translate a linear address to a physical address using the
LinearToPhysical method.

Various methods are provided to read and write directly to VCPU registers and
control structures.

Interrupts can be enqueued via the EnqueueInterrupt() method. They will be
injected once an interrupt injection window opens. The implementation will
automatically request for interrupt windows or cancel VCPU execution as needed.

For platforms that support guest debugging, it is possible to enable software
breakpoint and define or clear hardware breakpoints, as well as determine the
address of the last breakpoint hit as of the most recent execution.

Optional operations must return the Unsupported status code for platforms that
do not offer support for them. The corresponding methods must be overridden by
platforms that support them.

Some operations, such as bulk register read/write/copy, may be overridden if
the platform offers a more efficient method to perform them. The default
implementation delegates to the singular accessor methods in a loop.

Please note that the VirtualProcessor class is not meant to be shared between
multiple threads. It is, however, possible to use multiple virtual processors
on different threads, as long as each thread uses their own instance.
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

#include "gdt.hpp"
#include "idt.hpp"
#include "regs.hpp"
#include "fpregs.hpp"
#include "cpuid.hpp"
#include "hwbp.hpp"
#include "paging.hpp"
#include "status.hpp"
#include "mode.hpp"
#include "../vm/io.hpp"

#include <cstdint>
#include <cstring>
#include <vector>
#include <queue>
#include <mutex>

#define KiB (1024)
#define MiB (KiB*KiB)
#define PAGE_SIZE (4*KiB)
#define PAGE_SIZE_LARGE (4*MiB)
#define PAGE_SHIFT (12)

namespace virt86 {

// Forward declare the virtual machine class in order to give it access to the
// VirtualProcessor class destructor
class VirtualMachine;

// ----- Virtual processor base class -----------------------------------------

/**
 * Defines the interface for CPU virtualization operations.
 */
class VirtualProcessor {
public:
    // Prevent copy construction and copy assignment
    VirtualProcessor(const VirtualProcessor&) = delete;
    VirtualProcessor& operator=(const VirtualProcessor&) = delete;

    // Prevent move construction and move assignment
    VirtualProcessor(VirtualProcessor&&) = delete;
    VirtualProcessor&& operator=(VirtualProcessor&&) = delete;

    // Disallow taking the address
    VirtualProcessor *operator&() = delete;
    
    virtual ~VirtualProcessor() noexcept;
    
    // ----- Basic virtual processor operations -------------------------------

    /**
     * Runs the virtual processor until interrupted.
     */
    VPExecutionStatus Run();

    /**
     * Runs one instruction on the virtual processor.
     *
     * This is an optional operation, supported by platforms that provide the
     * guest breakpoint capability.
     */
    VPExecutionStatus Step();

    /**
     * Enqueues an interrupt request to the virtual processor.
     *
     * The interrupt request will be executed when an interrupt window is
     * opened by the underlying virtualization platform.
     */
    bool EnqueueInterrupt(uint8_t vector);

    // ----- CPU modes --------------------------------------------------------

    /**
     * Retrieves the current CPU execution mode based on the state of CR0.PE,
     * RFLAGS.VM and EFER.LMA.
     */
    CPUExecutionMode GetExecutionMode() noexcept;

    /**
     * Retrieves the current CPU paging mode based on the state of CR0.PG,
     * CR4.PAE and EFER.LME.
     */
    CPUPagingMode GetPagingMode() noexcept;

    // ----- Physical memory --------------------------------------------------

    /**
     * Reads a portion of physical memory into the specified value.
     */
    bool MemRead(const uint64_t paddr, const uint64_t size, void *value) const noexcept;

    /**
     * Writes the specified value into physical memory.
     */
    bool MemWrite(const uint64_t paddr, const uint64_t size, const void *value) const noexcept;

    // ----- Linear memory ----------------------------------------------------

    /**
     * Converts a linear address to a physical address. Returns true if the
     * linear address is valid.
     *
     * Takes into account the current state of the virtual processor's CR0.PG,
     * CR4.PAE and EFER.LME flags. More specifically:
     * - If CR0.PG = 0, paging is disabled.
     * - If CR0.PG = 1 and CR4.PAE = 0, 32-bit paging is used.
     * - If CR0.PG = 1, CR4.PAE = 1 and EFER.LME = 0, PAE paging is used.
     * - If CR0.PG = 1, CR4.PAE = 1 and EFER.LME = 1, 4-level paging is used.
     *
     * Translation is performed according to the specifications in "Intel� 64
     * and IA-32 Architectures Software Developer Manuals", Volume 3, section
     * 4.1, "Paging Modes and Control Bits".
     */
    bool LinearToPhysical(const uint64_t laddr, uint64_t *paddr) noexcept;

    /**
     * Reads a portion of linear memory into the specified value. x86 virtual
     * address translation is performed based on the current registers and
     * memory contents. Optionally, the caller may receive the number of bytes
     * read during the operation.
     */
    bool LMemRead(const uint64_t laddr, const uint64_t size, void *value, uint64_t *bytesRead = nullptr) noexcept;

    /**
     * Writes the specified value into linear memory. x86 virtual address
     * translation is performed based on the current registers and memory
     * contents. Optionally, the caller may receive the number of bytes written
     * during the operation.
     */
    bool LMemWrite(const uint64_t laddr, const uint64_t size, const void *value, uint64_t *bytesWritten = nullptr) noexcept;

    // ----- Registers --------------------------------------------------------

    /**
     * Reads from a register.
     */
    virtual VPOperationStatus RegRead(const Reg reg, RegValue& value) noexcept = 0;

    /**
     * Writes to a register.
     */
    virtual VPOperationStatus RegWrite(const Reg reg, const RegValue& value) noexcept = 0;

    /**
     * Copies the value between two segment registers.
     */
    VPOperationStatus RegCopy(const Reg dst, const Reg src) noexcept;

    /**
     * Reads from segment registers in bulk.
     */
    virtual VPOperationStatus RegRead(const Reg regs[], RegValue values[], const size_t numRegs) noexcept;

    /**
     * Writes to segment registers in bulk.
     */
    virtual VPOperationStatus RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) noexcept;

    /**
     * Copies the values between segment registers in bulk.
     */
    VPOperationStatus RegCopy(const Reg dsts[], const Reg srcs[], const size_t numRegs) noexcept;

    // ----- Floating point control registers ---------------------------------

    /**
     * Reads the FPU control registers.
     */
    virtual VPOperationStatus GetFPUControl(FPUControl& value) noexcept = 0;

    /**
     * Writes the FPU control registers.
     */
    virtual VPOperationStatus SetFPUControl(const FPUControl& value) noexcept = 0;

    /**
     * Reads the MXCSR register.
     */
    virtual VPOperationStatus GetMXCSR(MXCSR& value) noexcept = 0;

    /**
     * Writes the MXCSR registers.
     */
    virtual VPOperationStatus SetMXCSR(const MXCSR& value) noexcept = 0;

    /**
     * Reads the MXCSR_MASK register.
     */
    virtual VPOperationStatus GetMXCSRMask(MXCSR& value) noexcept = 0;

    /**
     * Writes the MXCSR_MASK registers.
     */
    virtual VPOperationStatus SetMXCSRMask(const MXCSR& value) noexcept = 0;
 
    // ----- Model specific registers -----------------------------------------

    /**
     * Reads from an MSR.
     *
     * Some platforms may only expose a subset of MSRs.
     * Returns VPOperationStatus::InvalidRegister if the platform doesn't
     * expose the requested MSR.
     */
    virtual VPOperationStatus GetMSR(const uint64_t msr, uint64_t& value) noexcept = 0;

    /**
     * Writes to an MSR.
     *
     * Some platforms may only expose a subset of MSRs.
     * Returns VPOperationStatus::InvalidRegister if the platform doesn't
     * expose the requested MSR.
     */
    virtual VPOperationStatus SetMSR(const uint64_t msr, const uint64_t value) noexcept = 0;

    /**
     * Reads from MSRs in bulk.
     *
     * Some platforms may only expose a subset of MSRs.
     * Returns VPOperationStatus::InvalidRegister if the platform doesn't
     * expose one of the requested MSRs.
     */
    virtual VPOperationStatus GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) noexcept;

    /**
     * Writes to MSRs in bulk.
     *
     * Some platforms may only expose a subset of MSRs.
     * Returns VPOperationStatus::InvalidRegister if the platform doesn't
     * expose one of the requested MSRs.
     */
    virtual VPOperationStatus SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) noexcept;
  
    // ----- Virtual TSC offset -----------------------------------------------

    /**
     * Retrieves the current virtual TSC offset.
     */
    virtual VPOperationStatus GetVirtualTSCOffset(uint64_t& offset) noexcept;

    /**
     * Modifies the virtual TSC offset.
     */
    virtual VPOperationStatus SetVirtualTSCOffset(const uint64_t offset) noexcept;

    // ----- Global Descriptor Table ------------------------------------------

    /**
     * Retrieves an entry from the Global Descriptor Table.
     */
    VPOperationStatus GetGDTEntry(const uint16_t selector, GDTEntry& entry) noexcept;

    /**
     * Modifies an entry in the Global Descriptor Table.
     */
    VPOperationStatus SetGDTEntry(const uint16_t selector, const GDTEntry& entry) noexcept;

    /**
     * Reads segment information for the specified selector into the register
     * value based on this virtual processor's GDT setup.
     */
    VPOperationStatus ReadSegment(const uint16_t selector, RegValue& value) noexcept;

    /**
     * Determines the bit width of the segment at the specified selector.
     */
    VPOperationStatus GetSegmentSize(const uint16_t selector, SegmentSize& size) noexcept;

    /**
     * Determines the bit width of the segment used by the specified register.
     */
    VPOperationStatus GetSegmentSize(const Reg& segmentReg, SegmentSize& size) noexcept;

    // ----- Interrupt Descriptor Table ---------------------------------------

    /**
     * Retrieves an entry from the Interrupt Descriptor Table.
     */
    VPOperationStatus GetIDTEntry(const uint8_t vector, IDTEntry& entry) noexcept;

    /**
     * Modifies an entry from the Interrupt Descriptor Table.
     */
    VPOperationStatus SetIDTEntry(const uint8_t vector, const IDTEntry& entry) noexcept;

    // ----- Breakpoints ------------------------------------------------------

    /**
     * Enables or disables software breakpoints.
     *
     * If the guest executes an INT 3h instruction while software breakpoints
     * are enabled, the hypervisor will stop with ExitReason::SoftwareBreakpoint.
     * The guest code will not have a chance to handle INT 3h while software
     * breakpoints are enabled.
     *
     * This is an optional operation, supported by platforms that provide the
     * guest breakpoint capability.
     */
    virtual VPOperationStatus EnableSoftwareBreakpoints(bool enable) noexcept;

    /**
     * Configures up to 4 hardware breakpoints.
     *
     * While these breakpoints are active, when the guest code triggers them,
     * the hypervisor will stop with ExitReason::HardwareBreakpoint.
     *
     * This is an optional operation, supported by platforms that provide the
     * guest breakpoint capability.
     */
    virtual VPOperationStatus SetHardwareBreakpoints(HardwareBreakpoints breakpoints) noexcept;

    /**
     * Clears all hardware breakpoints.
     *
     * This is an optional operation, supported by platforms that provide the
     * guest breakpoint capability.
     */
    virtual VPOperationStatus ClearHardwareBreakpoints() noexcept;

    /**
     * Retrieves the address of the most recently hit breakpoint. Must be
     * invoked after the virtual processor emulator stops due to a breakpoint,
     * and before running the virtual processor again.
     *
     * Returns VPOperationStatus::BreakpointNeverHit if a breakpoint was not
     * hit as of the most recent execution.
     *
     * This is an optional operation, supported by platforms that provide the
     * guest breakpoint capability.
     */
    virtual VPOperationStatus GetBreakpointAddress(uint64_t *address) const noexcept;

    // ----- Data -------------------------------------------------------------

    /**
     * Retrieves information the hypervisor exit.
     */
    const VMExitInfo& GetVMExitInfo() const noexcept { return m_exitInfo; }

    /**
     * Retrieves the virtual machine that owns this virtual processor.
     */
    const VirtualMachine& GetVirtualMachine() const noexcept { return m_vm; }

protected:
    VirtualProcessor(VirtualMachine& vm);

    /**
     * Tells the hypervisor to run the virtual processor until interrupted.
     */
    virtual VPExecutionStatus RunImpl() noexcept = 0;

    /**
     * Tells the hypervisor to run one instruction on the virtual processor.
     *
     * This is an optional operation, supported by platforms that provide the
     * guest breakpoint capability.
     */
    virtual VPExecutionStatus StepImpl() noexcept;

    /**
     * Gives a chance to perform additional operations on the virtual processor
     * to prepare for interrupt injection.
     *
     * Returns true if the virtual processor is ready to receive the interrupt.
     */
    virtual bool PrepareInterrupt(uint8_t vector) noexcept = 0;

    /**
     * Tells the hypervisor to inject an interrupt into the virtual processor.
     */
    virtual VPOperationStatus InjectInterrupt(uint8_t vector) noexcept = 0;

    /**
     * Determines if an interrupt can be injected into the virtual processor.
     */
    virtual bool CanInjectInterrupt() const noexcept = 0;

    /**
     * Requests for an interrupt injection window.
     */
    virtual void RequestInterruptWindow() noexcept = 0;
    
    /**
     * Reference to the virtual machine that owns this virtual processor.
     */
    const VirtualMachine& m_vm;

    /**
     * Reference to the I/O handles of the virtual machine.
     */
    const IOHandlers& m_io;

    /**
     * The hypervisor exit information.
     */
    VMExitInfo m_exitInfo;

    // Allow VirtualMachine to access the constructor
    friend class VirtualMachine;

private:
    std::mutex m_interruptMutex;
    std::queue<uint8_t> m_pendingInterrupts;

    void HandleInterruptQueue();
    void InjectPendingInterrupt();

    // ----- Helper functions -------------------------------------------------

    /**
     * Converts the given linear address into a physical address under 32-bit
     * paging mode.
     */
    bool LinearToPhysical32(const uint32_t laddr, uint64_t *paddr) noexcept;

    /**
     * Converts the given linear address into a physical address under PAE
     * paging mode.
     */
    bool LinearToPhysicalPAE(const uint32_t laddr, uint64_t *paddr) noexcept;

    /**
     * Converts the given linear address into a physical address under 4-level
     * paging mode.
     */
    bool LinearToPhysical4Level(const uint64_t laddr, uint64_t *paddr) noexcept;

    /**
     * Determines if the CPU is in IA-32e mode.
     */
    bool IsIA32eMode() noexcept;

    /**
     * Computes the size of a segment value.
     */
    SegmentSize ComputeSegmentSize(RegValue& value);
};

}
