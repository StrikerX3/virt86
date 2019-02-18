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

}
