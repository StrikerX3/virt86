/*
Contains definitions of the optional features exposed by a hypervisor platform.
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
#include "virt86/vp/exception.hpp"
#include "virt86/vp/cpuid.hpp"

#include <vector>

namespace virt86 {

/**
 * Floating point extensions supported by the hypervisor.
 */
enum class FloatingPointExtension {
    /**
     * No floating point extension supported.
     * XMM/YMM/ZMM registers are unavailable.
     */
    None = 0,

    /**
     * Supports SSE2 extensions, which includes the following set of registers:
     * - XMM0 to XMM7
     */
    SSE2 = (1 << 0),

    /**
     * Supports AVX extensions, which includes the following set of registers:
     * - YMM0 to YMM7
     */
    AVX = (1 << 1),
    
    /**
     * Supports AVX-512 extensions, which includes the following set of
     * registers:
     * - ZMM0 to ZMM7
     */
    AVX512 = (1 << 2),

    /**
     * Supports VEX vector extensions, which includes the following sets of
     * registers:
     * - XMM8 to XMM15
     * - YMM8 to YMM15
     * - ZMM8 to ZMM15
     */
    VEX = (1 << 3),

    /**
     * Supports MVEX vector extensions, which includes the following set of
     * registers:
     * - ZMM16 to ZMM31
     */
    MVEX = (1 << 4),

    /**
     * Supports EVEX vector extensions, which includes the following sets of
     * registers:
     * - XMM16 to XMM31
     * - YMM16 to YMM31
     * - ZMM16 to ZMM31
     */
    EVEX = (1 << 5),
};

/**
 * Extended control registers exposed by the hypervisor.
 */
enum class ExtendedControlRegister {
    None = 0,   // No extended control registers exposed
    
    XCR0 = (1 << 0),
    CR8 = (1 << 1),
    MXCSRMask = (1 << 2),
};

/**
 * Additional VM exit reasons supported by the hypervisor.
 */
enum class ExtendedVMExit {
    None = 0,                 // No extended VM exits supported
    
    CPUID = (1 << 0),         // Supports VM exit due to the CPUID instruction
    MSRAccess = (1 << 1),     // Supports VM exit on MSR access
    Exception = (1 << 2),     // Supports VM exit on CPU exception
};

/**
 * Specifies the features supported by a virtualization platform.
 */
struct PlatformFeatures {
    /**
     * The maximum number of virtual processors supported by the hypervisor.
     */
    uint32_t maxProcessorsGlobal = 0;

    /**
     * The maximum number of virtual processors supported per VM.
     */
    uint32_t maxProcessorsPerVM = 0;

    /**
     * Unrestricted guests are supported.
     */
    bool unrestrictedGuest = false;

    /**
     * Extended Page Tables (EPT) are supported.
     */
    bool extendedPageTables = false;

    /**
     * Guest debugging is available.
     */
    bool guestDebugging = false;

    /**
     * Guest memory protection is available.
     */
    bool guestMemoryProtection = false;

    /**
     * Dirty page tracking is available.
     */
    bool dirtyPageTracking = false;

    /**
     * Hypervisor allows users to read the dirty bitmap of a subregion of
     * mapped memory. When false, only full memory regions may be queried, if
     * the platform supports dirty page tracking.
     */
    bool partialDirtyBitmap = false;

    /**
     * Allows mapping memory regions larger than 4 GiB.
     */
    bool largeMemoryAllocation = false;

    /**
     * Guest memory aliasing (mapping one host memory range to multiple guest
     * memory ranges) is supported.
     */
    bool memoryAliasing = false;

    /**
     * Memory unmapping is supported.
     */
    bool memoryUnmapping = false;

    /**
     * Partial guest memory unmapping is supported.
     */
    bool partialUnmapping = false;

    /**
     * The instruction emulator used by the platform performs one MMIO
     * operation per execution, which causes more complex instructions to
     * require more than one execution to complete.
     */
    bool partialMMIOInstructions = false;

    /**
     * Floating point extensions supported by the hypervisor.
     */
    FloatingPointExtension floatingPointExtensions = FloatingPointExtension::None;

    /**
     * Extended control registers exposed by the hypervisor.
     */
    ExtendedControlRegister extendedControlRegisters = ExtendedControlRegister::None;

    /**
     * Additional VM exit reasons supported by the hypervisor.
     */
    ExtendedVMExit extendedVMExits = ExtendedVMExit::None;

    /**
     * Types of exception exits supported by the hypervisor.
     */
    ExceptionCode exceptionExits = ExceptionCode::None;

    /**
     * Hypervisor allows custom CPUID results to be configured.
     */
    bool customCPUIDs = false;

    /**
     * Supported CPUID codes and their default responses. Only valid if custom
     * CPUIDs are supported. Not all platforms fill this list.
     */
    std::vector<CPUIDResult> supportedCustomCPUIDs;
};

}

ENABLE_BITMASK_OPERATORS(virt86::FloatingPointExtension)
ENABLE_BITMASK_OPERATORS(virt86::ExtendedControlRegister)
ENABLE_BITMASK_OPERATORS(virt86::ExtendedVMExit)
