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
    Normal,              // Time slice expiration

    Cancelled,           // Execution was cancelled (possibly due to interrupt injection)
    Interrupt,           // An interrupt window has opened

    PIO,                 // IN or OUT instruction
    MMIO,                // MMIO instruction

    Step,                // Single stepping completed successfully
    SoftwareBreakpoint,  // Software breakpoint
    HardwareBreakpoint,  // Hardware breakpoint

    HLT,                 // HLT instruction
    CPUID,               // CPUID instruction
    MSRAccess,           // MSR access
    Exception,           // CPU exception

    Shutdown,            // System shutdown
    Error,               // Non-specific error
    Unhandled,           // VM exit reason returned by hypervisor is unhandled
};

struct VMExitInfo {
    // The reason for the VM exit
    VMExitReason reason;

    // Details about specific VM exits
    union {
        // The exception code, when reason == VMExitReason::Exception
        uint32_t exceptionCode;

        // MSR access information, whem VMExitReason::MSRAccess
        struct {
            bool isWrite;
            uint32_t msrNumber;
            uint64_t rax;
            uint64_t rdx;
        } msr;

        // CPUID access information, whem VMExitReason::CPUID.
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
