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

static uint8_t getMaxGPABits() {
	int result[4];
	__cpuid(result, 0x80000008);
	uint32_t eax = result[0];

	uint8_t maxGPABits = (uint8_t)(eax >> 16);
	if (maxGPABits != 0) return maxGPABits;
	return (uint8_t)(eax);
}

#else
#  include <cpuid.h>

static uint8_t getMaxGPABits() {
	uint32_t eax, ebx, ecx, edx;
	__get_cpuid(0x80000008, &eax, &ebx, &ecx, &edx);

	uint8_t maxGPABits = (uint8_t)(eax >> 16);
	if (maxGPABits != 0) return maxGPABits;
	return (uint8_t)(eax);
}

#endif

HostInfo::HostInfo() noexcept {
}

HostInfo::GPA::GPA() noexcept
	: maxBits(getMaxGPABits())
	, maxAddress(1ull << maxBits)
	, mask(maxAddress - 1) {
}

}
