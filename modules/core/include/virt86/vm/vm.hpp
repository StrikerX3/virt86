/*
Defines the VirtualMachine interface to be implemented by hypervisor adapters.

VirtualMachine instances are created through a Platform object. The parameters
used to create the virtual machine can be retrieved with GetSpecifications().

In order to use a virtual machine, you must retrieve virtual processors created
as part of its initialization using GetVirtualProcessor(index). You may also
map and unmap guest physical address ranges or change protection flags at any
time during the lifetime of the virtual machine, as long as the hypervisor
platform supports the operations. This can be checked by looking at the
platform's features:
- memoryUnmapping for all unmapping operations
- partialUnmapping if you wish to unmap a smaller portion of a mapped GPA range
- guestMemoryProtection to change protection flags of mapped GPA ranges

Some hypervisors also keep track of dirty pages and allow users to read the
dirty bitmap. Check the dirtyPageTracking feature in order to use the dirty
page query methods.

Virtual machines can be destroyed by invoking FreeVM on the platform that
created them. They are also automatically destroyed once the platform is
unloaded. Care must be taken to not use VirtualMachine pointers after a
platform is released.

Please note that VirtualMachine instances are not meant to be used concurrently
by multiple threads. You will need to provide your own concurrency control
mechanism if you wish to share instances between threads.
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

#include "../vp/vp.hpp"
#include "../platform/features.hpp"

#include "io.hpp"
#include "mem.hpp"
#include "status.hpp"
#include "specs.hpp"

#include <vector>
#include <optional>
#include <memory>

namespace virt86 {

// Forward declare the platform class in order to give it access to the
// VirtualMachine class destructor
class Platform;

/**
 * Defines the interface for a virtual machine of a virtualization platform.
 */
class VirtualMachine {
public:
    // Prevent copy construction and copy assignment
    VirtualMachine(const VirtualMachine&) = delete;
    VirtualMachine& operator=(const VirtualMachine&) = delete;

    // Prevent move construction and move assignment
    VirtualMachine(VirtualMachine&&) = delete;
    VirtualMachine&& operator=(VirtualMachine&&) = delete;

    virtual ~VirtualMachine() noexcept;

    /**
     * Retrieves the set of specifications used to create this virtual machine.
     */
    const VMSpecifications& GetSpecifications() const noexcept { return m_specifications; }

    /**
     * Retrieves the virtual processor with the specified index, if it exists.
     */
    std::optional<std::reference_wrapper<VirtualProcessor>> GetVirtualProcessor(const size_t index);

    /**
     * Retrieves the number of virtual processors present in this virtual
     * machine.
     */
    const size_t GetVirtualProcessorCount() const noexcept { return m_vps.size(); }

    /**
     * Maps a block of host physical memory to the guest.
     *
     * The host memory block's base address and the size must be aligned to the
     * page size (4 KiB).
     *
     * Attempting to remap guest physical pages may succeed or fail, depending
     * on the underlying hypervisor's implementation.
     *
     * Some platforms do not allow mapping memory ranges larger than 4 GiB, in
     * which case the method will return MemoryMappingStatus::Unsupported.
     *
     * Attempting to map guest physical pages beyond the host's limit will
     * return MemoryMappingStatus::OutOfBounds.
     */
    MemoryMappingStatus MapGuestMemory(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory);

    /**
     * Unmaps a physical memory region from the guest.
     *
     * The base address and size must be aligned to the page size (4 KiB).
     *
     * Not all platforms support umapping GPA ranges.
     *
     * Some platforms may allow unmapping portions of GPA ranges.
     *
     * Some platforms do not allow mapping memory ranges larger than 4 GiB, in
     * which case the method will return MemoryMappingStatus::Unsupported.
     */
    MemoryMappingStatus UnmapGuestMemory(const uint64_t baseAddress, const uint64_t size);

    /**
     * Changes flags for a region of guest memory.
     *
     * The base address and size must be aligned to the page size (4 KiB).
     *
     * This is an optional operation, supported by platforms that provide the
     * guest memory protection feature.
     */
    MemoryMappingStatus SetGuestMemoryFlags(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept;

    /**
     * Queries the specified range of memory for dirty pages.
     *
     * All pages in the specified range must be mapped with the
     * MemoryFlags::DirtyPageTracking flag set.
     *
     * This is an optional operation, supported by platforms that provide the
     * dirty page tracking feature.
     */
    DirtyPageTrackingStatus QueryDirtyPages(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept;

    /**
     * Clears the dirty pages for the specified range of memory.
     *
     * All pages in the specified range must be mapped with the
     * MemoryFlags::DirtyPageTracking flag set.
     *
     * This is an optional operation, supported by platforms that provide the
     * dirty page tracking feature.
     */
    DirtyPageTrackingStatus ClearDirtyPages(const uint64_t baseAddress, const uint64_t size) noexcept;

    /**
     * Reads a portion of physical memory into the specified value.
     */
    bool MemRead(const uint64_t paddr, const uint64_t size, void *value) const noexcept;

    /**
     * Writes the specified value into physical memory.
     */
    bool MemWrite(const uint64_t paddr, const uint64_t size, const void *value) const noexcept;

    /**
     * Registers a callback function for the I/O read operation.
     * nullptr specifies the no-op handler.
     */
    void RegisterIOReadCallback(const IOReadFunc_t func) noexcept;

    /**
     * Registers a callback function for the I/O write operation.
     * nullptr specifies the no-op handler.
     */
    void RegisterIOWriteCallback(const IOWriteFunc_t func) noexcept;

    /**
     * Registers a callback function for the MMIO read operation.
     * nullptr specifies the no-op handler.
     */
    void RegisterMMIOReadCallback(const MMIOReadFunc_t func) noexcept;

    /**
     * Registers a callback function for the MMIO write operation.
     * nullptr specifies the no-op handler.
     */
    void RegisterMMIOWriteCallback(const MMIOWriteFunc_t func) noexcept;

    /**
     * Registers a pointer to arbitrary data to be passed down to the I/O
     * callback functions.
     */
    void RegisterIOContext(void *context) noexcept;

    /**
     * Retrieves the platform that created this virtual machine.
     */
    const Platform& GetPlatform() const noexcept { return m_platform; }

protected:
    VirtualMachine(Platform& platform, const VMSpecifications& specifications) noexcept;

    /**
     * Does the actual mapping of the host physical memory block to the guest
     * on the underlying hypervisor platform.
     *
     * The following preconditions are true when this method is invoked:
     * - Base address is page-aligned
     * - Size is non-zero and page-aligned
     * - Host memory block is page-aligned
     *
     * If the platform does not support GPA aliasing when attempting to map a
     * region that was previously mapped, the function must fail.
     *
     * If the dirty tracking flag is set but the platform doesn't support
     * dirty page tracking, it will be ignored.
     */
    virtual MemoryMappingStatus MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept = 0;

    /**
     * Does the actual unmapping of the host physical memory block from the
     * guest on the underlying hypervisor platform.
     *
     * The following preconditions are true when this method is invoked:
     * - Base address is page-aligned
     * - Size is non-zero and page-aligned
     * - Size is not larger than 4 GiB if the platform doesn't support large
     *   memory allocations.
     */
    virtual MemoryMappingStatus UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) noexcept;

    /**
     * Tells the hypervisor to update the flags for the given memory range.
     *
     * The following preconditions are true when this method is invoked:
     * - Base address is page-aligned
     * - Size is non-zero and page-aligned
     * - Size is not larger than 4 GiB if the platform doesn't support large
     *   memory allocations.
     *
     * If the dirty tracking flag is set but the platform doesn't support
     * dirty page tracking, it will be ignored.
     */
    virtual MemoryMappingStatus SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) noexcept;

    /**
     * Asks the hypervisor to fill in the dirty page bitmap for the given
     * memory range and clear the dirty pages.
     *
     * The following preconditions are true when this method is invoked:
     * - Base address is page-aligned
     * - Size is non-zero and page-aligned
     * - Bitmap buffer is page-aligned and has is large enough to contain all
     *   bits for the requested range
     */
    virtual DirtyPageTrackingStatus QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept;

    /**
     * Asks the hypervisor to clear the dirty pages for the given memory range.
     *
     * The following preconditions are true when this method is invoked:
     * - Base address is page-aligned
     * - Size is non-zero and page-aligned
     */
    virtual DirtyPageTrackingStatus ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) noexcept;

    /**
     * Retrieves a pointer to the memory region that contains the given GPA.
     * 
     * Returns nullptr if there is no such memory region.
     */
    MemoryRegion *GetMemoryRegion(uint64_t address);

    /**
     * Registers a newly created virtual processor in this virtual machine's
     * list of owned virtual processors. Automatically invoked by
     * VirtualProcessor and used for cleanup.
     */
    void RegisterVP(std::unique_ptr<VirtualProcessor> vp);

    /**
     * Destroys all virtual processors owned by this virtual machine.
     *
     * This method is meant to be used in destructors of subclasses to ensure
     * resources are released in the proper order.
     */
    void DestroyVPs() noexcept;

    /**
     * The platform that created this virtual machine.
     */
    Platform& m_platform;

    /**
     * The set of specifications used to create this virtual machine.
     */
    const VMSpecifications& m_specifications;

private:
    void SubtractMemoryRange(uint64_t baseAddress, uint64_t size);

    /**
     * Stores all virtual processors owned by this virtual machine.
     */
    std::vector<std::unique_ptr<VirtualProcessor>> m_vps;

    // TODO: use an interval tree instead of a vector
    /**
     * The memory regions allocated to this virtual machine.
     */
    std::vector<MemoryRegion> m_memoryRegions;

    /**
     * The I/O handlers registered with this virtual machine.
     */
    IOHandlers m_io;

    // Only let friends take the address
    VirtualMachine *operator&() noexcept { return this; }

    // Allow VirtualProcessor to access the I/O handlers
    friend class VirtualProcessor;

    // Allow Platform to take the address
    friend class Platform;
};

}
