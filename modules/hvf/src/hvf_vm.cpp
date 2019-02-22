/*
Implementation of the Hypervisor.Framework VirtualMachine class.
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
#include "hvf_vm.hpp"
#include "hvf_vp.hpp"

namespace virt86::hvf {

HvFVirtualMachine::HvFVirtualMachine(HvFPlatform& platform, const VMSpecifications& specifications)
    : VirtualMachine(platform, specifications)
    , m_platform(platform)
{
}

HvFVirtualMachine::~HvFVirtualMachine() {
    DestroyVPs();
    // TODO: Close/release/free VM
}

bool HvFVirtualMachine::Initialize() {
    // TODO: Create the VM
    // TODO: Initialize virtual machine based on VMSpecifications (m_specifications).
    // Unsupported parameters should be ignored.

    // TODO: Initialize any additional features available to the platform

    // Create virtual processors
    for (uint32_t id = 0; id < m_specifications.numProcessors; id++) {
        auto vp = new HvFVirtualProcessor(*this, id);
        if (!vp->Initialize()) {
            delete vp;
            return false;
        }
        RegisterVP(vp);
    }

    return true;
}

MemoryMappingStatus HvFVirtualMachine::MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) {
    // TODO: Map the specified GPA range to the guest.

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HvFVirtualMachine::UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) {
    // TODO: Unmap the specified GPA range from the guest.

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HvFVirtualMachine::SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) {
    // TODO: Configure the flags of the specified GPA range.

    return MemoryMappingStatus::OK;
}

DrtyPageTrackingStatus HvFVirtualMachine::QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) {
    // TODO: Query the dirty page bitmap for the specified GPA range.

    return DirtyPageTrackingStatus::OK;
}

DirtyPageTrackingStatus HvFVirtualMachine::ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) {
    // TODO: Clear the dirty page bitmap for the specified GPA range.

    return DirtyPageTrackingStatus::OK;
}

}
