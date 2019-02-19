/*
Declares the VirtualMachine implementation class for HAXM.
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

#include "virt86/vm/vm.hpp"
#include "virt86/haxm/haxm_platform.hpp"
#include "haxm_platform_impl.hpp"
#include "haxm_sys_vm.hpp"

#include <memory>

namespace virt86::haxm {

class HaxmVirtualMachine : public VirtualMachine {
public:
    const bool FastMMIOEnabled() const { return m_fastMMIO; }
    const uint32_t ID() const { return m_vmID; }

protected:
    virtual ~HaxmVirtualMachine() override;

    MemoryMappingStatus MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) override;
    MemoryMappingStatus UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) override;
    MemoryMappingStatus SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) override;

private:
    HaxmVirtualMachine(HaxmPlatform& platform, const VMSpecifications& specifications, HaxmPlatformImpl& platformImpl);
    bool Initialize();

    HaxmPlatform& m_platform;
    HaxmPlatformImpl& m_platformImpl;
    std::unique_ptr<HaxmVirtualMachineSysImpl> m_sys;

    bool m_fastMMIO;
    uint32_t m_vmID;

    // Allow HaxmPlatform to access the constructor and Initialize()
    friend class HaxmPlatform;
};

}
