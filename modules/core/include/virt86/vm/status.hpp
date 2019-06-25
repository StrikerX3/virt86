/*
Defines status codes for various operations on virtual machines.
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

enum class MemoryMappingStatus {
    OK,                        // Memory mapped or unmapped successfully

    Unsupported,               // The operation is unsupported by the platform
    MisalignedHostMemory,      // The provided host memory block is misaligned
    MisalignedAddress,         // The provided memory region address is misaligned
    MisalignedSize,            // The provided memory region size is misaligned
    EmptyRange,                // Cannot map an empty memory range (size = 0)
    PartialUnmapUnsupported,   // Cannot unmap memory region because partial unmapping is not supported by the platform
    AlreadyAllocated,          // The host memory block has already been allocated
    InvalidFlags,              // Invalid flags were provided
    InvalidRange,              // An invalid (non-mapped or non-matching) memory range was provided
    Failed,                    // Failed to map or unmap the memory region
    OutOfBounds,               // Attempted to allocate GPA range beyond the host's limit
};

enum class DirtyPageTrackingStatus {
    OK,

    Unsupported,               // Dirty page tracking is unsupported by the hypervisor
    MisalignedAddress,         // The provided memory region address is misaligned
    MisalignedSize,            // The provided memory region size is misaligned
    MisalignedBitmap,          // The provided bitmap buffer is misaligned
    EmptyRange,                // Cannot query an empty memory range (size = 0)
    BitmapTooSmall,            // The provided bitmap buffer is too small
    NotEnabled,                // Dirty page tracking is not enabled for the specified guest memory range
    InvalidRange,              // An invalid memory range was specified
    Failed,                    // Failed to query dirty pages
};

}
