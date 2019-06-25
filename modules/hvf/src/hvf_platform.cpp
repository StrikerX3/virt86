/*
Implementation of the Hypervisor.Framework Platform class.
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
#include "virt86/hvf/hvf_platform.hpp"
#include "hvf_vm.hpp"

#include "virt86/util/host_info.hpp"

namespace virt86::hvf {

HvFPlatform& HvFPlatform::Instance() {
    static HvFPlatform instance;
    return instance;
}

HvFPlatform::HvFPlatform()
    : Platform("Hypervisor.Framework")
{
    // TODO: Initialize Hypervisor.Framework.
    // Update m_initStatus with the result.
    m_initStatus = PlatformInitStatus::OK;

    // TODO: Check and publish capabilities/features
    m_features.maxProcessorsPerVM = 1;
    m_features.maxProcessorsGlobal = 1;
    m_features.guestPhysicalAddress.maxBits = HostInfo.gpa.maxBits;  // Adjust if platform imposes stricter limits
    m_features.guestPhysicalAddress.maxAddress = HostInfo.gpa.maxAddress;  // Adjust according to the above if needed (= (1ull << bits))
    m_features.guestPhysicalAddress.mask = HostInfo.gpa.mask;  // Adjust according to the above if needed (= maxAddress - 1)
    m_features.unrestrictedGuest = false;
    m_features.extendedPageTables = false;
    m_features.guestDebugging = false;  // Required for single stepping, software and hardware breakpoints
    m_features.dirtyPageTracking = false;  // Allows mapping GPA ranges with the DirtyPageTracking feature
    m_features.partialDirtyBitmap = false;  // Allows QueryDirtyBitmap to query portions of an address range
    m_features.largeMemoryAllocation = false;  // Enables memory operations with 64-bit sizes
    m_features.partialUnmapping = false;  // Enables UnmapGuestMemory with subranges of mapped ranges
    m_features.memoryUnmapping = false;  // Enables UnmapGuestMemory operations
    m_features.partialMMIOInstructions = false;  // Complex MMIO instructions need multiple executions of the virtual processor to complete
    m_features.floatingPointExtensions = HostInfo.floatingPointExtensions;   // Hypervisors may or may not support all of the host's floating point extensions; see the enum class for more details
    m_features.extendedControlRegisters = ExtendedControlRegister::None;  // Hypervisor provides access to extended control registers
    m_features.extendedVMExits = ExtendedVMExit::None;   // Additional VM exits supported by the hypervisor; must be provided to the VMSpecifications to enable them (if supported)
    m_features.exceptionExits = ExceptionCode::None;  // Exception codes supported by the hypervisor that will cause VM exits if ExtendedVMExit::Exception is supported and enabled
    m_features.customCPUIDs = false;  // Hypervisor allows configuring custom CPUID results
}

HvFPlatform::~HvFPlatform() {
    DestroyVMs();
    // TODO: Close/release/free platform
}

VirtualMachine *HvFPlatform::CreateVMImpl(const VMSpecifications& specifications) {
    auto vm = new HvFVirtualMachine(*this, specifications);
    if (!vm->Initialize()) {
        delete vm;
        return nullptr;
    }
    return vm;
}

}
