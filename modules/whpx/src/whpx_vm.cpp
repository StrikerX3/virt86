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

WhpxVirtualMachine::WhpxVirtualMachine(WhpxPlatform& platform, const WhpxDispatch& dispatch, const VMSpecifications& specifications) noexcept
    : VirtualMachine(platform, specifications)
    , m_platform(platform)
    , m_dispatch(dispatch)
    , m_handle(INVALID_HANDLE_VALUE)
{
}

WhpxVirtualMachine::~WhpxVirtualMachine() noexcept {
    DestroyVPs();
    if (m_handle != INVALID_HANDLE_VALUE) {
        m_dispatch.WHvDeletePartition(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool WhpxVirtualMachine::Initialize() {
    const HRESULT hr = m_dispatch.WHvCreatePartition(&m_handle);
    if (S_OK != hr) {
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Configure partition to have the specified number of virtual processors
    WHV_PARTITION_PROPERTY procCount = { 0 };
    procCount.ProcessorCount = m_specifications.numProcessors;
    if (S_OK != m_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeProcessorCount, &procCount, sizeof(WHV_PARTITION_PROPERTY))) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Configure extended VM exits
    const auto enabledVMExits = BitmaskEnum(m_specifications.extendedVMExits & m_platform.GetFeatures().extendedVMExits);
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
        if (S_OK != m_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeExtendedVmExits, &vmExits, sizeof(WHV_PARTITION_PROPERTY))) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            return false;
        }
    }

    // Configure CPUID functions to trigger a VM exit if exit on CPUID
    // instruction is enabled
    if (enabledVMExits.AnyOf(ExtendedVMExit::CPUID)) {
        const auto count = m_specifications.vmExitCPUIDFunctions.size();
        if (count > 0) {
            UINT32 *functions = new UINT32[count];
            for (size_t i = 0; i < count; i++) {
                functions[i++] = m_specifications.vmExitCPUIDFunctions.at(i);
            }
            if (S_OK != m_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeCpuidExitList, functions, count * sizeof(UINT32))) {
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
        property.ExceptionExitBitmap = static_cast<UINT64>(m_specifications.vmExitExceptions);

        if (S_OK != m_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeExceptionExitBitmap, &property, sizeof(WHV_PARTITION_PROPERTY))) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            return false;
        }
    }

    // Define custom CPUID results
    if (m_specifications.CPUIDResults.size() > 0) {
        const auto count = m_specifications.CPUIDResults.size();
        WHV_X64_CPUID_RESULT *cpuidResultList = new WHV_X64_CPUID_RESULT[count];
        ZeroMemory(cpuidResultList, count * sizeof(WHV_X64_CPUID_RESULT));

        for (size_t i = 0; i < count; i++) {
            auto& entry = m_specifications.CPUIDResults.at(i);
            cpuidResultList[i].Function = entry.function;
            cpuidResultList[i].Eax = entry.eax;
            cpuidResultList[i].Ebx = entry.ebx;
            cpuidResultList[i].Ecx = entry.ecx;
            cpuidResultList[i].Edx = entry.edx;
        }

        if (S_OK != m_dispatch.WHvSetPartitionProperty(m_handle, WHvPartitionPropertyCodeCpuidResultList, cpuidResultList, count * sizeof(WHV_X64_CPUID_RESULT))) {
            delete[] cpuidResultList;
            return false;
        }
        delete[] cpuidResultList;
    }

    // Setup the partition
    if (S_OK != m_dispatch.WHvSetupPartition(m_handle)) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Create virtual processors
    for (uint32_t id = 0; id < m_specifications.numProcessors; id++) {
        auto vp = std::make_unique<WhpxVirtualProcessor>(*this, m_dispatch, id);
        if (!vp->Initialize()) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            return false;
        }
        RegisterVP(std::move(vp));
    }

    return true;
}

static inline WHV_MAP_GPA_RANGE_FLAGS ToWHVFlags(const MemoryFlags flags) noexcept {
    WHV_MAP_GPA_RANGE_FLAGS whvFlags = WHvMapGpaRangeFlagNone;
    const auto bmFlags = BitmaskEnum(flags);
    if (bmFlags.AnyOf(MemoryFlags::Read)) whvFlags |= WHvMapGpaRangeFlagRead;
    if (bmFlags.AnyOf(MemoryFlags::Write)) whvFlags |= WHvMapGpaRangeFlagWrite;
    if (bmFlags.AnyOf(MemoryFlags::Execute)) whvFlags |= WHvMapGpaRangeFlagExecute;
    if (bmFlags.AnyOf(MemoryFlags::DirtyPageTracking) && WHPX_MIN_VERSION(10_0_17763_0)) whvFlags |= WHvMapGpaRangeFlagTrackDirtyPages;
    return whvFlags;
}

MemoryMappingStatus WhpxVirtualMachine::MapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size, const MemoryFlags flags, void *memory) noexcept {
    const WHV_MAP_GPA_RANGE_FLAGS whvFlags = ToWHVFlags(flags);
    if (whvFlags == WHvMapGpaRangeFlagNone) {
        return MemoryMappingStatus::InvalidFlags;
    }

    const HRESULT hr = m_dispatch.WHvMapGpaRange(m_handle, memory, baseAddress, size, whvFlags);
    if (S_OK != hr) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

MemoryMappingStatus WhpxVirtualMachine::UnmapGuestMemoryImpl(const uint64_t baseAddress, const uint64_t size) noexcept {
    const HRESULT hr = m_dispatch.WHvUnmapGpaRange(m_handle, baseAddress, size);
    if (S_OK != hr) {
        return MemoryMappingStatus::Failed;
    }

    return MemoryMappingStatus::OK;
}

DirtyPageTrackingStatus WhpxVirtualMachine::QueryDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size, uint64_t *bitmap, const size_t bitmapSize) noexcept {
    if (m_dispatch.WHvQueryGpaRangeDirtyBitmap == nullptr) {
        return DirtyPageTrackingStatus::Unsupported;
    }

    // Bitmap buffer size must be a multiple of 8 bytes since WHPX uses UINT64
    if (bitmapSize & 0x7) {
        return DirtyPageTrackingStatus::MisalignedBitmap;
    }

    const HRESULT hr = m_dispatch.WHvQueryGpaRangeDirtyBitmap(m_handle, baseAddress, size, bitmap, bitmapSize);
    if (WHV_E_GPA_RANGE_NOT_FOUND == hr) {
        return DirtyPageTrackingStatus::NotEnabled;
    }
    if (S_OK != hr) {
        return DirtyPageTrackingStatus::Failed;
    }

    return DirtyPageTrackingStatus::OK;
}

DirtyPageTrackingStatus WhpxVirtualMachine::ClearDirtyPagesImpl(const uint64_t baseAddress, const uint64_t size) noexcept {
    const HRESULT hr = m_dispatch.WHvQueryGpaRangeDirtyBitmap(m_handle, baseAddress, size, NULL, 0);
    if (WHV_E_GPA_RANGE_NOT_FOUND == hr) {
        return DirtyPageTrackingStatus::NotEnabled;
    }
    if (S_OK != hr) {
        return DirtyPageTrackingStatus::Failed;
    }

    return DirtyPageTrackingStatus::OK;
}

}
