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

#include <Hypervisor/hv.h>

namespace virt86::hvf {

HvFVirtualMachine::HvFVirtualMachine(HvFPlatform& platform, const VMSpecifications& specifications)
    : VirtualMachine(platform, specifications)
    , m_platform(platform)
{
}

HvFVirtualMachine::~HvFVirtualMachine() {
    DestroyVPs();
    hv_vm_destroy();
}

bool HvFVirtualMachine::Initialize() {
    // Create the virtual machine
    if (HV_SUCCESS != hv_vm_create(HV_VM_DEFAULT)) {
        return false;
    }
    
    // Create virtual processors
    for (uint32_t id = 0; id < m_specifications.numProcessors; id++) {
        auto vp = new HvFVirtualProcessor(*this);
        if (!vp->Initialize()) {
            delete vp;
            return false;
        }
        RegisterVP(vp);
    }

    return true;
}

MemoryMappingStatus HvFVirtualMachine::MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) {
    hv_uvaddr_t uva = (hv_uvaddr_t)memory;
    hv_gpaddr_t gpa = (hv_gpaddr_t)baseAddress;
    size_t sz = (size_t)size;
    hv_memory_flags_t memFlags = 0;

    auto flagsBM = BitmaskEnum(flags);
    if (flagsBM.AnyOf(MemoryFlags::Read)) memFlags |= HV_MEMORY_READ;
    if (flagsBM.AnyOf(MemoryFlags::Write)) memFlags |= HV_MEMORY_WRITE;
    if (flagsBM.AnyOf(MemoryFlags::Execute)) memFlags |= HV_MEMORY_EXEC;

    if (HV_SUCCESS != hv_vm_map(uva, gpa, sz, memFlags)) {
        return MemoryMappingStatus::Failed;
    }
    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HvFVirtualMachine::UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) {
    hv_gpaddr_t gpa = (hv_gpaddr_t)baseAddress;
    size_t sz = (size_t)size;

    if (HV_SUCCESS != hv_vm_unmap(gpa, sz)) {
        return MemoryMappingStatus::Failed;
    }
    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HvFVirtualMachine::SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) {
    hv_gpaddr_t gpa = (hv_gpaddr_t)baseAddress;
    size_t sz = (size_t)size;
    hv_memory_flags_t memFlags = 0;

    auto flagsBM = BitmaskEnum(flags);
    if (flagsBM.AnyOf(MemoryFlags::Read)) memFlags |= HV_MEMORY_READ;
    if (flagsBM.AnyOf(MemoryFlags::Write)) memFlags |= HV_MEMORY_WRITE;
    if (flagsBM.AnyOf(MemoryFlags::Execute)) memFlags |= HV_MEMORY_EXEC;

    if (HV_SUCCESS != hv_vm_protect(gpa, sz, memFlags)) {
        return MemoryMappingStatus::Failed;
    }
    return MemoryMappingStatus::OK;
}

}
