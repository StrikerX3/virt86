/*
Ensures feature parity with the latest version of the Windows Hypervisor
Platform API if the system uses older versions.

The following Windows 10 SDK versions support WHPX:
- 10.0.17134.0: Introduced the Windows Hypervisor Platform API
- 10.0.17763.0: Expanded the API with five new features and access to three new
  registers.
- 10.0.18362.0: WHVSuspendPartitionTime and WHvResumePartitionTime functions

This header ensures users of older versions of the SDK can use the features of
the latest version of the SDK.
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

namespace virt86::whpx {

// In sdkddkver.h:
//   NTDDI_WIN10_19H1 = 10.0.18362.0
//   NTDDI_WIN10_RS5  = 10.0.17763.0
//   NTDDI_WIN10_RS4  = 10.0.17134.0

// Handle data structure differences between SDK versions
struct WhpxDefs {

#ifdef NTDDI_WIN10_RS5
    using WHV_CAPABILITY_FEATURES = ::WHV_CAPABILITY_FEATURES;
#else
    typedef union _WHV_CAPABILITY_FEATURES {
        struct {
            UINT64 PartialUnmap : 1;
            UINT64 LocalApicEmulation : 1;
            UINT64 Xsave : 1;
            UINT64 DirtyPageTracking : 1;
            UINT64 SpeculationControl : 1;
            UINT64 Reserved : 59;
        };

        UINT64 AsUINT64;
        
        _WHV_CAPABILITY_FEATURES(::WHV_CAPABILITY_FEATURES& features) : AsUINT64(features.AsUINT64) {}
        _WHV_CAPABILITY_FEATURES& operator=(::WHV_CAPABILITY_FEATURES& features) { AsUINT64 = features.AsUINT64; }
    } WHV_CAPABILITY_FEATURES;
#endif

};

// Add missing enum values
#ifndef NTDDI_WIN10_RS5
const WHV_REGISTER_NAME WHvX64RegisterXCr0 = (const WHV_REGISTER_NAME)0x00000027;

const WHV_REGISTER_NAME WHvX64RegisterSpecCtrl = (const WHV_REGISTER_NAME)0x00002084;
const WHV_REGISTER_NAME WHvX64RegisterPredCmd = (const WHV_REGISTER_NAME)0x00002085;

const WHV_MAP_GPA_RANGE_FLAGS WHvMapGpaRangeFlagTrackDirtyPages = (const WHV_MAP_GPA_RANGE_FLAGS)0x00000008;
#endif

}
