/*
Defines status codes for various operations on a virtual machine.
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

enum class VPExecutionStatus {
    OK,                  // Virtual processor executed successfully

    Failed,              // Virtual processor failed to execute due to an unspecified error
    Unsupported,         // Virtualization platform does not support the operation
};

enum class VPOperationStatus {
    OK,                  // Operation completed succesfully

    Failed,              // The operation failed
    InvalidArguments,    // Invalid arguments (such as null pointers) were specified
    InvalidSelector,     // An invalid selector was specified
    InvalidRegister,     // An invalid register was specified
    BreakpointNeverHit,  // A breakpoint was never hit

    Unsupported,         // The operation is not supported
};

enum class VMExitReason {
    Normal = 0,              // Time slice expiration

    Cancelled = 1,           // Execution was cancelled (possibly due to interrupt injection)
    Interrupt = 2,           // An interrupt window has opened

    PIO = 3,                 // IN or OUT instruction
    MMIO = 4,                // MMIO instruction

    Step = 5,                // Single stepping completed successfully
    SoftwareBreakpoint = 6,  // Software breakpoint
    HardwareBreakpoint = 7,  // Hardware breakpoint

    HLT = 8,                 // HLT instruction
    CPUID = 9,               // CPUID instruction
    MSRAccess = 10,          // MSR access
    TSCAccess = 15,          // TSC access (RDTSC, RDTSCP, RDMSR, WRMSR)
    Exception = 11,          // CPU exception

    Shutdown = 12,           // System shutdown
    Error = 13,              // Non-specific error
    Unhandled = 14,          // VM exit reason returned by hypervisor is unhandled
};

enum class TSCAccessType {
    RDTSC, RDTSCP, RDMSR, WRMSR
};

struct VMExitInfo {
    // The reason for the VM exit
    VMExitReason reason;

    // Details about specific VM exits
    union {
        // The exception code, when reason == VMExitReason::Exception
        uint32_t exceptionCode;

        // MSR access information, when VMExitReason::MSRAccess
        struct {
            bool isWrite;
            uint32_t msrNumber;
            uint64_t rax;
            uint64_t rdx;
        } msr;

        // TSC access information, when VMExitReason::TScAccess
        struct {
            TSCAccessType type;
            uint64_t tscAux;
            uint64_t virtualOffset;
        } tsc;

        // CPUID access information, when VMExitReason::CPUID.
        // The register fields contain the values when CPUID was executed and
        // the default* fields indicate the values the hypervisor would return
        // based on its properties and the host's capabilities.
        struct {
            uint64_t rax;
            uint64_t rcx;
            uint64_t rdx;
            uint64_t rbx;
            uint64_t defaultRax;
            uint64_t defaultRcx;
            uint64_t defaultRdx;
            uint64_t defaultRbx;
        } cpuid;
    };
};

}
