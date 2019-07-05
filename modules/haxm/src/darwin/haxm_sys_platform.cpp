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
#include "haxm_sys_platform.hpp"

#include "virt86/sys/darwin/kext.hpp"

#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

using namespace virt86::sys::darwin;

namespace virt86::haxm {

HaxmPlatformSysImpl::HaxmPlatformSysImpl() noexcept
    : m_fd(-1)
{
}

HaxmPlatformSysImpl::~HaxmPlatformSysImpl() noexcept {
    if (m_fd > 0) {
        close(m_fd);
        m_fd = -1;
    }
}

PlatformInitStatus HaxmPlatformSysImpl::Initialize(hax_module_version *haxVer, hax_capabilityinfo *haxCaps) noexcept {
    // Open the device
    m_fd = open("/dev/HAX", O_RDWR);
    if (m_fd < 0) {
        return PlatformInitStatus::Unavailable;
    }

    // Retrieve version
    int result = ioctl(m_fd, HAX_IOCTL_VERSION, haxVer);
    if (result < 0) {
        close(m_fd);
        m_fd = -1;
        return PlatformInitStatus::Failed;
    }

    // Retrieve capabilities
    result = ioctl(m_fd, HAX_IOCTL_CAPABILITY, haxCaps);
    if (result < 0) {
        close(m_fd);
        m_fd = -1;
        return PlatformInitStatus::Failed;
    }

    return PlatformInitStatus::OK;
}

HaxmVersion HaxmPlatformSysImpl::GetVersion() noexcept {
    HaxmVersion version = {0, 0, 0};

    // Get version string from kextstat
    char *ver = getKextVersion("com.intel.kext.intelhaxm");
    if (ver == nullptr) {
        return version;
    }

    // Parse version string
    char *vNext;
    version.major = strtol(ver, &vNext, 10);
    version.minor = strtol(vNext + 1, &vNext, 10);
    version.build = strtol(vNext + 1, &vNext, 10);

    // Release resources
    free(ver);
    return version;
}

bool HaxmPlatformSysImpl::SetGlobalMemoryLimit(bool enabled, uint64_t limitMB) noexcept {
    hax_set_memlimit memlimit;
    memlimit.enable_memlimit = enabled;
    memlimit.memory_limit = enabled ? limitMB : 0;
    return ioctl(m_fd, HAX_IOCTL_SET_MEMLIMIT, &memlimit) == 0;
}

}

