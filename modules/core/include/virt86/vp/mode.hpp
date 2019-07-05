/*
Defines enumerations of CPU modes.
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

namespace virt86 {

/**
 * The CPU execution mode, based on CR0.PE, RFLAGS.VM and EFER.LMA.
 */
enum class CPUExecutionMode {
    Unknown,

    RealAddress,   // Real-address mode (CR0.PE = 0)
    Virtual8086,   // Virtual-8086 mode (CR0.PE = 1, RFLAGS.VM = 1)
    Protected,     // Protected mode    (CR0.PE = 1, RFLAGS.VM = 0, EFER.LMA = 0)
    IA32e,         // IA-32e mode       (CR0.PE = 1, RFLAGS.VM = 0, EFER.LMA = 1)
};

/**
 * The CPU paging mode, based on CR0.PG, CR4.PAE and EFER.LME.
 */
enum class CPUPagingMode {
    Unknown,
    Invalid,         // Invalid paging mode               (CR0.PG = 1, CR4.PAE = 0, EFER.LME = 1)

    None,            // No paging                         (CR0.PG = 0, CR4.PAE = 0, EFER.LME = 0)
    NoneLME,         // No paging (LME enabled)           (CR0.PG = 0, CR4.PAE = 0, EFER.LME = 1)
    NonePAE,         // No paging (PAE enabled)           (CR0.PG = 0, CR4.PAE = 1, EFER.LME = 0)
    NonePAEandLME,   // No paging (PAE and LME enabled)   (CR0.PG = 0, CR4.PAE = 1, EFER.LME = 1)
    ThirtyTwoBit,    // 32-bit paging                     (CR0.PG = 1, CR4.PAE = 0, EFER.LME = 0)
    PAE,             // PAE paging                        (CR0.PG = 1, CR4.PAE = 1, EFER.LME = 0)
    FourLevel,       // 4-level paging                    (CR0.PG = 1, CR4.PAE = 1, EFER.LME = 1)
};

}
