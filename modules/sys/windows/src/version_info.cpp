#include "virt86/sys/windows/version_info.hpp"

namespace virt86::sys::windows {

std::optional<VersionInfo> getModuleVersion(HMODULE hModule) {
    std::optional<VersionInfo> result = std::nullopt;

    if (hModule != NULL) {
        HRSRC hVersion = FindResource(hModule, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
        if (hVersion != NULL) {
            HGLOBAL hGlobal = LoadResource(hModule, hVersion);
            if (hGlobal != NULL) {
                LPVOID versionInfo = LockResource(hGlobal);
                if (versionInfo != NULL) {
                    DWORD dwSize = SizeofResource(hModule, hVersion);
                    LPVOID localVersionInfo = new char[(int)dwSize * 2];
                    memcpy(localVersionInfo, versionInfo, (int)dwSize);

                    LPVOID retbuf;
                    UINT vLen;
                    if (VerQueryValueW(localVersionInfo, L"\\", &retbuf, &vLen)) {
                        VS_FIXEDFILEINFO verInfo = { 0 };
                        memcpy(&verInfo, (char*)retbuf, sizeof(verInfo));
                        if (verInfo.dwSignature == 0xfeef04bd) {
                            result = verInfo;
                        }
                    }

                    delete[] localVersionInfo;
                }

                UnlockResource(hGlobal);
                FreeResource(hGlobal);
            }
        }
    }

    return result;
}

std::optional<VersionInfo> getDriverVersion(std::wstring driverFileName) {
    std::optional<VersionInfo> result = std::nullopt;

    // Get Windows installation directory
    size_t pathSize = MAX_PATH;
    WCHAR *windowsPath = new WCHAR[pathSize];

    UINT len;
    for (;;) {
        len = GetWindowsDirectoryW(windowsPath, pathSize);
        if (len == 0) {
            delete[] windowsPath;
            return std::nullopt;
        }
        if (len <= pathSize) {
            break;
        }
        pathSize += MAX_PATH;
        delete[] windowsPath;
        windowsPath = new WCHAR[pathSize];
    }

    // Build path to driver: <Windows>\System32\driver\<driver file name>
    std::wstring driverPath(windowsPath);
    driverPath += L"\\System32\\drivers\\" + driverFileName;

    DWORD verHandle;
    DWORD verSize = GetFileVersionInfoSizeW(driverPath.c_str(), &verHandle);
    if (verSize != NULL) {
        LPSTR verData = new char[verSize];
        if (GetFileVersionInfoW(driverPath.c_str(), verHandle, verSize, verData)) {
            LPVOID lpBuffer = NULL;
            UINT size = 0;
            if (VerQueryValueW(verData, L"\\", &lpBuffer, &size)) {
                if (size) {
                    VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
                    if (verInfo->dwSignature == 0xfeef04bd) {
                        result = *verInfo;
                    }
                }
            }
        }
        delete[] verData;
    }

    return result;
}

}
