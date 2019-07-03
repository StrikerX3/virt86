/*
HAXM system-based platform  implementation for Windows.
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
#include "haxm_sys_platform.hpp"

namespace virt86::haxm {

HaxmPlatformSysImpl::HaxmPlatformSysImpl() noexcept
    : m_handle(INVALID_HANDLE_VALUE)
{
}

HaxmPlatformSysImpl::~HaxmPlatformSysImpl() noexcept {
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

PlatformInitStatus HaxmPlatformSysImpl::Initialize(hax_module_version *haxVer, hax_capabilityinfo *haxCaps) noexcept {
    // Open the device
    m_handle = CreateFileW(L"\\\\.\\HAX", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (m_handle == INVALID_HANDLE_VALUE) {
        return PlatformInitStatus::Unavailable;
    }

    DWORD returnSize;
    BOOLEAN bResult;

    // Retrieve version
    bResult = DeviceIoControl(m_handle,
        HAX_IOCTL_VERSION,
        NULL, 0,
        haxVer, sizeof(hax_module_version),
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return PlatformInitStatus::Failed;
    }

    // Retrieve capabilities
    bResult = DeviceIoControl(m_handle,
        HAX_IOCTL_CAPABILITY,
        NULL, 0,
        haxCaps, sizeof(hax_capabilityinfo),
        &returnSize,
        (LPOVERLAPPED)NULL);
    if (!bResult) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return PlatformInitStatus::Failed;
    }

    return PlatformInitStatus::OK;
}

HaxmVersion HaxmPlatformSysImpl::GetVersion() noexcept {
    auto opt_ver = sys::windows::getDriverVersion(L"IntelHaxm.sys");
    if (!opt_ver) {
        return { 0, 0, 0 };
    }

    auto ver = *opt_ver;
    return { ver.major, ver.minor, ver.build };
}

bool HaxmPlatformSysImpl::SetGlobalMemoryLimit(bool enabled, uint64_t limitMB) noexcept {
    DWORD returnSize;

    hax_set_memlimit memlimit;
    memlimit.enable_memlimit = enabled;
    memlimit.memory_limit = enabled ? limitMB : 0;
    return DeviceIoControl(m_handle,
        HAX_IOCTL_SET_MEMLIMIT,
        &memlimit, sizeof(memlimit),
        NULL, 0,
        &returnSize,
        (LPOVERLAPPED)NULL) == TRUE;
}

}
