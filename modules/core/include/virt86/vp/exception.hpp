/*
Defines the set of x86 exceptions that may be raised by the virtual processor.
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

#include "virt86/util/bitmask_enum.hpp"

#include <cstdint>

namespace virt86 {

enum class ExceptionCode : uint64_t {
    None = 0,

    DivideErrorFault = (1 << 0),
    DebugTrapOrFault = (1 << 1),
    BreakpointTrap = (1 << 3),
    OverflowTrap = (1 << 4),
    BoundRangeFault = (1 << 5),
    InvalidOpcodeFault = (1 << 6),
    DeviceNotAvailableFault = (1 << 7),
    DoubleFaultAbort = (1 << 8),
    InvalidTaskStateSegmentFault = (1 << 10),
    SegmentNotPresentFault = (1 << 11),
    StackFault = (1 << 12),
    GeneralProtectionFault = (1 << 13),
    PageFault = (1 << 14),
    FloatingPointErrorFault = (1 << 16),
    AlignmentCheckFault = (1 << 17),
    MachineCheckAbort = (1 << 18),
    SimdFloatingPointFault = (1 << 19),

    All = (1 << 20) - 1,
};

}

ENABLE_BITMASK_OPERATORS(virt86::ExceptionCode)
