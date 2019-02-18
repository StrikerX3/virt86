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

#include "virt86/haxm/haxm_platform.hpp"

#if defined(_WIN32)
#  if defined(VIRT86_WHPX_AVAILABLE)
#    include "virt86/whpx/whpx_platform.hpp"
#  endif
#elif defined(__linux__)
#  include "virt86/kvm/kvm_platform.hpp"
#elif defined(__APPLE__)
#  include "virt86/hvf/hvf_platform.hpp"
#endif

namespace virt86 {

using PlatformFactory = Platform& (*)();

inline PlatformFactory PlatformFactories[] = {
    []() -> Platform& { return haxm::HaxmPlatform::Instance(); },
#if defined(_WIN32)
#  if defined(VIRT86_WHPX_AVAILABLE)
    []() -> Platform& { return whpx::WhpxPlatform::Instance(); },
#  endif
#elif defined(__linux__)
    []() -> Platform& { return kvm::KvmPlatform::Instance(); },
#elif defined(__APPLE__)
    []() -> Platform& { return hvf::HvFPlatform::Instance(); },
#endif
};

}
