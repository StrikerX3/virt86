/*
 * Copyright (c) 2011 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HAX_TYPES_H_
#define HAX_TYPES_H_

/* Detect architecture */
// x86 (32-bit)
#if defined(__i386__) || defined(_M_IX86)
#define HAX_ARCH_X86_32
#define ASMCALL __cdecl
// x86 (64-bit)
#elif defined(__x86_64__) || defined(_M_X64)
#define HAX_ARCH_X86_64
#define ASMCALL
#else
#error "Unsupported architecture"
#endif

/* Detect compiler */
// Clang
#if defined(__clang__)
#define HAX_COMPILER_CLANG
#define PACKED     __attribute__ ((packed))
#define ALIGNED(x) __attribute__ ((aligned(x)))
// GCC
#elif defined(__GNUC__)
#define HAX_COMPILER_GCC
#define PACKED     __attribute__ ((packed))
#define ALIGNED(x) __attribute__ ((aligned(x)))
#define __cdecl    __attribute__ ((__cdecl__,regparm(0)))
#define __stdcall  __attribute__ ((__stdcall__))
// MSVC
#elif defined(_MSC_VER)
#define HAX_COMPILER_MSVC
// FIXME: MSVC doesn't have a simple equivalent for PACKED.
//        Instead, The corresponding #pragma directives are added manually.
#define PACKED     
#define ALIGNED(x) __declspec(align(x))
#else
#error "Unsupported compiler"
#endif

/* Detect platform */
#ifndef HAX_TESTS // Prevent kernel-only exports from reaching userland code
// MacOS
#if defined(__MACH__)
#define HAX_PLATFORM_DARWIN
#include "darwin/hax_types_mac.hpp"
// Linux
#elif defined(__linux__)
#define HAX_PLATFORM_LINUX
#include "linux/hax_types_linux.hpp"
// Windows
#elif defined(_WIN32)
#define HAX_PLATFORM_WINDOWS
#include "windows/hax_types_windows.hpp"
#else
#error "Unsupported platform"
#endif
#else // !HAX_TESTS
#include <stdint.h>
#endif // HAX_TESTS

#define HAX_PAGE_SIZE  4096
#define HAX_PAGE_SHIFT 12
#define HAX_PAGE_MASK  0xfff

/* Common typedef for all platforms */
typedef uint64_t hax_pa_t;
typedef uint64_t hax_pfn_t;
typedef uint64_t hax_paddr_t;
typedef uint64_t hax_vaddr_t;

enum exit_status {
    HAX_EXIT_IO = 1,
    HAX_EXIT_MMIO,
    HAX_EXIT_REALMODE,
    HAX_EXIT_INTERRUPT,
    HAX_EXIT_UNKNOWN,
    HAX_EXIT_HLT,
    HAX_EXIT_STATECHANGE,
    HAX_EXIT_PAUSED,
    HAX_EXIT_FAST_MMIO,
    HAX_EXIT_PAGEFAULT,
    HAX_EXIT_DEBUG
};

enum {
    VMX_EXIT_INT_EXCEPTION_NMI       =  0, // An SW interrupt, exception or NMI has occurred
    VMX_EXIT_EXT_INTERRUPT           =  1, // An external interrupt has occurred
    VMX_EXIT_TRIPLE_FAULT            =  2, // Triple fault occurred
    VMX_EXIT_INIT_EVENT              =  3, // INIT signal arrived
    VMX_EXIT_SIPI_EVENT              =  4, // SIPI signal arrived
    VMX_EXIT_SMI_IO_EVENT            =  5,
    VMX_EXIT_SMI_OTHER_EVENT         =  6,
    VMX_EXIT_PENDING_INTERRUPT       =  7,
    VMX_EXIT_PENDING_NMI             =  8,
    VMX_EXIT_TASK_SWITCH             =  9, // Guest attempted a task switch
    VMX_EXIT_CPUID                   = 10, // Guest executed CPUID instruction
    VMX_EXIT_GETSEC                  = 11, // Guest executed GETSEC instruction
    VMX_EXIT_HLT                     = 12, // Guest executed HLT instruction
    VMX_EXIT_INVD                    = 13, // Guest executed INVD instruction
    VMX_EXIT_INVLPG                  = 14, // Guest executed INVLPG instruction
    VMX_EXIT_RDPMC                   = 15, // Guest executed RDPMC instruction
    VMX_EXIT_RDTSC                   = 16, // Guest executed RDTSC instruction
    VMX_EXIT_RSM                     = 17, // Guest executed RSM instruction in SMM
    VMX_EXIT_VMCALL                  = 18,
    VMX_EXIT_VMCLEAR                 = 19,
    VMX_EXIT_VMLAUNCH                = 20,
    VMX_EXIT_VMPTRLD                 = 21,
    VMX_EXIT_VMPTRST                 = 22,
    VMX_EXIT_VMREAD                  = 23,
    VMX_EXIT_VMRESUME                = 24,
    VMX_EXIT_VMWRITE                 = 25,
    VMX_EXIT_VMXOFF                  = 26,
    VMX_EXIT_VMXON                   = 27,
    VMX_EXIT_CR_ACCESS               = 28, // Guest accessed a control register
    VMX_EXIT_DR_ACCESS               = 29, // Guest attempted access to debug register
    VMX_EXIT_IO                      = 30, // Guest attempted I/O
    VMX_EXIT_MSR_READ                = 31, // Guest attempted to read an MSR
    VMX_EXIT_MSR_WRITE               = 32, // Guest attempted to write an MSR
    VMX_EXIT_FAILED_VMENTER_GS       = 33, // VMENTER failed due to guest state
    VMX_EXIT_FAILED_VMENTER_MSR      = 34, // VMENTER failed due to MSR loading
    VMX_EXIT_MWAIT                   = 36,
    VMX_EXIT_MTF_EXIT                = 37,
    VMX_EXIT_MONITOR                 = 39,
    VMX_EXIT_PAUSE                   = 40,
    VMX_EXIT_MACHINE_CHECK           = 41,
    VMX_EXIT_TPR_BELOW_THRESHOLD     = 43,
    VMX_EXIT_APIC_ACCESS             = 44,
    VMX_EXIT_GDT_IDT_ACCESS          = 46,
    VMX_EXIT_LDT_TR_ACCESS           = 47,
    VMX_EXIT_EPT_VIOLATION           = 48,
    VMX_EXIT_EPT_MISCONFIG           = 49,
    VMX_EXIT_INVEPT                  = 50,
    VMX_EXIT_RDTSCP                  = 51,
    VMX_EXIT_VMX_TIMER_EXIT          = 52,
    VMX_EXIT_INVVPID                 = 53,
    VMX_EXIT_WBINVD                  = 54,
    VMX_EXIT_XSETBV                  = 55,
    VMX_EXIT_APIC_WRITE              = 56,
    VMX_EXIT_RDRAND                  = 57,
    VMX_EXIT_INVPCID                 = 58,
    VMX_EXIT_VMFUNC                  = 59,
    VMX_EXIT_ENCLS                   = 60,
    VMX_EXIT_RDSEED                  = 61,
    VMX_EXIT_XSAVES                  = 63,
    VMX_EXIT_XRSTORS                 = 64
};

#endif  // HAX_TYPES_H_
