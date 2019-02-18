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

#include <Windows.h>
#include <WinHvPlatform.h>

namespace virt86::whpx {

class WhpxVirtualMachine : public VirtualMachine {
public:
    const WHV_PARTITION_HANDLE Handle() const { return m_handle; }

protected:
    virtual ~WhpxVirtualMachine() override;

    MemoryMappingStatus MapGuestMemoryImpl(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags, void *memory) override;
    MemoryMappingStatus MapGuestMemoryLargeImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) override;

    MemoryMappingStatus UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint32_t size) override;
    MemoryMappingStatus UnmapGuestMemoryLargeImpl(const uint64_t baseAddress, const uint64_t size) override;

    MemoryMappingStatus SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags) override;
    MemoryMappingStatus SetGuestMemoryFlagsLargeImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) override;

    DirtyPageTrackingStatus QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) override;
    DirtyPageTrackingStatus ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) override;

private:
    WhpxPlatform& m_platform;

    WHV_PARTITION_HANDLE m_handle;

    WhpxVirtualMachine(WhpxPlatform& platform, const VMInitParams& params);
    bool Initialize();

    // Allow WhpxPlatform to access the constructor and Initialize()
    friend class WhpxPlatform;
};

}
