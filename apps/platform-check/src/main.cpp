/*
Entry point of the Platform Checker application.

The Platform Checker lists the features of all platforms available on the
user's system. The list of available platforms depends on the operating
system and, in the case of Windows Hypervisor Platform, the presence of
Windows 10 SDK 10.0.17134.0 or later when building the project.
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
#include "virt86/virt86.hpp"

#if defined(_WIN32)
#  include <Windows.h>
#elif defined(__linux__)
#  include <stdlib.h>
#elif defined(__APPLE__)
#  include <stdlib.h>
#endif

#include <cstdio>

using namespace virt86;

template<class T, size_t N>
constexpr size_t array_size(T(&)[N]) {
    return N;
}

int main() {
    printf("virt86 Platform Checker 1.0.1\n");  // TODO: externalize version string
    printf("Copyright (c) 2019 Ivan Roberto de Oliveira\n");
    printf("\n");
    
    if constexpr (array_size(PlatformFactories) == 0) {
        printf("No virtualization platforms are available on this system\n");
        return -1;
    }

    printf("Virtualization platforms available on this system:\n");
    printf("\n");
    for (size_t i = 0; i < array_size(PlatformFactories); i++) {
        const auto& platform = PlatformFactories[i]();

        printf("%s - ", platform.GetName().c_str());

        const auto initStatus = platform.GetInitStatus();
        switch (initStatus) {
        case PlatformInitStatus::OK: printf("Available\n"); break;
        case PlatformInitStatus::Failed: printf("Initialization failed\n"); break;
        case PlatformInitStatus::Uninitialized: printf("Uninitialized\n"); break;
        case PlatformInitStatus::Unavailable: printf("Unavailable\n"); break;
        case PlatformInitStatus::Unsupported: printf("Unsupported\n"); break;
        default: printf("Unknown initialization status\n");
        }

        if (initStatus != PlatformInitStatus::OK) {
            printf("\n");
            continue;
        }

        const auto& features = platform.GetFeatures();
        printf("  Features:\n");
        printf("    Maximum number of VCPUs: %u per VM, %u global\n", features.maxProcessorsPerVM, features.maxProcessorsGlobal);
        printf("    Unrestricted guest: %s\n", (features.unrestrictedGuest) ? "supported" : "unsuported");
        printf("    Extended Page Tables: %s\n", (features.extendedPageTables) ? "supported" : "unsuported");
        printf("    Guest debugging: %s\n", (features.guestDebugging) ? "available" : "unavailable");
        printf("    Memory protection: %s\n", (features.guestMemoryProtection) ? "available" : "unavailable");
        printf("    Dirty page tracking: %s\n", (features.dirtyPageTracking) ? "available" : "unavailable");
        printf("    Partial dirty bitmap: %s\n", (features.partialDirtyBitmap) ? "supported" : "unsupported");
        printf("    Large memory allocation: %s\n", (features.largeMemoryAllocation) ? "supported" : "unsuported");
        printf("    Memory aliasing: %s\n", (features.memoryAliasing) ? "supported" : "unsuported");
        printf("    Memory unmapping: %s\n", (features.memoryUnmapping) ? "supported" : "unsuported");
        printf("    Partial unmapping: %s\n", (features.partialUnmapping) ? "supported" : "unsuported");
        printf("    Partial MMIO instructions: %s\n", (features.partialMMIOInstructions) ? "yes" : "no");
        printf("    Custom CPUID results: %s\n", (features.customCPUIDs) ? "supported" : "unsupported");
        if (features.customCPUIDs && features.supportedCustomCPUIDs.size() > 0) {
            printf("       Function        EAX         EBX         ECX         EDX\n");
            for (auto it = features.supportedCustomCPUIDs.cbegin(); it != features.supportedCustomCPUIDs.cend(); it++) {
                printf("      0x%08x = 0x%08x  0x%08x  0x%08x  0x%08x\n", it->function, it->eax, it->ebx, it->ecx, it->edx);
            }
        }
        printf("    Floating point extensions:");
        const auto fpExts = BitmaskEnum(features.floatingPointExtensions);
        if (!fpExts) printf(" None");
        else {
            if (fpExts.AnyOf(FloatingPointExtension::SSE2)) printf(" SSE2");
            if (fpExts.AnyOf(FloatingPointExtension::AVX)) printf(" AVX");
            if (fpExts.AnyOf(FloatingPointExtension::VEX)) printf(" VEX");
            if (fpExts.AnyOf(FloatingPointExtension::MVEX)) printf(" MVEX");
            if (fpExts.AnyOf(FloatingPointExtension::EVEX)) printf(" EVEX");
        }
        printf("\n");
        printf("    Extended control registers:");
        const auto extCRs = BitmaskEnum(features.extendedControlRegisters);
        if (!extCRs) printf(" None");
        else {
            if (extCRs.AnyOf(ExtendedControlRegister::CR8)) printf(" CR8");
            if (extCRs.AnyOf(ExtendedControlRegister::XCR0)) printf(" XCR0");
            if (extCRs.AnyOf(ExtendedControlRegister::MXCSRMask)) printf(" MXCSR_MASK");
        }
        printf("\n");
        printf("    Extended VM exits:");
        const auto extVMExits = BitmaskEnum(features.extendedVMExits);
        if (!extVMExits) printf(" None");
        else {
            if (extVMExits.AnyOf(ExtendedVMExit::CPUID)) printf(" CPUID");
            if (extVMExits.AnyOf(ExtendedVMExit::MSRAccess)) printf(" MSRAccess");
            if (extVMExits.AnyOf(ExtendedVMExit::Exception)) printf(" Exception");
        }
        printf("\n");
        printf("    Exception exits:");
        const auto excptExits = BitmaskEnum(features.exceptionExits);
        if (!excptExits) printf(" None");
        else {
            if (excptExits.AnyOf(ExceptionCode::DivideErrorFault)) printf(" DivideErrorFault");
            if (excptExits.AnyOf(ExceptionCode::DebugTrapOrFault)) printf(" DebugTrapOrFault");
            if (excptExits.AnyOf(ExceptionCode::BreakpointTrap)) printf(" BreakpointTrap");
            if (excptExits.AnyOf(ExceptionCode::OverflowTrap)) printf(" OverflowTrap");
            if (excptExits.AnyOf(ExceptionCode::BoundRangeFault)) printf(" BoundRangeFault");
            if (excptExits.AnyOf(ExceptionCode::InvalidOpcodeFault)) printf(" InvalidOpcodeFault");
            if (excptExits.AnyOf(ExceptionCode::DeviceNotAvailableFault)) printf(" DeviceNotAvailableFault");
            if (excptExits.AnyOf(ExceptionCode::DoubleFaultAbort)) printf(" DoubleFaultAbort");
            if (excptExits.AnyOf(ExceptionCode::InvalidTaskStateSegmentFault)) printf(" InvalidTaskStateSegmentFault");
            if (excptExits.AnyOf(ExceptionCode::SegmentNotPresentFault)) printf(" SegmentNotPresentFault");
            if (excptExits.AnyOf(ExceptionCode::StackFault)) printf(" StackFault");
            if (excptExits.AnyOf(ExceptionCode::GeneralProtectionFault)) printf(" GeneralProtectionFault");
            if (excptExits.AnyOf(ExceptionCode::PageFault)) printf(" PageFault");
            if (excptExits.AnyOf(ExceptionCode::FloatingPointErrorFault)) printf(" FloatingPointErrorFault");
            if (excptExits.AnyOf(ExceptionCode::AlignmentCheckFault)) printf(" AlignmentCheckFault");
            if (excptExits.AnyOf(ExceptionCode::MachineCheckAbort)) printf(" MachineCheckAbort");
            if (excptExits.AnyOf(ExceptionCode::SimdFloatingPointFault)) printf(" SimdFloatingPointFault");
        }
        printf("\n\n");
    }

    return 0;
}
