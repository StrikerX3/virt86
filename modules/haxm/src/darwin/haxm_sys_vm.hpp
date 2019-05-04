/*
HAXM system-based virtual machine implementation for Mac OS X.
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

#include "virt86/platform/platform.hpp"
#include "haxm_platform_impl.hpp"
#include "haxm_sys_platform.hpp"

#include "interface/hax_interface.hpp"

#include <sys/ioctl.h>

namespace virt86::haxm {

class HaxmVirtualMachineSysImpl {
public:
    HaxmVirtualMachineSysImpl(HaxmPlatformImpl& platformImpl) noexcept;
    ~HaxmVirtualMachineSysImpl() noexcept;

    // Prevent copy construction and copy assignment
    HaxmVirtualMachineSysImpl(const HaxmVirtualMachineSysImpl&) = delete;
    HaxmVirtualMachineSysImpl& operator=(const HaxmVirtualMachineSysImpl&) = delete;

    // Prevent move construction and move assignment
    HaxmVirtualMachineSysImpl(HaxmVirtualMachineSysImpl&&) = delete;
    HaxmVirtualMachineSysImpl&& operator=(HaxmVirtualMachineSysImpl&&) = delete;

    // Disallow taking the address
    HaxmVirtualMachineSysImpl *operator&() = delete;

    bool Initialize(const size_t numProcessors, uint32_t *out_vmID) noexcept;
    void Destroy() noexcept;

    bool ReportQEMUVersion(hax_qemu_version& version) noexcept;

    MemoryMappingStatus MapHostMemory(void *memory, const uint32_t size) noexcept;
    MemoryMappingStatus MapGuestMemory(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags, void *memory) noexcept;
    bool UnmapGuestMemory(const uint64_t baseAddress, const uint32_t size) noexcept;
    
    MemoryMappingStatus MapHostMemoryLarge(void *memory, const uint64_t size) noexcept;
    MemoryMappingStatus MapGuestMemoryLarge(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept;
    bool UnmapGuestMemoryLarge(const uint64_t baseAddress, const uint64_t size) noexcept;

    bool SetGuestMemoryFlagsLarge(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept;

    const int FileDescriptor() const noexcept { return m_fd; }

private:
    HaxmPlatformImpl& m_platformImpl;

    int m_fdHAXM;
    int m_fd;
};

}
