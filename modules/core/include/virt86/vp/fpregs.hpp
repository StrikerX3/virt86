/*
Defines data structures for FPU and SSE control registers.
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

namespace virt86 {

// Assert that float and double have the expected sizes in x86 platforms.
static_assert(sizeof(float) == 4, "float is not 4 bytes long");
static_assert(sizeof(double) == 8, "double is not 8 bytes long");

// FPU control registers
struct FPUControl {
    uint16_t cw;  // Control word
    uint16_t sw;  // Status word
    uint16_t tw;  // Tag word
    uint16_t op;  // Opcode bits
    struct {
        uint16_t cs;  // Code segment
        union {
            uint32_t ip;    // Instruction pointer (32-bit)
            uint64_t rip;  // Instruction pointer (64-bit)
        };
    };  // Instruction pointer
    struct {
        uint16_t ds;  // Data segment
        union {
            uint32_t dp;    // Data pointer (32-bit)
            uint64_t rdp;  // Data pointer (64-bit)
        };
    };  // Data pointer
};

// MXCSR register
union MXCSR {
    struct {
        uint32_t
            ie : 1,    // Invalid operation flag
            de : 1,    // Denormal flag
            ze : 1,    // Divide By Zero flag
            oe : 1,    // Overflow flag
            ue : 1,    // Underflow flag
            pe : 1,    // Precision flag
            daz : 1,   // Denormals Are Zero
            im : 1,    // Invalid operation mask
            dm : 1,    // Denormal mask
            zm : 1,    // Divide By Zero mask
            om : 1,    // Overflow mask
            um : 1,    // Underflow mask
            pm : 1,    // Precision mask
            rc : 2,    // Rounding control
                       //   0 = round to nearest (RN)
                       //   1 = round toward negative infinity (R-)
                       //   2 = round toward positive infinity (R+)
                       //   3 = rount toward zero (RZ)
            fz : 1,    // Flush To Zero
            : 16;
    };
    uint32_t u32;
};

// ST(#) register value
#pragma pack(push, 1)
struct STValue {
    uint64_t significand;
    uint16_t exponentSign;
};
#pragma pack(pop)
static_assert(sizeof(STValue) == 10);

union MMValue {
    int64_t i64[1];
    int32_t i32[2];
    int16_t i16[4];
    int8_t i8[8];
};
static_assert(sizeof(MMValue) == 8);

// XMM# register value
union XMMValue {
    int64_t i64[2];
    int32_t i32[4];
    int16_t i16[8];
    int8_t i8[16];

    double f64[2];
    float f32[4];
};
static_assert(sizeof(XMMValue) == 16);

// YMM#_H register value
union YMMHighValue {
    int64_t i64[2];
    int32_t i32[4];
    int16_t i16[8];
    int8_t i8[16];

    double f64[2];
    float f32[4];
};
static_assert(sizeof(YMMHighValue) == 16);

// YMM# register value
union YMMValue {
    struct {
        XMMValue low;
        YMMHighValue high;
    } partitions;

    int64_t i64[4];
    int32_t i32[8];
    int16_t i16[16];
    int8_t i8[32];

    double f64[4];
    float f32[8];
};
static_assert(sizeof(YMMValue) == 32);

// ZMM#_H register value
union ZMMHighValue {
    int64_t i64[4];
    int32_t i32[8];
    int16_t i16[16];
    int8_t i8[32];

    double f64[4];
    float f32[8];
};
static_assert(sizeof(ZMMHighValue) == 32);

// ZMM# register value
union ZMMValue {
    struct {
        YMMValue low;
        ZMMHighValue high;
    } partitions;

    int64_t i64[8];
    int32_t i32[16];
    int16_t i16[32];
    int8_t i8[64];

    double f64[8];
    float f32[16];
};
static_assert(sizeof(ZMMValue) == 64);

// FXSAVE area contents
struct FXSAVEArea {
    uint16_t fcw;
    uint16_t fsw;
    uint8_t ftw;
    uint8_t /*reserved*/: 8;
    uint16_t fop;
    union {
        struct {
            uint32_t fip;
            uint16_t fcs;
            uint16_t /*reserved*/ : 16;
        } ip32;
        struct {
            uint64_t fip;
        } ip64;
    };
    union {
        struct {
            uint32_t fdp;
            uint16_t fds;
            uint16_t /*reserved*/ : 16;
        } dp32;
        struct {
            uint64_t fdp;
        } dp64;
    };
    MXCSR mxcsr;
    MXCSR mxcsr_mask;
    union {
        STValue st;
        MMValue mm;
        uint8_t _padding[16];
    } st_mm[8];
    
    XMMValue xmm[16];
    uint8_t _reserved[48];
    uint8_t _unused[48];
};
static_assert(sizeof(FXSAVEArea) == 512, "FXSAVEArea struct is not 512 bytes long");

// XSAVE area contents
struct XSAVEArea {
    FXSAVEArea fxsave;

    // XSAVE header
    struct {
        union {
            struct {
                uint64_t
                    x87           : 1,
                    SSE           : 1,
                    AVX           : 1,
                    MPX_bndregs   : 1,
                    MPX_bndcsr    : 1,
                    AVX512_opmask : 1,
                    ZMM_Hi256     : 1,
                    Hi16_ZMM      : 1,
                    PT            : 1,
                    PKRU          : 1,
                                  : 3,
                    HDC           : 1;
            } data;
            uint64_t u64;
        } xstate_bv;
        union {
            struct {
                uint64_t
                    x87           : 1,
                    SSE           : 1,
                    AVX           : 1,
                    MPX_bndregs   : 1,
                    MPX_bndcsr    : 1,
                    AVX512_opmask : 1,
                    ZMM_Hi256     : 1,
                    Hi16_ZMM      : 1,
                    PT            : 1,
                    PKRU          : 1,
                                  : 3,
                    HDC           : 1,
                                  : 50,
                    format        : 1;
            } data;
            uint64_t u64;
        } xcomp_bv;
        uint8_t _reserved[48];
    } header;
};

struct XSAVE_AVX {
    YMMHighValue ymmHigh[16];
};
static_assert(sizeof(XSAVE_AVX) == 256);

struct XSAVE_MPX_BNDREGS {
    struct {
        uint64_t low;
        uint64_t high;
    } bnd[4];
};
static_assert(sizeof(XSAVE_MPX_BNDREGS) == 64);

struct XSAVE_MPX_BNDCSR {
    uint64_t BNDCFGU;
    uint64_t BNDSTATUS;
};
static_assert(sizeof(XSAVE_MPX_BNDCSR) == 16);

struct XSAVE_AVX512_Opmask {
    uint64_t k[8];
};
static_assert(sizeof(XSAVE_AVX512_Opmask) == 64);

struct XSAVE_AVX512_ZMM_Hi256 {
    ZMMHighValue zmmHigh[16];
};
static_assert(sizeof(XSAVE_AVX512_ZMM_Hi256) == 512);

struct XSAVE_AVX512_Hi16_ZMM {
    ZMMValue zmm[16];
};
static_assert(sizeof(XSAVE_AVX512_Hi16_ZMM) == 1024);

struct XSAVE_PT {
    uint64_t IA32_RTIT_CTL;
    uint64_t IA32_RTIT_OUTPUT_BASE;
    uint64_t IA32_RTIT_OUTPUT_MASK_PTRS;
    uint64_t IA32_RTIT_STATUS;
    uint64_t IA32_RTIT_CR3_MATCH;
    uint64_t IA32_RTIT_ADDR0_A;
    uint64_t IA32_RTIT_ADDR0_B;
    uint64_t IA32_RTIT_ADDR1_A;
    uint64_t IA32_RTIT_ADDR1_B;
};
static_assert(sizeof(XSAVE_PT) == 72);

struct XSAVE_PKRU {
    uint32_t pkru;
};
static_assert(sizeof(XSAVE_PKRU) == 4);

struct XSAVE_HDC {
    uint64_t IA32_PM_CTL1;
};
static_assert(sizeof(XSAVE_HDC) == 8);

}
