/*
Implementation of the HAXM Platform internal implementation class.
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
#include "haxm_platform_impl.hpp"
#include "haxm_sys_platform.hpp"

namespace virt86::haxm {

HaxmVersion g_haxmVersion;

HaxmPlatformImpl::HaxmPlatformImpl() noexcept
    : m_haxVer({ 0 })
    , m_haxCaps({ 0 })
    , m_sys(std::make_unique<HaxmPlatformSysImpl>())
{
}

HaxmPlatformImpl::~HaxmPlatformImpl() noexcept {
}

PlatformInitStatus HaxmPlatformImpl::Initialize() noexcept {
    return m_sys->Initialize(&m_haxVer, &m_haxCaps);
}

HaxmVersion HaxmPlatformImpl::GetVersion() noexcept {
    return m_sys->GetVersion();
}

bool HaxmPlatformImpl::SetGlobalMemoryLimit(bool enabled, uint64_t limitMB) noexcept {
    return m_sys->SetGlobalMemoryLimit(enabled, limitMB);
}

}
