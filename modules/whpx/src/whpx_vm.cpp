/*
Implementation of the Windows Hypervisor Platform VirtualMachine class.
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
#include "whpx_vm.hpp"
#include "whpx_vp.hpp"
#include "whpx_dispatch.hpp"
#include "whpx_defs.hpp"

namespace virt86::whpx {

WhpxVirtualMachine::WhpxVirtualMachine(WhpxPlatform& platform, const VMInitParams& params)
    : VirtualMachine(platform, params)
    , m_platform(platform)
    , m_handle(INVALID_HANDLE_VALUE)
{
}

WhpxVirtualMachine::~WhpxVirtualMachine() {
    DestroyVPs();
    if (m_handle != INVALID_HANDLE_VALUE) {
        g_dispatch.WHvDeletePartition(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool WhpxVirtualMachine::Initialize() {
    HRESULT hr = g_dispatch.WHvCreatePartition(&m_handle);
    if (S_OK != hr) {
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Configure partition to have the specified number of virtual processors
    WHV_PARTITION_PROPERTY procCount = { 0 };
    procCount.ProcessorCount = m_initParams.numProcessors;
    if (S_OK != g_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeProcessorCount, &procCount, sizeof(WHV_PARTITION_PROPERTY))) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Configure extended VM exits
    auto enabledVMExits = BitmaskEnum(m_initParams.extendedVMExits & m_platform.GetFeatures().extendedVMExits);
    if (enabledVMExits) {
        WHV_PARTITION_PROPERTY vmExits = { 0 };
        if (enabledVMExits.AnyOf(ExtendedVMExit::CPUID)) {
            vmExits.ExtendedVmExits.X64CpuidExit = TRUE;
        }
        if (enabledVMExits.AnyOf(ExtendedVMExit::MSRAccess)) {
            vmExits.ExtendedVmExits.X64MsrExit = TRUE;
        }
        if (enabledVMExits.AnyOf(ExtendedVMExit::Exception)) {
            vmExits.ExtendedVmExits.ExceptionExit = TRUE;
        }

        vmExits.ExtendedVmExits.X64CpuidExit = TRUE;
        if (S_OK != g_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeExtendedVmExits, &vmExits, sizeof(WHV_PARTITION_PROPERTY))) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            return false;
        }
    }

    // Configure CPUID functions to trigger a VM exit if exit on CPUID
    // instruction is enabled
    if (enabledVMExits.AnyOf(ExtendedVMExit::CPUID)) {
        auto count = m_initParams.vmExitCPUIDFunctions.size();
        if (count > 0) {
            UINT32 *functions = new UINT32[count];
            for (size_t i = 0; i < count; i++) {
                functions[i++] = m_initParams.vmExitCPUIDFunctions[i];
            }
            if (S_OK != g_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeCpuidExitList, functions, count * sizeof(UINT32))) {
                delete[] functions;
                return false;
            }
            delete[] functions;
        }
    }
    
    // Configure exception codes that will trigger a VM exit if exit on
    // exception is enabled
    if (enabledVMExits.AnyOf(ExtendedVMExit::Exception)) {
        WHV_PARTITION_PROPERTY property = { 0 };
        property.ExceptionExitBitmap = static_cast<UINT64>(m_initParams.vmExitExceptions);

        if (S_OK != g_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeExceptionExitBitmap, &property, sizeof(WHV_PARTITION_PROPERTY))) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            return false;
        }
    }

    // Define custom CPUID results
    if (m_initParams.CPUIDResults.size() > 0) {
        auto count = m_initParams.CPUIDResults.size();
        WHV_X64_CPUID_RESULT *cpuidResultList = new WHV_X64_CPUID_RESULT[count];
        ZeroMemory(cpuidResultList, count * sizeof(WHV_X64_CPUID_RESULT));

        for (size_t i = 0; i < count; i++) {
            cpuidResultList[i].Function = m_initParams.CPUIDResults[i].function;
            cpuidResultList[i].Eax = m_initParams.CPUIDResults[i].eax;
            cpuidResultList[i].Ebx = m_initParams.CPUIDResults[i].ebx;
            cpuidResultList[i].Ecx = m_initParams.CPUIDResults[i].ecx;
            cpuidResultList[i].Edx = m_initParams.CPUIDResults[i].edx;
        }

        if (S_OK != g_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeCpuidResultList, cpuidResultList, count * sizeof(WHV_X64_CPUID_RESULT))) {
            delete[] cpuidResultList;
            return false;
        }
        delete[] cpuidResultList;
    }

    // Setup the partition
    if (S_OK != g_dispatch.WHvSetupPartition(m_handle)) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Create virtual processors
    for (uint32_t id = 0; id < m_initParams.numProcessors; id++) {
        auto vp = new WhpxVirtualProcessor(*this, id);
        if (!vp->Initialize()) {
            delete vp;
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            return false;
        }
        RegisterVP(vp);
    }

    return true;
}

static inline WHV_MAP_GPA_RANGE_FLAGS ToWHVFlags(const MemoryFlags flags) {
    WHV_MAP_GPA_RANGE_FLAGS whvFlags = WHvMapGpaRangeFlagNone;
    auto bmFlags = BitmaskEnum(flags);
    if (bmFlags.AnyOf(MemoryFlags::Read)) whvFlags |= WHvMapGpaRangeFlagRead;
    if (bmFlags.AnyOf(MemoryFlags::Write)) whvFlags |= WHvMapGpaRangeFlagWrite;
    if (bmFlags.AnyOf(MemoryFlags::Execute)) whvFlags |= WHvMapGpaRangeFlagExecute;
    if (bmFlags.AnyOf(MemoryFlags::DirtyPageTracking)) whvFlags |= WHvMapGpaRangeFlagTrackDirtyPages;
    return whvFlags;
}

MemoryMappingStatus WhpxVirtualMachine::MapGuestMemoryImpl(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags, void *memory) {
    return MapGuestMemoryLargeImpl(baseAddress, size, flags, memory);
}

MemoryMappingStatus WhpxVirtualMachine::MapGuestMemoryLargeImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) {
    WHV_MAP_GPA_RANGE_FLAGS whvFlags = ToWHVFlags(flags);
    if (whvFlags == WHvMapGpaRangeFlagNone) {
        return MemoryMappingStatus::InvalidFlags;
    }

    HRESULT hr = g_dispatch.WHvMapGpaRange(m_handle, memory, baseAddress, size, whvFlags);
    if (S_OK != hr) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus WhpxVirtualMachine::UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint32_t size) {
    return UnmapGuestMemoryLargeImpl(baseAddress, size);
}

MemoryMappingStatus WhpxVirtualMachine::UnmapGuestMemoryLargeImpl(const uint64_t baseAddress, const uint64_t size) {
    HRESULT hr = g_dispatch.WHvUnmapGpaRange(m_handle, baseAddress, size);
    if (S_OK != hr) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus WhpxVirtualMachine::SetGuestMemoryFlagsImpl(const uint64_t baseAddress, const uint32_t size, const MemoryFlags flags) {
    return SetGuestMemoryFlagsLargeImpl(baseAddress, size, flags);
}

MemoryMappingStatus WhpxVirtualMachine::SetGuestMemoryFlagsLargeImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags) {
    WHV_MAP_GPA_RANGE_FLAGS whvFlags = ToWHVFlags(flags);
    if (whvFlags == WHvMapGpaRangeFlagNone) {
        return MemoryMappingStatus::InvalidFlags;
    }

    MemoryRegion *region = GetMemoryRegion(baseAddress);
    if (region == nullptr) {
        return MemoryMappingStatus::InvalidRange;
    }
    void *hostMemory = reinterpret_cast<uint8_t*>(region->hostMemory) + (baseAddress - region->baseAddress);

    auto status = UnmapGuestMemoryLargeImpl(baseAddress, size);
    if (status != MemoryMappingStatus::OK) {
        return status;
    }

    return MapGuestMemoryLargeImpl(baseAddress, size, flags, hostMemory);
}

DirtyPageTrackingStatus WhpxVirtualMachine::QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) {
    // Bitmap buffer size must be a multiple of 8 bytes since WHPX uses UINT64
    if (bitmapSize & 0x7) {
        return DirtyPageTrackingStatus::MisalignedBitmap;
    }

    HRESULT hr = g_dispatch.WHvQueryGpaRangeDirtyBitmap(m_handle, baseAddress, size, bitmap, bitmapSize);
    if (WHV_E_GPA_RANGE_NOT_FOUND == hr) {
        return DirtyPageTrackingStatus::NotEnabled;
    }
    if (S_OK != hr) {
        return DirtyPageTrackingStatus::Failed;
    }

    return DirtyPageTrackingStatus::OK;
}

DirtyPageTrackingStatus WhpxVirtualMachine::ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) {
    HRESULT hr = g_dispatch.WHvQueryGpaRangeDirtyBitmap(m_handle, baseAddress, size, NULL, 0);
    if (WHV_E_GPA_RANGE_NOT_FOUND == hr) {
        return DirtyPageTrackingStatus::NotEnabled;
    }
    if (S_OK != hr) {
        return DirtyPageTrackingStatus::Failed;
    }

    return DirtyPageTrackingStatus::OK;
}

}
