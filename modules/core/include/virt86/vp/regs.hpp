/*
Defines the set of x86 registers and data structures for manipulating their
values.

The set of registers include:
- All 8-bit general purpose registers
- All 16-bit general purpose registers
- All 32-bit general purpose registers
- All 64-bit general purpose registers
- All segment registers
- All table registers
- All control registers, including extended control registers
- All debug registers
- All floating point registers, including the x87 FPU stack, MMX registers and
all SSE/AVX extensions registers
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

#include <cstdint>
#include <cstddef>

namespace virt86 {

// ----- Registers ------------------------------------------------------------

enum class Reg {
    // Segment registers
    CS, SS, DS, ES, FS, GS,
    LDTR, TR,

    // Table registers
    GDTR, IDTR,

    // 8-bit general purpose registers
    AL, AH, CL, CH, DL, DH, BL, BH,
    SPL, BPL, SIL, DIL,
    R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B,
    
    // 16-bit general purpose registers
    AX, CX, DX, BX,
    SP, BP, SI, DI,
    R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W,
    IP,
    FLAGS,

    // 32-bit general purpose registers
    EAX, ECX, EDX, EBX,
    ESP, EBP, ESI, EDI,
    R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,
    EIP,
    EFLAGS,

    // 64-bit general purpose registers
    RAX, RCX, RDX, RBX,
    RSP, RBP, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15,
    RIP,
    RFLAGS,

    // Control registers
    CR0, CR2, CR3, CR4, CR8,

    // Extended control registers
    EFER,
    XCR0,

    // Debug registers
    DR0, DR1, DR2, DR3, DR6, DR7,

    // Floating point registers
    ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,

    // MMX registers
    MM0, MM1, MM2, MM3, MM4, MM5, MM6, MM7,

    // XMM registers
    // SSE2
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    // VEX
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
    // EVEX
    XMM16, XMM17, XMM18, XMM19, XMM20, XMM21, XMM22, XMM23,
    XMM24, XMM25, XMM26, XMM27, XMM28, XMM29, XMM30, XMM31,

    // YMM registers
    // AVX
    YMM0, YMM1, YMM2, YMM3, YMM4, YMM5, YMM6, YMM7,
    // VEX
    YMM8, YMM9, YMM10, YMM11, YMM12, YMM13, YMM14, YMM15,
    // EVEX
    YMM16, YMM17, YMM18, YMM19, YMM20, YMM21, YMM22, YMM23,
    YMM24, YMM25, YMM26, YMM27, YMM28, YMM29, YMM30, YMM31,

    // ZMM registers
    // AVX-512
    ZMM0, ZMM1, ZMM2, ZMM3, ZMM4, ZMM5, ZMM6, ZMM7,
    // VEX
    ZMM8, ZMM9, ZMM10, ZMM11, ZMM12, ZMM13, ZMM14, ZMM15,
    // MVEX/EVEX
    ZMM16, ZMM17, ZMM18, ZMM19, ZMM20, ZMM21, ZMM22, ZMM23,
    ZMM24, ZMM25, ZMM26, ZMM27, ZMM28, ZMM29, ZMM30, ZMM31,

    // Aliases
    R0B = AL,
    R1B = CL,
    R2B = DL,
    R3B = BL,
    R4B = SPL,
    R5B = BPL,
    R6B = SIL,
    R7B = DIL,

    R0W = AX,
    R1W = CX,
    R2W = DX,
    R3W = BX,
    R4W = SP,
    R5W = BP,
    R6W = SI,
    R7W = DI,

    R0D = EAX,
    R1D = ECX,
    R2D = EDX,
    R3D = EBX,
    R4D = ESP,
    R5D = EBP,
    R6D = ESI,
    R7D = EDI,

    R0 = RAX,
    R1 = RCX,
    R2 = RDX,
    R3 = RBX,
    R4 = RSP,
    R5 = RBP,
    R6 = RSI,
    R7 = RDI,

};

template<class T>
inline T RegOffset(Reg base, Reg reg) noexcept {
    return static_cast<T>(reg) - static_cast<T>(base);
}

template<class T>
inline Reg RegAdd(Reg base, T offset) noexcept {
    return static_cast<Reg>(static_cast<T>(base) + offset);
}

inline bool RegBetween(Reg reg, Reg first, Reg last) noexcept {
    size_t iReg = static_cast<size_t>(reg);
    size_t iFirst = static_cast<size_t>(first);
    size_t iLast = static_cast<size_t>(last);
    return iReg >= iFirst && iReg <= iLast;
}

// ----- Register values ------------------------------------------------------

union RegValue {
    // General purpose 8-bit registers
    uint8_t u8;
    
    // General purpose 16-bit registers
    uint16_t u16;

    // General purpose 32-bit registers
    uint32_t u32;

    // General purpose 64-bit registers
    uint64_t u64;

    // Segment registers
    struct {
        uint16_t selector;
        uint64_t base;
        uint32_t limit;
        union {
            struct {
                uint16_t type : 4;
                uint16_t system : 1;
                uint16_t privilegeLevel : 2;
                uint16_t present : 1;
                uint16_t : 4;
                uint16_t available : 1;
                uint16_t longMode : 1;
                uint16_t defaultSize : 1;
                uint16_t granularity : 1;
            };
            uint16_t u16;
        } attributes;
    } segment;

    // Table registers
    struct {
        uint64_t base;
        uint16_t limit;
    } table;

    // ST(0) .. ST(7) registers
    struct {
        uint64_t significand;
        uint16_t exponentSign;
    } st;

    // MM0 .. MM7 registers
    union {
        int64_t i64[1];
        int32_t i32[2];
        int16_t i16[4];
        int8_t i8[8];
    } mm;

    // XMM0 .. XMM31 registers
    union {
        int64_t i64[2];
        int32_t i32[4];
        int16_t i16[8];
        int8_t i8[16];

        // NOTE: Assuming double = 8 bytes, float = 4 bytes
        double f64[2];
        float f32[4];
    } xmm;

    // YMM0 .. YMM31 registers
    union {
        int64_t i64[4];
        int32_t i32[8];
        int16_t i16[16];
        int8_t i8[32];

        // NOTE: Assuming double = 8 bytes, float = 4 bytes
        double f64[4];
        float f32[8];
    } ymm;

    // ZMM0 .. ZMM31 registers
    union {
        int64_t i64[8];
        int32_t i32[16];
        int16_t i16[32];
        int8_t i8[64];

        // NOTE: Assuming double = 8 bytes, float = 4 bytes
        double f64[8];
        float f32[16];
    } zmm;
    
    RegValue() = default;

    RegValue(const uint8_t& u8) noexcept : u64(u8) {}
    RegValue(const uint16_t& u16) noexcept : u64(u16) {}
    RegValue(const uint32_t& u32) noexcept : u64(u32) {}
    RegValue(const uint64_t& u64) noexcept : u64(u64) {}

    RegValue(const int8_t& i8) noexcept : u64(i8) {}
    RegValue(const int16_t& i16) noexcept : u64(i16) {}
    RegValue(const int32_t& i32) noexcept : u64(i32) {}
    RegValue(const int64_t& i64) noexcept : u64(i64) {}
};

// ----- Segment size ---------------------------------------------------------

enum class SegmentSize {
    Invalid,

    _16,
    _32,
    _64,
};

// ----- Register bits and masks ----------------------------------------------

// RFLAGS
constexpr uint64_t RFLAGS_CF      = (1u << 0);       // Carry flag
constexpr uint64_t RFLAGS_PF      = (1u << 2);       // Parity flag
constexpr uint64_t RFLAGS_AF      = (1u << 4);       // Adjust flag
constexpr uint64_t RFLAGS_ZF      = (1u << 6);       // Zero flag
constexpr uint64_t RFLAGS_SF      = (1u << 7);       // Sign flag
constexpr uint64_t RFLAGS_TF      = (1u << 8);       // Trap flag
constexpr uint64_t RFLAGS_IF      = (1u << 9);       // Interrupt flag
constexpr uint64_t RFLAGS_DF      = (1u << 10);      // Direction flag
constexpr uint64_t RFLAGS_OF      = (1u << 11);      // Overflow flag
constexpr uint64_t RFLAGS_IOPL    = (3u << 12);      // IO privilege level
constexpr uint64_t RFLAGS_IOPL_SHIFT = 12;
constexpr uint64_t RFLAGS_NT      = (1u << 14);      // Nested task
constexpr uint64_t RFLAGS_RF      = (1u << 16);      // Resume flag
constexpr uint64_t RFLAGS_VM      = (1u << 17);      // Virtual Mode
constexpr uint64_t RFLAGS_AC      = (1u << 18);      // Alignment check
constexpr uint64_t RFLAGS_VIF     = (1u << 19);      // Virtual Interrupt flag
constexpr uint64_t RFLAGS_VIP     = (1u << 20);      // Virtual Interrupt pending
constexpr uint64_t RFLAGS_ID      = (1u << 21);      // ID flag

// Segment type bits
constexpr uint8_t SEG_TYPE_ACCESSED   = (1 << 0);    // Accessed
constexpr uint8_t SEG_TYPE_READABLE   = (1 << 1);    // Readable (code segments only)
constexpr uint8_t SEG_TYPE_WRITABLE   = (1 << 1);    // Writable (data segments only)
constexpr uint8_t SEG_TYPE_CONFORMING = (1 << 2);    // Expand-down (data segments only)
constexpr uint8_t SEG_TYPE_EXPANDDOWN = (1 << 2);    // Conforming (code segments only)
constexpr uint8_t SEG_TYPE_CODE       = (1 << 3);    // Code segment if set, data segment if clear

// CR0 bits
constexpr uint64_t CR0_PE         = (1u << 0);       // Protection Enable
constexpr uint64_t CR0_MP         = (1u << 1);       // Monitor Co-Processor
constexpr uint64_t CR0_EM         = (1u << 2);       // Emulate Math Co-Processor
constexpr uint64_t CR0_TS         = (1u << 3);       // Task Switched
constexpr uint64_t CR0_ET         = (1u << 4);       // Extension Type (80387)
constexpr uint64_t CR0_NE         = (1u << 5);       // Numeric Error
constexpr uint64_t CR0_WP         = (1u << 16);      // Write Protect
constexpr uint64_t CR0_AM         = (1u << 18);      // Alignment Mask
constexpr uint64_t CR0_NW         = (1u << 29);      // Not Write-Through
constexpr uint64_t CR0_CD         = (1u << 30);      // Cache Disable
constexpr uint64_t CR0_PG         = (1u << 31);      // Paging

// CR4 bits
constexpr uint64_t CR4_VME        = (1u << 0);       // Virtual-8086 Mode Extensions
constexpr uint64_t CR4_PVI        = (1u << 1);       // Protected Mode Virtual Interrupts
constexpr uint64_t CR4_TSD        = (1u << 2);       // Time Stamp Disable (enabled only in ring 0)
constexpr uint64_t CR4_DE         = (1u << 3);       // Debugging Extensions
constexpr uint64_t CR4_PSE        = (1u << 4);       // Page Size Extensions
constexpr uint64_t CR4_PAE        = (1u << 5);       // Physical Address Extension
constexpr uint64_t CR4_MCE        = (1u << 6);       // Machine Check Exception
constexpr uint64_t CR4_PGE        = (1u << 7);       // Page Global Enable
constexpr uint64_t CR4_PCE        = (1u << 8);       // Performance Monitoring Counter Enable
constexpr uint64_t CR4_OSFXSR     = (1u << 9);       // OS support for FXSAVE and FXRSTOR instructions
constexpr uint64_t CR4_OSXMMEXCPT = (1u << 10);      // OS Support for unmasked SIMD floating point exceptions
constexpr uint64_t CR4_UMIP       = (1u << 11);      // User-Mode Instruction Prevention (SGDT, SIDT, SLDT, SMSW, and STR are disabled in user mode)
constexpr uint64_t CR4_VMXE       = (1u << 13);      // Virtual Machine Extensions Enable
constexpr uint64_t CR4_SMXE       = (1u << 14);      // Safer Mode Extensions Enable
constexpr uint64_t CR4_PCID       = (1u << 17);      // PCID Enable
constexpr uint64_t CR4_OSXSAVE    = (1u << 18);      // XSAVE and processor extended states enable
constexpr uint64_t CR4_SMEP       = (1u << 20);      // Supervisor Mode Executions Protection Enable
constexpr uint64_t CR4_SMAP       = (1u << 21);      // Supervisor Mode Access Protection Enable

// CR8 bits
constexpr uint64_t CR8_TPR        = (0xF << 0);      // Task-Priority Register

// EFER bits
constexpr uint64_t EFER_SCE       = (1u << 0);       // System Call Extensions
constexpr uint64_t EFER_LME       = (1u << 8);       // Long Mode Enable
constexpr uint64_t EFER_LMA       = (1u << 10);      // Long Mode Active
constexpr uint64_t EFER_NXE       = (1u << 11);      // No-Execute Enable
constexpr uint64_t EFER_SVME      = (1u << 12);      // Secure Virtual Machine Enable
constexpr uint64_t EFER_LMSLE     = (1u << 13);      // Long Mode Segment Limit Enable
constexpr uint64_t EFER_FFXSR     = (1u << 14);      // Fast FXSAVE/FXRSTOR
constexpr uint64_t EFER_TCE       = (1u << 15);      // Translation Cache Extension

// XCR0 bits
constexpr uint64_t XCR0_FP        = (1u << 0);       // X87 enabled / FPU/MMX state
constexpr uint64_t XCR0_SSE       = (1u << 1);       // SSE enabled / XSAVE feature set enabled for MXCSR and XMM regs
constexpr uint64_t XCR0_AVX       = (1u << 2);       // AVX enabled / XSAVE feature set enabled for YMM regs
constexpr uint64_t XCR0_BNDREG    = (1u << 3);       // MPX enabled / XSAVE feature set enabled for BND regs
constexpr uint64_t XCR0_BNDCSR    = (1u << 4);       // MPX enabled / XSAVE feature set enabled for BNDCFGU and BNDSTATUS regs
constexpr uint64_t XCR0_opmask    = (1u << 5);       // AVX-512 enabled / XSAVE feature set enabled for AVX opmask (k-mask) regs
constexpr uint64_t XCR0_ZMM_Hi256 = (1u << 6);       // AVX-512 enabled / XSAVE feature set enabled for upper-halves of the lower ZMM regs
constexpr uint64_t XCR0_Hi16_ZMM  = (1u << 7);       // AVX-512 enabled / XSAVE feature set enabled for the upper ZMM regs
constexpr uint64_t XCR0_PKRU      = (1u << 9);       // XSAVE feature set enabled for PKRU register (Protection Key Rights register for User pages)

// DR6 bits
constexpr uint64_t DR6_BP0        = (1u << 0);       // DR0 breakpoint hit
constexpr uint64_t DR6_BP1        = (1u << 1);       // DR1 breakpoint hit
constexpr uint64_t DR6_BP2        = (1u << 2);       // DR2 breakpoint hit
constexpr uint64_t DR6_BP3        = (1u << 3);       // DR3 breakpoint hit

// DR7 bits

// Local DR# breapoint enable
constexpr uint64_t DR7_LOCAL(uint8_t index) { return 1ull << (index << 1); }
// Global DR# breapoint enable
constexpr uint64_t DR7_GLOBAL(uint8_t index) { return 1ull << ((index << 1) + 1); }
// Conditions for DR#
constexpr uint64_t DR7_COND_SHIFT(uint8_t index) { return (static_cast<uint64_t>(index) << 2ull) + 16; }
constexpr uint64_t DR7_COND(uint8_t index) { return 0b11ull << DR7_COND_SHIFT(index); }
// Size of DR# breakpoint
constexpr uint64_t DR7_SIZE_SHIFT(uint8_t index) { return (static_cast<uint64_t>(index) << 2ull) + 18; }
constexpr uint64_t DR7_SIZE(uint8_t index) { return 0b11ull << DR7_SIZE_SHIFT(index); }

constexpr uint64_t DR7_COND_EXEC = 0b00u;        // Break on execution
constexpr uint64_t DR7_COND_WRITE = 0b01u;       // Write watchpoint
constexpr uint64_t DR7_COND_WIDTH8 = 0b10u;      // Breakpoint is 8 bytes wide
constexpr uint64_t DR7_COND_READWRITE = 0b11u;   // Read/write watchpoint

constexpr uint64_t DR7_SIZE_BYTE = 0b00u;        // 1 byte
constexpr uint64_t DR7_SIZE_WORD = 0b01u;        // 2 bytes
constexpr uint64_t DR7_SIZE_QWORD = 0b10u;       // 8 bytes
constexpr uint64_t DR7_SIZE_DWORD = 0b11u;       // 4 bytes

}
