/*
Declares the Platform internal implementation class for HAXM.
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
#include "interface/hax_interface.hpp"
#include "haxm_version.hpp"

#include <memory>

namespace virt86::haxm {

extern HaxmVersion g_haxmVersion;

class HaxmPlatformSysImpl;

class HaxmPlatformImpl {
public:
    HaxmPlatformImpl() noexcept;
    ~HaxmPlatformImpl() noexcept;

    // Prevent copy construction and copy assignment
    HaxmPlatformImpl(const HaxmPlatformImpl&) = delete;
    HaxmPlatformImpl& operator=(const HaxmPlatformImpl&) = delete;

    // Prevent move construction and move assignment
    HaxmPlatformImpl(HaxmPlatformImpl&&) = delete;
    HaxmPlatformImpl&& operator=(HaxmPlatformImpl&&) = delete;

    // Disallow taking the address
    HaxmPlatformImpl *operator&() = delete;

    PlatformInitStatus Initialize() noexcept;

    HaxmVersion GetVersion() noexcept;
    bool SetGlobalMemoryLimit(bool enabled, uint64_t limitMB) noexcept;

    hax_module_version m_haxVer;
    hax_capabilityinfo m_haxCaps;

    // OS-specific implementation
    std::unique_ptr<HaxmPlatformSysImpl> m_sys;
};

struct HaxmPlatform::Delegate {
    HaxmPlatformImpl impl;
};

}
