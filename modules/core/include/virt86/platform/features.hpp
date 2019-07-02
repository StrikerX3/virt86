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
 * Floating point extensions supported by the hypervisor and the host CPU.
 */
enum class FloatingPointExtension : int64_t {
    /**
     * No floating point extension supported.
     * XMM/YMM/ZMM registers are unavailable.
     */
    None = 0,

    /**
     * Supports MMX extensions, which includes the following set of registers:
     * - MM0 to MM7
     */
    MMX = (1 << 0),

    /**
     * Supports SSE extensions, which includes the following set of registers:
     * - XMM0 to XMM7
     */
    SSE = (1 << 1),

    /**
     * Supports SSE2 extensions, which adds the following set of registers:
     * - XMM0 to XMM15 (in IA-32e mode)
     */
    SSE2 = (1 << 2),

    /**
     * Supports SSE3 extensions.
     */
    SSE3 = (1 << 3),

    /**
     * Supports SSSE3 extensions.
     */
    SSSE3 = (1 << 4),

    /**
     * Supports SSE4.1 extensions.
     */
    SSE4_1 = (1 << 5),

    /**
     * Supports SSE4.2 extensions.
     */
    SSE4_2 = (1 << 6),

    /**
     * Supports SSE4a extensions. AMD CPUs only.
     */
    SSE4a = (1 << 7),

    /**
     * Supports XOP (extended operations). AMD CPUs only.
     */
    XOP = (1 << 8),

    /**
     * Supports 16-bit Floating-Point conversion instructions.
     * Also known as CVT16.
     */
    F16C = (1 << 9),

    /**
     * Supports 4-operand fused multiply-add instructions.
     * AMD CPUs only (so far).
     */
    FMA4 = (1 << 10),

    /**
     * Supports AVX extensions, which adds the following set of registers:
     * - YMM0 to YMM7
     * - YMM0 to YMM15 (in IA-32e mode)
     * AVX also adds support for the VEX prefix, allowing SSE instructions to
     * access YMM registers and use a third operand for parity with AVX.
     */
    AVX = (1 << 11),

    /**
     * Supports 3-operand fused multiply-add instructions.
     */
    FMA3 = (1 << 12),

    /**
     * Supports AVX2 extensions.
     */
    AVX2 = (1 << 13),

    /**
     * Supports AVX-512 foundation extensions, which adds the following set of
     * registers:
     * - ZMM0 to ZMM7
     * - XMM0 to XMM31 (in IA-32e mode)
     * - YMM0 to YMM31 (in IA-32e mode)
     * - ZMM0 to ZMM31 (in IA-32e mode)
     * AVX-512 also adds support for the EVEX prefix, allowing SSE and AVX
     * instructions to access ZMM registers.
     */
    AVX512F = (1 << 14),

    /**
     * Supports AVX-512 Double and Quadword instructions.
     */
    AVX512DQ = (1 << 15),

    /**
     * Supports AVX-512 Integer Fused Multiply-Add instructions.
     */
    AVX512IFMA = (1 << 16),

    /**
     * Supports AVX-512 Prefetch instructions.
     */
    AVX512PF = (1 << 17),

    /**
     * Supports AVX-512 Exponential and Reciprocal instructions.
     */
    AVX512ER = (1 << 18),

    /**
     * Supports AVX-512 Conflict Detection instructions.
     */
    AVX512CD = (1 << 19),

    /**
     * Supports AVX-512 Byte and Word instructions.
     */
    AVX512BW = (1 << 20),

    /**
     * Supports AVX-512 Vector Length extensions.
     */
    AVX512VL = (1 << 21),

    /**
     * Supports AVX-512 Vector Bit Manipulation instructions.
     */
    AVX512VBMI = (1 << 22),

    /**
     * Supports AVX-512 Vector Bit Manipulation instructions, version 2.
     */
    AVX512VBMI2 = (1 << 23),

    /**
     * Supports AVX-512 Galois Field New Instructions.
     */
    AVX512GFNI = (1 << 24),

    /**
     * Supports AVX-512 Vector AES instructions.
     */
    AVX512VAES = (1 << 25),

    /**
     * Supports AVX-512 Vector Neural Network instructions.
     */
    AVX512VNNI = (1 << 26),

    /**
     * Supports AVX-512 Bit Algorithms.
     */
    AVX512BITALG = (1 << 27),

    /**
     * Supports AVX-512 Vector Population Count Doubleword and Quadword instructions.
     */
    AVX512VPOPCNTDQ = (1 << 28),

    /**
     * Supports AVX-512 Vector Neural Network Instructions Word Variable Precision instructions.
     */
    AVX512QVNNIW = (1 << 29),

    /**
     * Supports AVX-512 Fused Multiply Accumulation Packed Single Precision instructions.
     */
    AVX512QFMA = (1 << 30),
     
    /**
     * Supports the FXSAVE and FXRSTOR instructions.
     */
    FXSAVE = (1 << 31),
   
    /**
     * Supports the XSAVE and XRSTOR instructions.
     */
    XSAVE = (1ull << 32),
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
    TSCAccess = (1 << 3),     // Supports VM exit on TSC access (RDTSC, RDTSCP, RDMSR, WRMSR)
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
     * Guest physical address limits.
     */
    struct {
        /**
         * The number of bits in a valid guest physical address.
         * Based on the host's capabilities and the platform's limits.
         */
        uint8_t maxBits;

        /**
         * The maximum guest physical address supported by the platform.
         * Based on the host's capabilities and the platform's limits.
         */
        uint64_t maxAddress;

        /**
         * A precomputed mask for guest physical addresses.
         * If any bit is set outside of the mask, the address is unsupported.
         */
        uint64_t mask;
    } guestPhysicalAddress;

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

    /**
     * Guest TSC scaling and virtual TSC offset is supported.
     */
    bool guestTSCScaling = false;
};

}

ENABLE_BITMASK_OPERATORS(virt86::FloatingPointExtension)
ENABLE_BITMASK_OPERATORS(virt86::ExtendedControlRegister)
ENABLE_BITMASK_OPERATORS(virt86::ExtendedVMExit)
