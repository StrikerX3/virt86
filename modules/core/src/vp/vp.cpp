/*
Base implementation of the VirtualProcessor interface.
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
#include "virt86/vp/vp.hpp"
#include "virt86/vm/vm.hpp"
#include "virt86/platform/platform.hpp"

namespace virt86 {

VirtualProcessor::VirtualProcessor(VirtualMachine& vm)
    : m_vm(vm)
    , m_io(vm.m_io)
{
}

VirtualProcessor::~VirtualProcessor() noexcept {
}

// ----- Basic virtual processor operations -----------------------------------

VPExecutionStatus VirtualProcessor::Run() {
    HandleInterruptQueue();
    return RunImpl();
}

VPExecutionStatus VirtualProcessor::Step() {
    if (!m_vm.GetPlatform().GetFeatures().guestDebugging) {
        return VPExecutionStatus::Unsupported;
    }

    HandleInterruptQueue();
    return StepImpl();
}

bool VirtualProcessor::EnqueueInterrupt(uint8_t vector) {
    std::lock_guard<std::mutex> guard(m_interruptMutex);
    m_pendingInterrupts.push(vector);
    return PrepareInterrupt(vector);
}

// ----- Physical memory ------------------------------------------------------

bool VirtualProcessor::MemRead(const uint64_t paddr, const uint64_t size, void *value) const noexcept {
    return m_vm.MemRead(paddr, size, value);
}

bool VirtualProcessor::MemWrite(const uint64_t paddr, const uint64_t size, const void *value) const noexcept {
    return m_vm.MemWrite(paddr, size, value);
}

// ----- Linear memory --------------------------------------------------------

template<uint64_t linAddrBits, uint64_t tableBits>
static constexpr uint64_t BuildAddress(const uint64_t linAddr, const uint64_t tableAddr) {
    // Ensure we have the correct types and valid values
    static_assert(linAddrBits > 0);
    static_assert(tableBits > 0);
    static_assert(linAddrBits + tableBits <= 64);

    // Compute the masks and shifts
    constexpr uint64_t linAddrMask = 0xFFFFFFFF'FFFFFFFFull >> (64 - linAddrBits);
    constexpr uint64_t tableMask = 0xFFFFFFFF'FFFFFFFFull >> (64 - tableBits);
    constexpr uint64_t tableShift = linAddrBits;

    // Compose the address
    return ((tableAddr & tableMask) << tableShift) | (linAddr & linAddrMask);
}

template<uint64_t linAddrBits, uint64_t tableBits, class PagingEntryType>
static bool GetEntry(const VirtualProcessor& vp, PagingEntryType& entry, const uint64_t linAddr, const uint64_t tableAddr) noexcept {
    // Ensure we have the correct type of paging entry
    static_assert(EnablePagingEntry<PagingEntryType>::enable);
    // Address bits are checked by BuildAddress below

    // Compute the entry address
    const uint64_t entryAddr = BuildAddress<linAddrBits, tableBits>(linAddr, tableAddr);

    // Get the entry
    if (!vp.MemRead(entryAddr, sizeof(PagingEntryType), &entry)) {
        return false;
    }

    // Check that it is valid
    return entry.valid;
}

bool VirtualProcessor::LinearToPhysical(const uint64_t laddr, uint64_t *paddr) noexcept {
    // It's pointless to convert without a place to store the result
    if (paddr == nullptr) {
        return false;
    }

    // TODO: check that reserved bits are all zero

    // Read registers used by paging
    RegValue cr0, cr4, efer;
    if (RegRead(Reg::CR0, cr0) != VPOperationStatus::OK) {
        return false;
    }
    if (RegRead(Reg::CR4, cr4) != VPOperationStatus::OK) {
        return false;
    }
    if (RegRead(Reg::EFER, efer) != VPOperationStatus::OK) {
        return false;
    }

    // Check paging flag
    if ((cr0.u64 & CR0_PG) == 0) {
        // No paging

        // In this mode, linear addresses are 32 bits wide and translate
        // directly to physical addresses
        *paddr = laddr & 0xFFFFFFFF;
        return true;
    }

    // Paging is enabled

    // Check Physical Address Extensions flag
    if ((cr4.u64 & CR4_PAE) == 0) {
        // 32-bit paging
        return LinearToPhysical32(static_cast<uint32_t>(laddr), paddr);
    }

    // Physical Address Extensions are enabled

    // Check Long Mode Enable flag
    if ((efer.u64 & EFER_LME) == 0) {
        // PAE paging
        return LinearToPhysicalPAE(static_cast<uint32_t>(laddr), paddr);
    }

    // 4-level paging
    return LinearToPhysical4Level(laddr, paddr);
}

bool VirtualProcessor::LinearToPhysical32(const uint32_t laddr, uint64_t *paddr) noexcept {
    // TODO: check PAT

    // Read registers used in this mode
    RegValue cr3, cr4;
    if (RegRead(Reg::CR3, cr3) != VPOperationStatus::OK) {
        return false;
    }
    if (RegRead(Reg::CR4, cr4) != VPOperationStatus::OK) {
        return false;
    }

    // In this mode, the PDE address bits are defined as follows:
    // [39:32] = 0
    // [31:12] = CR3     [31:12]
    // [11: 2] = linaddr [31:22]
    // [ 1: 0] = 0
    PDE32 pde;
    if (!GetEntry<12, 20>(*this, pde, ((static_cast<uint64_t>(laddr) >> 22u) << 2u), (cr3.u32 >> 12u))) {
        return false;
    }

    // If CR4.PSE = 1 and the PDE uses large pages, it points to a 4 MB page,
    // producing physical addresses that are 40 bits wide.
    // If CR4.PSE = 0, the bit is ignored.
    if (pde.largePage && (cr4.u64 & CR4_PSE)) {
        // The final physical address bits are as follows:
        // [39:32] = PDE     [20:13]
        // [31:22] = PDE     [31:22]
        // [21: 0] = linaddr [21: 0]
        *paddr = BuildAddress<22, 18>(laddr, (static_cast<uint64_t>(pde.large.addrHigh) << 10ull) | (static_cast<uint64_t>(pde.large.addrLow)));
        return true;
    }

    // The PDE points to a table consisting of 1024 32-bit PTEs.
    // The physical address of the PTE is defined as follows:
    // [39:32] = 0
    // [31:12] = PDE     [31:12]
    // [11: 2] = linaddr [21:12]
    // [ 1: 0] = 0
    PTE32 pte;
    if (!GetEntry<12, 20>(*this, pte, ((static_cast<uint64_t>(laddr) >> 12u) << 2u), pde.table.pageFrameNumber)) {
        return false;
    }

    // The final physical address bits are defined as follows:
    // [39:32] = 0
    // [31:12] = PTE     [31:12]
    // [11: 0] = linaddr [11: 0]
    *paddr = BuildAddress<12, 20>(laddr, pte.pageFrameNumber);
    return true;
}

bool VirtualProcessor::LinearToPhysicalPAE(const uint32_t laddr, uint64_t *paddr) noexcept {
    // Read register used in this mode
    RegValue cr3;
    if (RegRead(Reg::CR3, cr3) != VPOperationStatus::OK) {
        return false;
    }

    // Determine PDPTE index, which comes from bits [31:30] of the
    // linear address
    const uint8_t pdpteIndex = laddr >> 30u;

    // Technically, the CPU maintains PDPTEs 0 through 4 in
    // internal registers, but they are read from physical memory

    // The PDPTE physical address bits are defined as follows:
    // [31: 5] = CR3 [31: 5]
    // [ 4: 3] = PDPTE index [1:0]
    // [ 2: 0] = 0
    PDPTE pdpte;
    if (!GetEntry<5, 27>(*this, pdpte, (static_cast<uint64_t>(pdpteIndex) << 3u), (cr3.u64 >> 5ull))) {
        return false;
    }

    // The PDPTE points to a table consisting of 512 64-bit PDEs.
    // The PDE physical address bits are defined as follows:
    // [51:12] = PDPTE   [51:12]
    // [11: 3] = linaddr [11: 3]
    // [ 2: 0] = 0
    PDE64 pde;
    if (!GetEntry<12, 40>(*this, pde, (static_cast<uint64_t>(laddr) & ~0b111), pdpte.table.address)) {
        return false;
    }

    // If the PDE uses 2 MiB pages, the final physical address bits are defined
    // as follows:
    // [51:21] = PDE     [51:21]
    // [20: 0] = linaddr [20: 0]
    if (pde.largePage) {
        *paddr = BuildAddress<21, 31>(laddr, pde.large.address);
        return true;
    }

    // The PDE points to a table consisting of 512 64-bit PTEs.
    // The PTE physical address bits are defined as follows:
    // [51:12] = PDE     [51:12]
    // [11: 3] = linaddr [20:12]
    // [ 2: 0] = 0
    PTE64 pte;
    if (!GetEntry<12, 40>(*this, pte, ((static_cast<uint64_t>(laddr) >> 12u) << 3u), pde.table.address)) {
        return false;
    }

    // The final physical address bits are defined as follows:
    // [51:12] = PTE     [51:12]
    // [11: 0] = linaddr [11: 0]
    *paddr = BuildAddress<12, 40>(laddr, pte.address);
    return true;
}

bool VirtualProcessor::LinearToPhysical4Level(const uint64_t laddr, uint64_t *paddr) noexcept {
    // TODO: check CR4.PKE

    // Read register used in this mode
    RegValue cr3;
    if (RegRead(Reg::CR3, cr3) != VPOperationStatus::OK) {
        return false;
    }

    // The PML4E address bits are defined as follows:
    // [51:12] = CR3     [51:12]
    // [11: 3] = linaddr [47:39]
    // [ 2: 0] = 0
    PML4E pml4e;
    if (!GetEntry<12, 40>(*this, pml4e, ((laddr >> 39ull) << 3ull), (cr3.u64 >> 12ull))) {
        return false;
    }

    // The PML4E points to a table consisting of 512 64-bit PDPTEs.
    // The PDPTE physical address bits are defined as follows:
    // [51:12] = PML4E   [51:12]
    // [11: 3] = linaddr [38:30]
    // [ 2: 0] = 0
    PDPTE pdpte;
    if (!GetEntry<12, 40>(*this, pdpte, ((laddr >> 30ull) << 3ull), pml4e.address)) {
        return false;
    }

    // If the PDPTE uses 1 GiB pages, the final physical address bits are
    // defined as follows:
    // [51:30] = PDPTE   [51:30]
    // [29: 0] = linaddr [29: 0]
    if (pdpte.largePage) {
        *paddr = BuildAddress<30, 22>(laddr, pdpte.large.address);
        return true;
    }

    // The PDPTE points to a table consisting of 512 64-bit PDEs.
    // The PDE physical address bits are defined as follows:
    // [51:12] = PDPTE   [51:12]
    // [11: 3] = linaddr [29:21]
    // [ 2: 0] = 0
    PDE64 pde;
    if (!GetEntry<12, 40>(*this, pde, ((laddr >> 21ull) << 3ull), pdpte.table.address)) {
        return false;
    }

    // If the PDE uses 2 MiB pages, the final physical address bits are defined
    // as follows:
    // [51:21] = PDE     [51:21]
    // [20: 0] = linaddr [20: 0]
    if (pde.largePage) {
        *paddr = BuildAddress<21, 31>(laddr, pde.large.address);
        return true;
    }

    // The PDE points to a table consisting of 512 64-bit PTEs.
    // The PTE physical address bits are defined as follows:
    // [51:12] = PDE     [51:12]
    // [11: 3] = linaddr [20:12]
    // [ 2: 0] = 0
    PTE64 pte;
    if (!GetEntry<12, 40>(*this, pte, ((laddr >> 12ull) << 3ull), pde.table.address)) {
        return false;
    }

    // The final physical address bits are defined as follows:
    // [51:12] = PTE     [51:12]
    // [11: 0] = linaddr [11: 0]
    *paddr = BuildAddress<12, 40>(laddr, pte.address);
    return true;
}

bool VirtualProcessor::LMemRead(const uint64_t laddr, const uint64_t size, void *value, uint64_t *bytesRead) noexcept {
    // Value pointer is required
    if (value == nullptr) {
        return false;
    }

    // Compute the address range and align to 4 KiB pages
    uint64_t srcAddrStart = laddr;
    uint64_t srcAddrEnd = ((srcAddrStart + PAGE_SIZE) & ~(PAGE_SIZE - 1)) - 1;
    uint64_t pos = 0;

    // Define the amount of bytes to copy based on the computed addresses
    // and trim to requested size
    uint64_t copySize = srcAddrEnd - srcAddrStart + 1;
    if (size < copySize) {
        copySize = size;
    }
    
    // Copy individual pages
    while (pos < size) {
        uint64_t physAddr;
        if (!LinearToPhysical(srcAddrStart, &physAddr)) {
            return false;
        }

        const auto result = MemRead(physAddr, copySize, static_cast<uint8_t*>(value) + pos);
        if (!result) {
            return result;
        }

        pos += copySize;
        srcAddrStart = srcAddrEnd + 1;
        srcAddrEnd += PAGE_SIZE;
        copySize = size - pos;
        if (PAGE_SIZE < copySize) {
            copySize = PAGE_SIZE;
        }
    }

    if (bytesRead != nullptr) {
        *bytesRead = pos;
    }
    return true;
}

bool VirtualProcessor::LMemWrite(const uint64_t laddr, const uint64_t size, const void *value, uint64_t *bytesWritten) noexcept {
    // Value pointer is required
    if (value == nullptr) {
        return false;
    }

    // Compute the address range and align to 4 KiB pages
    uint64_t srcAddrStart = laddr;
    uint64_t srcAddrEnd = ((srcAddrStart + PAGE_SIZE) & ~(PAGE_SIZE - 1)) - 1;
    uint64_t pos = 0;

    // Define the amount of bytes to copy based on the computed addresses
    // and trim to requested size
    uint64_t copySize = srcAddrEnd - srcAddrStart + 1;
    if (size < copySize) {
        copySize = size;
    }

    // Copy individual pages
    while (pos < size) {
        uint64_t physAddr;
        if (!LinearToPhysical(srcAddrStart, &physAddr)) {
            return false;
        }

        const auto result = MemWrite(physAddr, copySize, static_cast<const uint8_t*>(value) + pos);
        if (!result) {
            return result;
        }

        pos += copySize;
        srcAddrStart = srcAddrEnd + 1;
        srcAddrEnd += PAGE_SIZE;
        copySize = size - pos;
        if (PAGE_SIZE < copySize) {
            copySize = PAGE_SIZE;
        }
    }

    if (bytesWritten != nullptr) {
        *bytesWritten = pos;
    }
    return true;
}

// ----- Utility macros for registers -----------------------------------------

#define CHECK_RESULT(expr) do { \
    const auto result = (expr);\
    if (result != VPOperationStatus::OK) return result; \
} while (0)

#define CHECK_RESULT_MEM(expr) do { \
    const auto result = (expr);\
    if (!result) return VPOperationStatus::Failed; \
} while (0)

// ----- Registers ------------------------------------------------------------

VPOperationStatus VirtualProcessor::RegCopy(const Reg dst, const Reg src) noexcept {
    RegValue tmp;
    CHECK_RESULT(RegRead(src, tmp));
    CHECK_RESULT(RegWrite(dst, tmp));
    return VPOperationStatus::OK;
}

VPOperationStatus VirtualProcessor::RegRead(const Reg regs[], RegValue values[], const size_t numRegs) noexcept {
    if (regs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    for (size_t i = 0; i < numRegs; i++) {
        const auto status = RegRead(regs[i], values[i]);
        if (status != VPOperationStatus::OK) {
            return status;
        }
    }
    return VPOperationStatus::OK;
}

VPOperationStatus VirtualProcessor::RegWrite(const Reg regs[], const RegValue values[], const size_t numRegs) noexcept {
    if (regs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    for (size_t i = 0; i < numRegs; i++) {
        const auto status = RegWrite(regs[i], values[i]);
        if (status != VPOperationStatus::OK) {
            return status;
        }
    }
    return VPOperationStatus::OK;
}

VPOperationStatus VirtualProcessor::RegCopy(const Reg dsts[], const Reg srcs[], const size_t numRegs) noexcept {
    if (dsts == nullptr || srcs == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    for (size_t i = 0; i < numRegs; i++) {
        const auto status = RegCopy(dsts[i], srcs[i]);
        if (status != VPOperationStatus::OK) {
            return status;
        }
    }
    return VPOperationStatus::OK;
}

// ----- Model specific registers ---------------------------------------------

VPOperationStatus VirtualProcessor::GetMSRs(const uint64_t msrs[], uint64_t values[], const size_t numRegs) noexcept {
    if (msrs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    for (size_t i = 0; i < numRegs; i++) {
        const auto status = GetMSR(msrs[i], values[i]);
        if (status != VPOperationStatus::OK) {
            return status;
        }
    }
    return VPOperationStatus::OK;
}

VPOperationStatus VirtualProcessor::SetMSRs(const uint64_t msrs[], const uint64_t values[], const size_t numRegs) noexcept {
    if (msrs == nullptr || values == nullptr) {
        return VPOperationStatus::InvalidArguments;
    }

    for (size_t i = 0; i < numRegs; i++) {
        const auto status = SetMSR(msrs[i], values[i]);
        if (status != VPOperationStatus::OK) {
            return status;
        }
    }
    return VPOperationStatus::OK;
}

// ----- Utility methods for segment and table registers ----------------------

bool VirtualProcessor::IsIA32eMode() noexcept {
    Reg regs[3] = { Reg::CR0, Reg::RFLAGS, Reg::EFER };
    RegValue vals[3];
    if (RegRead(regs, vals, std::size(regs)) != VPOperationStatus::OK) return false;

    bool cr0_pe = (vals[0].u64 & CR0_PE) != 0;
    bool rflags_vm = (vals[1].u64 & RFLAGS_VM) != 0;
    bool efer_lma = (vals[2].u64 & EFER_LMA) != 0;

    return cr0_pe && rflags_vm && efer_lma;
}

// ----- Global Descriptor Table ----------------------------------------------

VPOperationStatus VirtualProcessor::GetGDTEntry(const uint16_t selector, GDTEntry& entry) noexcept {
    RegValue gdt;
    CHECK_RESULT(RegRead(Reg::GDTR, gdt));
    if (selector + sizeof(GenericGDTDescriptor) > gdt.table.limit) {
        return VPOperationStatus::InvalidSelector;
    }
    
    // Check GDT entry type
    CHECK_RESULT_MEM(MemRead(gdt.table.base + selector, sizeof(GenericGDTDescriptor), &entry));
    if (entry.generic.data.system) {
        // GDT code or data descriptor; nothing to do here
        return VPOperationStatus::OK;
    }
    
    // At this point we have a system descriptor: LDT, TSS or any gate
    
    // In IA-32e mode, some of these descriptors are extended to 16 bytes.
    if (IsIA32eMode()) {
        switch (entry.generic.data.type) {
        case 0b0010: // LDT
        case 0b1001: case 0b1011: // TSS
        case 0b1100: case 0b1110: case 0b1111: // Call Gate, Interrupt Gate, Trap Gate
            if (selector + sizeof(GDTEntry) > gdt.table.limit) {
                return VPOperationStatus::InvalidSelector;
            }
            CHECK_RESULT_MEM(MemRead(gdt.table.base + selector, sizeof(GDTEntry), &entry));
            break;
        default: // Reserved
            return VPOperationStatus::InvalidSelector;
        }
    }
    else {
        // No descriptors need to be extended; just return an error if the
        // entry type is reserved
        switch (entry.generic.data.type) {
        case 0b0000: case 0b1000: case 0b1010: case 0b1101: // Reserved
            return VPOperationStatus::InvalidSelector;
        }
    }

    return VPOperationStatus::OK;
}

VPOperationStatus VirtualProcessor::SetGDTEntry(const uint16_t selector, const GDTEntry& entry) noexcept {
    RegValue gdt;
    CHECK_RESULT(RegRead(Reg::GDTR, gdt));
    if (selector + sizeof(GenericGDTDescriptor) > gdt.table.limit) {
        return VPOperationStatus::InvalidSelector;
    }

    // Check GDT entry type
    CHECK_RESULT_MEM(MemWrite(gdt.table.base + selector, sizeof(GenericGDTDescriptor), &entry));
    if (entry.generic.data.system) {
        // GDT code or data descriptor; nothing to do here
        return VPOperationStatus::OK;
    }
    
    // In IA-32e mode, some of these descriptors are extended to 16 bytes.
    if (IsIA32eMode()) {
        switch (entry.generic.data.type) {
        case 0b0010: // LDT
        case 0b1001: case 0b1011: // TSS
        case 0b1100: case 0b1110: case 0b1111: // Call Gate, Interrupt Gate, Trap Gate
            if (selector + sizeof(GDTEntry) > gdt.table.limit) {
                return VPOperationStatus::InvalidSelector;
            }
            CHECK_RESULT_MEM(MemWrite(gdt.table.base + selector, sizeof(GDTEntry), &entry));
            break;
        default: // Reserved
            return VPOperationStatus::InvalidSelector;
        }
    }
    else {
        // No descriptors need to be extended; just return an error if the
        // entry type is reserved
        switch (entry.generic.data.type) {
        case 0b0000: case 0b1000: case 0b1010: case 0b1101: // Reserved
            return VPOperationStatus::InvalidSelector;
        }
    }

    return VPOperationStatus::OK;
}

// ----- Interrupt Descriptor Table -------------------------------------------

VPOperationStatus VirtualProcessor::GetIDTEntry(const uint8_t vector, IDTEntry& entry) noexcept {
    RegValue idt;
    CHECK_RESULT(RegRead(Reg::IDTR, idt));
    if (vector * sizeof(IDTEntry) > idt.table.limit) {
        return VPOperationStatus::InvalidSelector;
    }
    CHECK_RESULT_MEM(MemRead(idt.table.base + vector * sizeof(IDTEntry), sizeof(IDTEntry), &entry));
    return VPOperationStatus::OK;
}

VPOperationStatus VirtualProcessor::SetIDTEntry(const uint8_t vector, const IDTEntry& entry) noexcept {
    RegValue idt;
    CHECK_RESULT(RegRead(Reg::IDTR, idt));
    if (vector * sizeof(IDTEntry) > idt.table.limit) {
        return VPOperationStatus::InvalidSelector;
    }
    CHECK_RESULT_MEM(MemWrite(idt.table.base + vector * sizeof(IDTEntry), sizeof(IDTEntry), &entry));
    return VPOperationStatus::OK;
}

// ----- Segment registers ----------------------------------------------------

VPOperationStatus VirtualProcessor::ReadSegment(const uint16_t selector, RegValue& value) noexcept {
    // Get GDT entry from memory
    GDTEntry gdtEntry;
    auto status = GetGDTEntry(selector, gdtEntry);
    if (status != VPOperationStatus::OK) {
        return status;
    }

    // Handle system entries (LDT and TSS)
    if (!gdtEntry.generic.data.system) {
        // Load the appropriate entry type
        switch (gdtEntry.generic.data.type) {
        case 0b0010:  // LDT
        {
            // Fill in segment info with data from the LDT entry
            value.segment.selector = selector;
            value.segment.base = gdtEntry.ldt.GetBase();
            value.segment.limit = gdtEntry.ldt.GetLimit();
            value.segment.attributes.u16 = gdtEntry.ldt.GetAttributes();

            return VPOperationStatus::OK;
        }
        case 0b0001: case 0b0011:  // 16-bit TSS in 32-bit mode; reserved in IA-32e mode
        case 0b1001: case 0b1011:  // 32-bit TSS in 32-bit mode; 64-bit TSS in IA-32e mode
        {
            if ((gdtEntry.generic.data.type & 0b1000) == 0 && IsIA32eMode()) {
                // Reserved entries are invalid
                return VPOperationStatus::InvalidSelector;
            }

            // Fill in segment info with data from the TSS entry
            value.segment.selector = selector;
            value.segment.base = gdtEntry.tss.GetBase();
            value.segment.limit = gdtEntry.tss.GetLimit();
            value.segment.attributes.u16 = gdtEntry.tss.GetAttributes();

            return VPOperationStatus::OK;
        }
        default:
            // None of these entry types can be loaded into segment registers:
            // - Call gates
            // - Task gates
            // - Interrupt gates
            // - Trap gates
            return VPOperationStatus::InvalidSelector;
        }
    }

    // Fill in segment info with data from the GDT entry
    value.segment.selector = selector;
    value.segment.base = gdtEntry.gdt.GetBase();
    value.segment.limit = gdtEntry.gdt.GetLimit();
    value.segment.attributes.u16 = gdtEntry.gdt.GetAttributes();

    return VPOperationStatus::OK;
}

// ----- Intenal functions ----------------------------------------------------

void VirtualProcessor::HandleInterruptQueue() {
    // Inject an interrupt if available and possible, requesting an interrupt
    // window if the virtual processor is not ready
    if (m_pendingInterrupts.size() > 0) {
        if (CanInjectInterrupt()) {
            InjectPendingInterrupt();
        }
        else {
            RequestInterruptWindow();
        }
    }
}

void VirtualProcessor::InjectPendingInterrupt() {
    // If there are no pending interrupts, leave
    if (m_pendingInterrupts.size() == 0) {
        return;
    }

    // Dequeue the next interrupt vector and inject it into the virtual
    // processor while holding the pending interrupts mutex
    {
        std::lock_guard<std::mutex> guard(m_interruptMutex);

        const uint8_t vector = m_pendingInterrupts.front();
        m_pendingInterrupts.pop();
        InjectInterrupt(vector);
    }

    return;
}

// ----- Optional operations --------------------------------------------------

VPOperationStatus VirtualProcessor::EnableSoftwareBreakpoints(bool enable) noexcept {
    return VPOperationStatus::Unsupported;
}

VPOperationStatus VirtualProcessor::SetHardwareBreakpoints(HardwareBreakpoints breakpoints) noexcept {
    return VPOperationStatus::Unsupported;
}

VPOperationStatus VirtualProcessor::ClearHardwareBreakpoints() noexcept {
    return VPOperationStatus::Unsupported;
}

VPOperationStatus VirtualProcessor::GetBreakpointAddress(uint64_t *address) const noexcept {
    return VPOperationStatus::Unsupported;
}

VPExecutionStatus VirtualProcessor::StepImpl() noexcept {
    return VPExecutionStatus::Unsupported;
}

#undef CHECK_RESULT
#undef CHECK_RESULT_MEM

}
