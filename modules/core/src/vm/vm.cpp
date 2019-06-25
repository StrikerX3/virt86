/*
Base implemetation of the VirtualMachine interface.
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
#include "virt86/vm/vm.hpp"
#include "virt86/platform/platform.hpp"
#include "virt86/util/host_info.hpp"

namespace virt86 {

// No-op I/O handlers to use as default when nullptr is specified.
// They always return 0 on reads and discard writes.

uint32_t __nullIORead(void*, uint16_t, size_t) noexcept { return 0; }
void __nullIOWrite(void*, uint16_t, size_t, uint32_t) noexcept { }

uint64_t __nullMMIORead(void*, uint64_t, size_t) noexcept { return 0; }
void __nullMMIOWrite(void*, uint64_t, size_t, uint64_t) noexcept { }

// ----------------------------------------------------------------------------

VirtualMachine::VirtualMachine(Platform& platform, const VMSpecifications& specifications) noexcept
    : m_specifications(specifications)
    , m_platform(platform)
    , m_io({ __nullIORead, __nullIOWrite, __nullMMIORead, __nullMMIOWrite, nullptr })
{
}

VirtualMachine::~VirtualMachine() noexcept {
}

std::optional<std::reference_wrapper<VirtualProcessor>> VirtualMachine::GetVirtualProcessor(const size_t index) {
    if (index < m_vps.size()) {
        return *m_vps.at(index);
    }
    return std::nullopt;
}

MemoryMappingStatus VirtualMachine::MapGuestMemory(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) {
    // Host memory block must be page-aligned
    if (reinterpret_cast<uintptr_t>(memory) & 0xFFF) {
        return MemoryMappingStatus::MisalignedHostMemory;
    }

    // Base address must be page-aligned
    if (baseAddress & 0xFFF) {
        return MemoryMappingStatus::MisalignedAddress;
    }

    // Size must be greater than zero
    if (size == 0) {
        return MemoryMappingStatus::EmptyRange;
    }
    
    // Size must be page-aligned
    if (size & 0xFFF) {
        return MemoryMappingStatus::MisalignedSize;
    }

    // Platform must support large memory allocation in order to map more
    // than 4 GiB of guest memory
    if ((size > 0xFFFFFFFFull) && !m_platform.GetFeatures().largeMemoryAllocation) {
        return MemoryMappingStatus::Unsupported;
    }

    // GPA range must fit within the host's limits
    if ((baseAddress & ~HostInfo.gpa.mask) || ((baseAddress + size - 1) & ~HostInfo.gpa.mask)) {
        return MemoryMappingStatus::OutOfBounds;
    }

    const auto status = MapGuestMemoryImpl(baseAddress, size, flags, memory);
    if (status == MemoryMappingStatus::OK) {
        m_memoryRegions.emplace_back(baseAddress, size, memory);
    }
    return status;
}

MemoryMappingStatus VirtualMachine::UnmapGuestMemory(const uint64_t baseAddress, const uint64_t size) {
    // Base address must be page-aligned
    if (baseAddress & 0xFFF) {
        return MemoryMappingStatus::MisalignedAddress;
    }

    // Size must be greater than zero
    if (size == 0) {
        return MemoryMappingStatus::EmptyRange;
    }

    // Size must be page-aligned
    if (size & 0xFFF) {
        return MemoryMappingStatus::MisalignedSize;
    }
    
    // Platform must support large memory allocation in order to unmap more
    // than 4 GiB of guest memory
    if ((size > 0xFFFFFFFFull) && !m_platform.GetFeatures().largeMemoryAllocation) {
        return MemoryMappingStatus::Unsupported;
    }

    const auto status = UnmapGuestMemoryImpl(baseAddress, size);
    if (status == MemoryMappingStatus::OK) {
        SubtractMemoryRange(baseAddress, size);
    }
    return status;
}

MemoryMappingStatus VirtualMachine::SetGuestMemoryFlags(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept {
    // Base address must be page-aligned
    if (baseAddress & 0xFFF) {
        return MemoryMappingStatus::MisalignedAddress;
    }

    // Size must be greater than zero
    if (size == 0) {
        return MemoryMappingStatus::EmptyRange;
    }

    // Size must be page-aligned
    if (size & 0xFFF) {
        return MemoryMappingStatus::MisalignedSize;
    }

    // Platform must support large memory allocation in order to unmap more
    // than 4 GiB of guest memory
    if ((size > 0xFFFFFFFFull) && !m_platform.GetFeatures().largeMemoryAllocation) {
        return MemoryMappingStatus::Unsupported;
    }

    return SetGuestMemoryFlagsImpl(baseAddress, size, flags);
}

DirtyPageTrackingStatus VirtualMachine::QueryDirtyPages(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept {
    // Base address must be page-aligned
    if (baseAddress & 0xFFF) {
        return DirtyPageTrackingStatus::MisalignedAddress;
    }

    // Size must be greater than zero
    if (size == 0) {
        return DirtyPageTrackingStatus::EmptyRange;
    }

    // Size must be page-aligned
    if (size & 0xFFF) {
        return DirtyPageTrackingStatus::MisalignedSize;
    }

    // Bitmap must be aligned to 8 bytes
    if (reinterpret_cast<uintptr_t>(bitmap) & 0x7) {
        return DirtyPageTrackingStatus::MisalignedBitmap;
    }

    // Bitmap buffer must be large enough to contain bits for all pages
    const uint64_t requiredSize = ((size / PAGE_SIZE + sizeof(uint64_t) - 1) / sizeof(uint64_t));
    if (bitmapSize < requiredSize) {
        return DirtyPageTrackingStatus::BitmapTooSmall;
    }

    return QueryDirtyPagesImpl(baseAddress, size, bitmap, bitmapSize);
}

DirtyPageTrackingStatus VirtualMachine::ClearDirtyPages(const uint64_t baseAddress, const uint64_t size) noexcept {
    // Base address must be page-aligned
    if (baseAddress & 0xFFF) {
        return DirtyPageTrackingStatus::MisalignedAddress;
    }

    // Size must be greater than zero
    if (size == 0) {
        return DirtyPageTrackingStatus::EmptyRange;
    }

    // Size must be page-aligned
    if (size & 0xFFF) {
        return DirtyPageTrackingStatus::MisalignedSize;
    }
    
    return ClearDirtyPagesImpl(baseAddress, size);
}

bool VirtualMachine::MemRead(const uint64_t paddr, uint64_t size, void *value) const noexcept {
    // Go through every memory region and copy data from ranges that contain
    // the requested range.
    // We need to go in reverse order to ensure the most recent mappings
    // take precedence over previous, overlapping mappings.
    const uint64_t finalPAddr = paddr + size - 1;
    for (auto it = m_memoryRegions.crbegin(); it != m_memoryRegions.crend(); it++) {
        auto& memoryRegion = *it;
        const uint64_t finalAddress = memoryRegion.baseAddress + memoryRegion.size - 1;
        if (paddr >= memoryRegion.baseAddress && finalPAddr <= finalAddress) {
            // Copy as many bytes as possible from the region
            if (size <= memoryRegion.size) {
                memcpy(value, static_cast<uint8_t*>(memoryRegion.hostMemory) + paddr - memoryRegion.baseAddress, size);
                return true;
            }
            // Decrement remaining size
            size -= memoryRegion.size;
        }
    }

    return false;
}

bool VirtualMachine::MemWrite(const uint64_t paddr, uint64_t size, const void *value) const noexcept {
    // Go through every memory region and copy data to ranges that contain the
    // requested range.
    // We need to go in reverse order to ensure the most recent mappings
    // take precedence over previous, overlapping mappings.
    const uint64_t finalPAddr = paddr + size - 1;
    for (auto it = m_memoryRegions.rbegin(); it != m_memoryRegions.rend(); it++) {
        auto& memoryRegion = *it;
        const uint64_t finalAddress = memoryRegion.baseAddress + memoryRegion.size - 1;
        if (paddr >= memoryRegion.baseAddress && finalPAddr <= finalAddress) {
            // Copy as many bytes as possible from the region
            if (size <= memoryRegion.size) {
                memcpy(static_cast<uint8_t*>(memoryRegion.hostMemory) + paddr - memoryRegion.baseAddress, value, size);
                return true;
            }
            // Decrement remaining size
            size -= memoryRegion.size;
        }
    }

    return false;
}

void VirtualMachine::RegisterIOReadCallback(IOReadFunc_t func) noexcept {
    m_io.IOReadFunc = (func == nullptr) ? __nullIORead : func;
}

void VirtualMachine::RegisterIOWriteCallback(IOWriteFunc_t func) noexcept {
    m_io.IOWriteFunc = (func == nullptr) ? __nullIOWrite : func;
}

void VirtualMachine::RegisterMMIOReadCallback(MMIOReadFunc_t func) noexcept {
    m_io.MMIOReadFunc = (func == nullptr) ? __nullMMIORead : func;
}

void VirtualMachine::RegisterMMIOWriteCallback(MMIOWriteFunc_t func) noexcept {
    m_io.MMIOWriteFunc = (func == nullptr) ? __nullMMIOWrite : func;
}

void VirtualMachine::RegisterIOContext(void *context) noexcept {
    m_io.context = context;
}

MemoryRegion *VirtualMachine::GetMemoryRegion(uint64_t address) {
    for (auto it = m_memoryRegions.begin(); it != m_memoryRegions.end(); it++) {
        auto& memoryRegion = *it;

        // Compute inclusive final address of the memory region
        const uint64_t finalRegionAddress = memoryRegion.baseAddress + memoryRegion.size - 1;

        if (address >= memoryRegion.baseAddress && address <= finalRegionAddress) {
            return &memoryRegion;
        }
    }

    return nullptr;
}

void VirtualMachine::SubtractMemoryRange(const uint64_t baseAddress, const uint64_t size) {
    // Compute inclusive final address of the region that was unmapped
    const uint64_t finalAddress = baseAddress + size - 1;

    // Find every memory region that contains the specified address range.
    // If the entire region was unmapped, remove it from the vector.
    // If only a portion of the region was unmapped, update the region and
    // possibly add a new region to reflect the change.
    auto it = m_memoryRegions.begin();
    while (it != m_memoryRegions.end()) {
        auto& memoryRegion = *it;

        // Compute inclusive final address of the memory region
        const uint64_t finalRegionAddress = memoryRegion.baseAddress + memoryRegion.size - 1;

        // Case 1: unmapped range covers the entire memory region
        // -> Remove memory region from vector
        if (baseAddress <= memoryRegion.baseAddress && finalAddress >= finalRegionAddress) {
            it = m_memoryRegions.erase(it); // Make it point to the region after the erased one
            continue; // We don't want to skip the next region
        }
        // Case 2: unmapped range covers final portion of the memory region
        // -> Update region size
        else if (baseAddress > memoryRegion.baseAddress && finalAddress >= finalRegionAddress) {
            memoryRegion.size = baseAddress - memoryRegion.baseAddress;
        }
        // Case 3: unmapped range covers initial portion of the memory region
        // -> Update region base address, size and the host memory pointer
        else if (baseAddress <= memoryRegion.baseAddress && finalAddress < finalRegionAddress) {
            uint64_t baseAddressDelta = (finalAddress + 1) - memoryRegion.baseAddress;
            memoryRegion.baseAddress = finalAddress + 1;
            memoryRegion.size = finalRegionAddress - finalAddress;
            memoryRegion.hostMemory = static_cast<uint8_t*>(memoryRegion.hostMemory) + baseAddressDelta;
        }
        // Case 4: unmapped range is contained within the memory region
        // -> Update region size and create new region for the second portion
        else {
            memoryRegion.size = baseAddress - memoryRegion.baseAddress;

            const uint64_t secondBaseAddress = finalAddress + 1;
            const uint64_t secondSize = finalRegionAddress - finalAddress;
            void *pSecondHostMemory = static_cast<uint8_t*>(memoryRegion.hostMemory) + memoryRegion.size + size;
            it = m_memoryRegions.insert(it, MemoryRegion{ secondBaseAddress, secondSize, pSecondHostMemory });
            // Don't continue here, we want to skip the inserted region
        }

        ++it;
    }
}

void VirtualMachine::RegisterVP(std::unique_ptr<VirtualProcessor> vp) {
    m_vps.emplace_back(std::move(vp));
}

void VirtualMachine::DestroyVPs() noexcept {
    m_vps.clear();
}

// ----- Optional features ----------------------------------------------------

MemoryMappingStatus VirtualMachine::UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) noexcept {
    return MemoryMappingStatus::Unsupported;
}

MemoryMappingStatus VirtualMachine::SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept {
    return MemoryMappingStatus::Unsupported;
}

DirtyPageTrackingStatus VirtualMachine::QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept {
    return DirtyPageTrackingStatus::Unsupported;
}

DirtyPageTrackingStatus VirtualMachine::ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) noexcept {
    return DirtyPageTrackingStatus::Unsupported;
}

}
