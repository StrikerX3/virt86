#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <optional>

#pragma comment(lib, "version.lib")

namespace virt86::sys::windows {

union VersionInfo {
    struct {
        uint16_t major, minor, build, revision;
    };
    uint64_t ver;

    VersionInfo() : ver(0) {}

    VersionInfo(uint16_t major, uint16_t minor, uint16_t build, uint16_t revision)
        : major(major)
        , minor(minor)
        , build(build)
        , revision(revision)
    {}

    VersionInfo(VS_FIXEDFILEINFO& fileInfo) {
        major = (fileInfo.dwFileVersionMS >> 16) & 0xFFFF;
        minor = (fileInfo.dwFileVersionMS) & 0xFFFF;
        build = (fileInfo.dwFileVersionLS >> 16) & 0xFFFF;
        revision = (fileInfo.dwFileVersionLS) & 0xFFFF;
    }

    bool operator==(const VersionInfo& versionInfo) const { return ver == versionInfo.ver; }
    bool operator!=(const VersionInfo& versionInfo) const { return ver != versionInfo.ver; }
    bool operator>=(const VersionInfo& versionInfo) const { return ver >= versionInfo.ver; }
    bool operator<=(const VersionInfo& versionInfo) const { return ver <= versionInfo.ver; }
    bool operator>(const VersionInfo& versionInfo) const { return ver > versionInfo.ver; }
    bool operator<(const VersionInfo& versionInfo) const { return ver < versionInfo.ver; }
};

std::optional<VersionInfo> getModuleVersion(HMODULE hModule);
std::optional<VersionInfo> getDriverVersion(std::wstring driverFileName);

}
