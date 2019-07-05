/*
Declares dynamic dispatching functions for the Windows Hypervisor Platform API.

This avoids directly linking against the platform's dynamic libraries, which
would cause the application to require said libraries that are not available
in older versions of Windows.
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

#include <Windows.h>
#include <WinHvPlatform.h>
#include <WinHvEmulation.h>

#include "whpx_defs.hpp"
#include "virt86/sys/windows/version_info.hpp"

using namespace virt86::sys::windows;

#define WHPX_PLATFORM_FUNCTIONS(FUNC) \
    FUNC(HRESULT, WHvGetCapability, (WHV_CAPABILITY_CODE CapabilityCode, VOID* CapabilityBuffer, UINT32 CapabilityBufferSizeInBytes, UINT32* WrittenSizeInBytes)) \
    FUNC(HRESULT, WHvCreatePartition, (WHV_PARTITION_HANDLE* Partition)) \
    FUNC(HRESULT, WHvSetupPartition, (WHV_PARTITION_HANDLE Partition)) \
    FUNC(HRESULT, WHvDeletePartition, (WHV_PARTITION_HANDLE Partition)) \
    FUNC(HRESULT, WHvGetPartitionProperty, (WHV_PARTITION_HANDLE Partition, WHV_PARTITION_PROPERTY_CODE PropertyCode, VOID* PropertyBuffer, UINT32 PropertyBufferSizeInBytes, UINT32* WrittenSizeInBytes)) \
    FUNC(HRESULT, WHvSetPartitionProperty, (WHV_PARTITION_HANDLE Partition, WHV_PARTITION_PROPERTY_CODE PropertyCode, const VOID* PropertyBuffer, UINT32 PropertyBufferSizeInBytes)) \
    FUNC(HRESULT, WHvMapGpaRange, (WHV_PARTITION_HANDLE Partition, VOID* SourceAddress, WHV_GUEST_PHYSICAL_ADDRESS GuestAddress, UINT64 SizeInBytes, WHV_MAP_GPA_RANGE_FLAGS Flags)) \
    FUNC(HRESULT, WHvUnmapGpaRange, (WHV_PARTITION_HANDLE Partition, WHV_GUEST_PHYSICAL_ADDRESS GuestAddress, UINT64 SizeInBytes)) \
    FUNC(HRESULT, WHvTranslateGva, (WHV_PARTITION_HANDLE Partition, UINT32 VpIndex, WHV_GUEST_VIRTUAL_ADDRESS Gva, WHV_TRANSLATE_GVA_FLAGS TranslateFlags, WHV_TRANSLATE_GVA_RESULT* TranslationResult, WHV_GUEST_PHYSICAL_ADDRESS* Gpa)) \
    FUNC(HRESULT, WHvCreateVirtualProcessor, (WHV_PARTITION_HANDLE Partition, UINT32 VpIndex, UINT32 Flags)) \
    FUNC(HRESULT, WHvDeleteVirtualProcessor, (WHV_PARTITION_HANDLE Partition, UINT32 VpIndex)) \
    FUNC(HRESULT, WHvRunVirtualProcessor, (WHV_PARTITION_HANDLE Partition, UINT32 VpIndex, VOID* ExitContext, UINT32 ExitContextSizeInBytes)) \
    FUNC(HRESULT, WHvCancelRunVirtualProcessor, (WHV_PARTITION_HANDLE Partition, UINT32 VpIndex, UINT32 Flags)) \
    FUNC(HRESULT, WHvGetVirtualProcessorRegisters, (WHV_PARTITION_HANDLE Partition, UINT32 VpIndex, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, WHV_REGISTER_VALUE* RegisterValues)) \
    FUNC(HRESULT, WHvSetVirtualProcessorRegisters, (WHV_PARTITION_HANDLE Partition, UINT32 VpIndex, const WHV_REGISTER_NAME* RegisterNames, UINT32 RegisterCount, const WHV_REGISTER_VALUE* RegisterValues))

#define WHPX_OPTIONAL_PLATFORM_FUNCTIONS(FUNC) \
    FUNC(HRESULT, WHvQueryGpaRangeDirtyBitmap, (WHV_PARTITION_HANDLE Partition, WHV_GUEST_PHYSICAL_ADDRESS GuestAddress, UINT64 RangeSizeInBytes, UINT64* Bitmap, UINT32 BitmapSizeInBytes)) \
    FUNC(HRESULT, WHvSuspendPartitionTime, (WHV_PARTITION_HANDLE Partition)) \
    FUNC(HRESULT, WHvResumePartitionTime, (WHV_PARTITION_HANDLE Partition))

#define WHPX_EMULATION_FUNCTIONS(FUNC) \
    FUNC(HRESULT, WHvEmulatorCreateEmulator, (const WHV_EMULATOR_CALLBACKS* Callbacks, WHV_EMULATOR_HANDLE* Emulator)) \
    FUNC(HRESULT, WHvEmulatorDestroyEmulator, (WHV_EMULATOR_HANDLE Emulator)) \
    FUNC(HRESULT, WHvEmulatorTryIoEmulation, (WHV_EMULATOR_HANDLE Emulator, VOID* Context, const WHV_VP_EXIT_CONTEXT* VpContext, const WHV_X64_IO_PORT_ACCESS_CONTEXT* IoInstructionContext, WHV_EMULATOR_STATUS* EmulatorReturnStatus)) \
    FUNC(HRESULT, WHvEmulatorTryMmioEmulation, (WHV_EMULATOR_HANDLE Emulator, VOID* Context, const WHV_VP_EXIT_CONTEXT* VpContext, const WHV_MEMORY_ACCESS_CONTEXT* MmioInstructionContext, WHV_EMULATOR_STATUS* EmulatorReturnStatus))

#define WHPX_TYPEDEF_FUNC(returnType, name, parameters) \
    typedef returnType (WINAPI *name##_t) parameters;

#define WHPX_DEFINE_FUNC(returnType, name, parameters) \
    name##_t name = nullptr; 

WHPX_PLATFORM_FUNCTIONS(WHPX_TYPEDEF_FUNC);
WHPX_OPTIONAL_PLATFORM_FUNCTIONS(WHPX_TYPEDEF_FUNC);
WHPX_EMULATION_FUNCTIONS(WHPX_TYPEDEF_FUNC);

namespace virt86::whpx {

// The version of WHPX present in the system.
extern VersionInfo g_whpxVersion;

struct WhpxDispatch {
    WhpxDispatch() = default;

    bool Load() noexcept;
    bool m_loaded = false;

    HMODULE m_hWinHvPlatform = NULL;
    WHPX_PLATFORM_FUNCTIONS(WHPX_DEFINE_FUNC);
    WHPX_OPTIONAL_PLATFORM_FUNCTIONS(WHPX_DEFINE_FUNC);

    HMODULE m_hWinHvEmulation = NULL;
    WHPX_EMULATION_FUNCTIONS(WHPX_DEFINE_FUNC);
};

}
