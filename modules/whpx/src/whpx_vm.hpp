/*
Declares the VirtualMachine implementation class for the Windows Hypervisor
Platform adapter.
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
#include "virt86/whpx/whpx_platform.hpp"
#include "whpx_dispatch.hpp"

namespace virt86::whpx {

class WhpxVirtualMachine : public VirtualMachine {
public:
    WhpxVirtualMachine(WhpxPlatform& platform, const WhpxDispatch& dispatch, const VMSpecifications& specifications) noexcept;
    ~WhpxVirtualMachine() noexcept final;

    // Prevent copy construction and copy assignment
    WhpxVirtualMachine(const WhpxVirtualMachine&) = delete;
    WhpxVirtualMachine& operator=(const WhpxVirtualMachine&) = delete;

    // Prevent move construction and move assignment
    WhpxVirtualMachine(WhpxVirtualMachine&&) = delete;
    WhpxVirtualMachine&& operator=(WhpxVirtualMachine&&) = delete;

    // Disallow taking the address
    WhpxVirtualMachine *operator&() = delete;

    const WHV_PARTITION_HANDLE Handle() const noexcept { return m_handle; }

protected:

    MemoryMappingStatus MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept override;
    MemoryMappingStatus UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) noexcept override;

    DirtyPageTrackingStatus QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept override;
    DirtyPageTrackingStatus ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) noexcept override;

private:
    WhpxPlatform& m_platform;
    const WhpxDispatch& m_dispatch;

    WHV_PARTITION_HANDLE m_handle;

    bool Initialize();

    // Allow WhpxPlatform to access Initialize()
    friend class WhpxPlatform;
};

}
