/*
Defines virtual machine specifications to be used when creating a virtual
machine through a Platform instance.

Features not supported by a platform are ignored.
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

#include "virt86/platform/features.hpp"
#include "virt86/vp/cpuid.hpp"

#include <vector>

namespace virt86 {

/**
 * Virtual machine specifications.
 */
struct VMSpecifications {
    /**
     * The number of virtual processors to create in the virtual machine.
     * Must be a positive number.
     */
    size_t numProcessors;

    /**
     * The set of extended VM exits to enable in the virtual machine.
     */
    ExtendedVMExit extendedVMExits;

    /**
     * CPUID functions to trigger a VM exit when exit on CPUID instruction is
     * enabled.
     */
    std::vector<uint32_t> vmExitCPUIDFunctions;

    /**
     * Exception codes to trigger a VM exit when exit on exceptions is enabled.
     */
    ExceptionCode vmExitExceptions;

    /**
     * Custom CPUID results to generate.
     */
    std::vector<CPUIDResult> CPUIDResults;

    /**
     * Guest TSC frequency to use. A value of zero means no adjustment.
     */
    uint64_t guestTSCFrequency;

    /**
     * KVM-specific parameters. Ignored by all other platforms.
     */
    struct {
        /**
         * Specifies the base address of the identity map page, used with the
         * KVM_SET_IDENTITY_MAP_ADDR ioctl. If set to zero, assumes the default
         * value of 0xfffbc000.
         */
        uint32_t identityMapPageAddress = 0xfffbc000;
    } kvm;
};

}
