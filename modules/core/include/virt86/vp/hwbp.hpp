/*
Defines data structures for dealing with hardware breakpoints.
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

#include "regs.hpp"

namespace virt86 {

enum class HardwareBreakpointTrigger : uint8_t {
    Execution = DR7_COND_EXEC,            // Breakpoint triggered when code at the specified address is executed
    DataWrite = DR7_COND_WRITE,           // Breakpoint triggered when data is written to the specified address
    Width8 = DR7_COND_WIDTH8,             // Indicates that the breakpoint is 8 bytes wide
    DataReadWrite = DR7_COND_READWRITE,   // Breakpoint triggered when data is read from or written to the specified address
};

enum class HardwareBreakpointLength : uint8_t {
    Byte = DR7_SIZE_BYTE,     // Breakpoint is 1 byte wide
    Word = DR7_SIZE_WORD,     // Breakpoint is 2 bytes wide
    Qword = DR7_SIZE_QWORD,   // Breakpoint is 8 bytes wide
    Dword = DR7_SIZE_DWORD,   // Breakpoint is 4 bytes wide
};

struct HardwareBreakpoints {
    struct {
        uint64_t address;
        bool localEnable;
        bool globalEnable;
        HardwareBreakpointTrigger trigger;
        HardwareBreakpointLength length;
    } bp[4];
};

}
