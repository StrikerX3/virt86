/*
Declares the VirtualMachine implementation class for the KVM platform adapter.
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
#include "virt86/kvm/kvm_platform.hpp"

#include <linux/kvm.h>
#include <vector>

namespace virt86::kvm {

class KvmVirtualMachine : public VirtualMachine {
public:
    KvmVirtualMachine(KvmPlatform& platform, const VMSpecifications& specifications, int fdKVM) noexcept;
    ~KvmVirtualMachine() noexcept final;

    // Prevent copy construction and copy assignment
    KvmVirtualMachine(const KvmVirtualMachine&) = delete;
    KvmVirtualMachine& operator=(const KvmVirtualMachine&) = delete;

    // Prevent move construction and move assignment
    KvmVirtualMachine(KvmVirtualMachine&&) = delete;
    KvmVirtualMachine&& operator=(KvmVirtualMachine&&) = delete;

    // Disallow taking the address
    KvmVirtualMachine *operator&() = delete;

    const int FileDescriptor() const noexcept { return m_fd; }
    const int KVMFileDescriptor() const noexcept { return m_fdKVM; }

protected:
    MemoryMappingStatus MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept override;
    MemoryMappingStatus SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept override;

    DirtyPageTrackingStatus QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept override;
    DirtyPageTrackingStatus ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) noexcept override;

private:
    bool Initialize();

    KvmPlatform& m_platform;
    int m_fdKVM;
    int m_fd;

    uint32_t m_memSlot;

    std::vector<kvm_userspace_memory_region> m_memoryRegions;

    // Allow KvmPlatform to access the constructor and Initialize()
    friend class KvmPlatform;
};

}
