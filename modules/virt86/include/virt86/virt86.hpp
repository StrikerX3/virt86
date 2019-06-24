/*
Includes all available platforms based on the current build system
configuration and availability according to operating system and SDK
requirements:

         Windows   Linux   macOS
   HAXM    yes      yes     yes
   WHPX    yes[1]    -       -
   KVM      -       yes      -
   HvF      -        -      yes[2]
  
[1] WHPX requires Windows 10 SDK version 10.0.17134.0 or later
[2] Hypervisor.Framework support is currently unimplemented

The header also exposes a fixed-size array of available platform factories on
virt86::PlatformFactories.
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

#include "virt86/util/host_info.hpp"

#if defined(VIRT86_HAXM_AVAILABLE)
#  include "virt86/haxm/haxm_platform.hpp"
#endif

#if defined(VIRT86_WHPX_AVAILABLE)
#  include "virt86/whpx/whpx_platform.hpp"
#endif

#if defined(VIRT86_KVM_AVAILABLE)
#  include "virt86/kvm/kvm_platform.hpp"
#endif

#if defined(VIRT86_HVF_AVAILABLE)
#  include "virt86/hvf/hvf_platform.hpp"
#endif

namespace virt86 {

using PlatformFactory = Platform& (*)();

inline PlatformFactory PlatformFactories[] = {
#if defined(VIRT86_HAXM_AVAILABLE)
    []() noexcept -> Platform& { return haxm::HaxmPlatform::Instance(); },
#endif

#if defined(VIRT86_WHPX_AVAILABLE)
    []() noexcept -> Platform& { return whpx::WhpxPlatform::Instance(); },
#endif

#if defined(VIRT86_KVM_AVAILABLE)
    []() noexcept -> Platform& { return kvm::KvmPlatform::Instance(); },
#endif

#if defined(VIRT86_HVF_AVAILABLE)
    []() noexcept -> Platform& { return hvf::HvFPlatform::Instance(); },
#endif
};

}
