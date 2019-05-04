/*
Implementation of the HAXM VirtualMachine class.
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
#include "haxm_vm.hpp"
#include "haxm_vp.hpp"

namespace virt86::haxm {

HaxmVirtualMachine::HaxmVirtualMachine(HaxmPlatform& platform, const VMSpecifications& specifications, HaxmPlatformImpl& platformImpl)
    : VirtualMachine(platform, specifications)
    , m_platform(platform)
    , m_platformImpl(platformImpl)
    , m_sys(std::make_unique<HaxmVirtualMachineSysImpl>(platformImpl))
    , m_fastMMIO(false)
    , m_vmID(0xFFFFFFFF)
{
}

HaxmVirtualMachine::~HaxmVirtualMachine() noexcept {
    DestroyVPs();
}

bool HaxmVirtualMachine::Initialize() {
    if (!m_sys->Initialize(m_specifications.numProcessors, &m_vmID)) {
        return false;
    }

    // Report QEMU API version 4 to enable fast MMIO
    if (m_platformImpl.m_haxCaps.winfo & HAX_CAP_FASTMMIO) {
        hax_qemu_version qversion;
        qversion.cur_version = 0x4;
        qversion.least_version = 0x4;
        m_fastMMIO = m_sys->ReportQEMUVersion(qversion);
    }

    // Create virtual processors
    for (uint32_t id = 0; id < m_specifications.numProcessors; id++) {
        auto vp = std::make_unique<HaxmVirtualProcessor>(*this, *m_sys, id);
        if (!vp->Initialize()) {
            m_sys->Destroy();
            return false;
        }
        RegisterVP(std::move(vp));
    }

    return true;
}

MemoryMappingStatus HaxmVirtualMachine::MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept {
    // Use the regular version for memory ranges up to 4 GiB
    if (size <= 0xFFFFFFFFull) {
        // FIXME: (optimization) Avoid calling this method if the host memory
        // was already allocated previously, as it incurs user to kernel space
        // transition costs
        auto result = m_sys->MapHostMemory(memory, static_cast<uint32_t>(size));
        if (result != MemoryMappingStatus::OK && result != MemoryMappingStatus::AlreadyAllocated) {
            return result;
        }

        return m_sys->MapGuestMemory(baseAddress, static_cast<uint32_t>(size), flags, memory);
    }

    // HAXM module must support 64-bit memory operations to map guest memory
    // ranges larger than 4 GiB
    if ((m_platformImpl.m_haxCaps.winfo & (HAX_CAP_64BIT_SETRAM | HAX_CAP_64BIT_RAMBLOCK)) == 0) {
        return MemoryMappingStatus::Unsupported;
    }

    // FIXME: (optimization) Avoid calling this method if the host memory
    // was already allocated previously, as it incurs user to kernel space
    // transition costs
    auto result = m_sys->MapHostMemoryLarge(memory, size);
    if (result != MemoryMappingStatus::OK && result != MemoryMappingStatus::AlreadyAllocated) {
        return result;
    }

    return m_sys->MapGuestMemoryLarge(baseAddress, size, flags, memory);
}

MemoryMappingStatus HaxmVirtualMachine::UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) noexcept {
    // HAXM API version must be 4 or greater to support this operation
    if (m_platformImpl.m_haxVer.cur_version < 4) {
        return MemoryMappingStatus::Unsupported;
    }

    // Use the regular version for memory ranges up to 4 GiB
    if (size <= 0xFFFFFFFFull) {
        if (!m_sys->UnmapGuestMemory(baseAddress, static_cast<uint32_t>(size))) {
            return MemoryMappingStatus::Failed;
        }
    }

    // HAXM module must support 64-bit memory operations
    if ((m_platformImpl.m_haxCaps.winfo & HAX_CAP_64BIT_SETRAM) == 0) {
        return MemoryMappingStatus::Unsupported;
    }

    if (!m_sys->UnmapGuestMemoryLarge(baseAddress, size)) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus HaxmVirtualMachine::SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept {
    // HAXM module must support guest memory protection operation
    if ((m_platformImpl.m_haxCaps.winfo & HAX_CAP_RAM_PROTECTION) == 0) {
        return MemoryMappingStatus::Unsupported;
    }

    if (!m_sys->SetGuestMemoryFlagsLarge(baseAddress, size, flags)) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

}
