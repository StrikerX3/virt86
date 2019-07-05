/*
HAXM system-based platform implementation for macOS.
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

#include "virt86/platform/platform.hpp"

#include "interface/hax_interface.hpp"
#include "haxm_version.hpp"

namespace virt86::haxm {

class HaxmPlatformSysImpl {
public:
    HaxmPlatformSysImpl() noexcept;
    ~HaxmPlatformSysImpl() noexcept;

    // Prevent copy construction and copy assignment
    HaxmPlatformSysImpl(const HaxmPlatformSysImpl&) = delete;
    HaxmPlatformSysImpl& operator=(const HaxmPlatformSysImpl&) = delete;

    // Prevent move construction and move assignment
    HaxmPlatformSysImpl(HaxmPlatformSysImpl&&) = delete;
    HaxmPlatformSysImpl&& operator=(HaxmPlatformSysImpl&&) = delete;

    // Disallow taking the address
    HaxmPlatformSysImpl *operator&() = delete;

    PlatformInitStatus Initialize(hax_module_version *haxVer, hax_capabilityinfo *haxCaps) noexcept;

    HaxmVersion GetVersion() noexcept;
    bool SetGlobalMemoryLimit(bool enabled, uint64_t limitMB) noexcept;

    const int FileDescriptor() const noexcept { return m_fd; }

private:
    int m_fd;
};

}
