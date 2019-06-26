/*
Host information.
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
#include "virt86/util/host_info.hpp"

namespace virt86 {

// ----- Compute number of bits in a guest physical address -------------------
// (1) CPUID 8000_0008h.EAX[23..16] = bits in a GPA
// (2) CPUID 8000_0008h.EAX[7..0]   = bits in a physical address
// GPA bits = (1) if non-zero, (2) otherwise

#if defined(_MSC_VER)
#  include <intrin.h>

static void cpuid(int leaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    int output[4];
    __cpuid(output, leaf);
    if (eax != nullptr) *eax = (unsigned int)output[0];
    if (ebx != nullptr) *ebx = (unsigned int)output[1];
    if (ecx != nullptr) *ecx = (unsigned int)output[2];
    if (edx != nullptr) *edx = (unsigned int)output[3];
}

#else
#  include <cpuid.h>

static void cpuid(int leaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    unsigned int dummy;
    if (eax == nullptr) eax = &dummy;
    if (ebx == nullptr) ebx = &dummy;
    if (ecx == nullptr) ecx = &dummy;
    if (edx == nullptr) edx = &dummy;
    __get_cpuid(leaf, eax, ebx, ecx, edx);
}

#endif

static uint8_t getMaxGPABits() {
    uint32_t eax;
    cpuid(0x80000008, &eax, nullptr, nullptr, nullptr);

    uint8_t maxGPABits = (uint8_t)(eax >> 16);
    if (maxGPABits != 0) return maxGPABits;
    return (uint8_t)(eax);
}

static FloatingPointExtension getFPExts() {
    FloatingPointExtension result = FloatingPointExtension::None;

    uint32_t ecx1, edx1;
    cpuid(0x1, nullptr, nullptr, &ecx1, &edx1);
    uint32_t ebx7, ecx7, edx7;
    cpuid(0x7, nullptr, &ebx7, &ecx7, &edx7);
    uint32_t ecx81;
    cpuid(0x80000001, nullptr, nullptr, &ecx81, nullptr);

    if (edx1 & (1 << 23)) result |= FloatingPointExtension::MMX;
    if (edx1 & (1 << 25)) result |= FloatingPointExtension::SSE;
    if (edx1 & (1 << 26)) result |= FloatingPointExtension::SSE2;
    if (ecx1 & (1 << 0)) result |= FloatingPointExtension::SSE3;
    if (ecx1 & (1 << 9)) result |= FloatingPointExtension::SSSE3;
    if (ecx1 & (1 << 19)) result |= FloatingPointExtension::SSE4_1;
    if (ecx1 & (1 << 20)) result |= FloatingPointExtension::SSE4_2;
    if (ecx81 & (1 << 6)) result |= FloatingPointExtension::SSE4a;
    if (ecx81 & (1 << 11)) result |= FloatingPointExtension::XOP;
    if (ecx1 & (1 << 29)) result |= FloatingPointExtension::F16C;
    if (ecx81 & (1 << 16)) result |= FloatingPointExtension::FMA4;
    if (ecx1 & (1 << 28)) result |= FloatingPointExtension::AVX;
    if (ecx1 & (1 << 12)) result |= FloatingPointExtension::FMA3;
    if (ebx7 & (1 << 5)) result |= FloatingPointExtension::AVX2;
    if (ebx7 & (1 << 16)) result |= FloatingPointExtension::AVX512F;
    if (ebx7 & (1 << 17)) result |= FloatingPointExtension::AVX512DQ;
    if (ebx7 & (1 << 21)) result |= FloatingPointExtension::AVX512IFMA;
    if (ebx7 & (1 << 26)) result |= FloatingPointExtension::AVX512PF;
    if (ebx7 & (1 << 27)) result |= FloatingPointExtension::AVX512ER;
    if (ebx7 & (1 << 28)) result |= FloatingPointExtension::AVX512CD;
    if (ebx7 & (1 << 30)) result |= FloatingPointExtension::AVX512BW;
    if (ebx7 & (1 << 31)) result |= FloatingPointExtension::AVX512VL;
    if (ecx7 & (1 << 1)) result |= FloatingPointExtension::AVX512VBMI;
    if (ecx7 & (1 << 6)) result |= FloatingPointExtension::AVX512VBMI2;
    if (ecx7 & (1 << 8)) result |= FloatingPointExtension::AVX512GFNI;
    if (ecx7 & (1 << 9)) result |= FloatingPointExtension::AVX512VAES;
    if (ecx7 & (1 << 11)) result |= FloatingPointExtension::AVX512VNNI;
    if (ecx7 & (1 << 12)) result |= FloatingPointExtension::AVX512BITALG;
    if (ecx7 & (1 << 14)) result |= FloatingPointExtension::AVX512VPOPCNTDQ;
    if (edx7 & (1 << 2)) result |= FloatingPointExtension::AVX512QVNNIW;
    if (edx7 & (1 << 3)) result |= FloatingPointExtension::AVX512QFMA;
    if (edx1 & (1 << 24)) result |= FloatingPointExtension::FXSAVE;
    if (ecx1 & (1 << 26)) result |= FloatingPointExtension::XSAVE;

    return result;
}

HostInfo::HostInfo() noexcept
    : floatingPointExtensions(getFPExts()) {
}

HostInfo::GPA::GPA() noexcept
    : maxBits(getMaxGPABits())
    , maxAddress(1ull << maxBits)
    , mask(maxAddress - 1) {
}

}
