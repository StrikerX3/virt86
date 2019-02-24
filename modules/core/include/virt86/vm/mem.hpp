/*
Specifies data structures related to memory mapping.
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

// Flags for memory operations
enum class MemoryFlags : uint32_t {
    None = 0,
    Read = (1 << 0),
    Write = (1 << 1),
    Execute = (1 << 2),
    DirtyPageTracking = (1 << 3),
};

struct MemoryRegion {
    uint64_t baseAddress = 0;
    uint64_t size = 0;
    void *hostMemory = nullptr;

    MemoryRegion() = default;

    MemoryRegion(uint64_t baseAddress, uint64_t size, void *hostMemory) noexcept
        : baseAddress(baseAddress)
        , size(size)
        , hostMemory(hostMemory)
    {}
};

}

ENABLE_BITMASK_OPERATORS(virt86::MemoryFlags)
