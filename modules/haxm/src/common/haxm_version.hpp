/*
Declares the HAXM version type.
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

namespace virt86::haxm {

// Undefine system-defined macros from <sys/sysmacros.h> / <sys/types.h> in Linux
#ifdef major
#undef major
#endif

#ifdef minor
#undef minor
#endif

union HaxmVersion {
    struct {
        uint16_t major, minor, build;
    };
    uint64_t u64;

    HaxmVersion() : u64(0) {}

    HaxmVersion(uint16_t major, uint16_t minor, uint16_t build)
        : major(major)
        , minor(minor)
        , build(build) {
    }

    bool operator==(const HaxmVersion& version) { return u64 == version.u64; }
    bool operator!=(const HaxmVersion& version) { return u64 != version.u64; }
    bool operator>=(const HaxmVersion& version) { return u64 >= version.u64; }
    bool operator<=(const HaxmVersion& version) { return u64 <= version.u64; }
    bool operator>(const HaxmVersion& version) { return u64 > version.u64; }
    bool operator<(const HaxmVersion& version) { return u64 < version.u64; }
};

// Global instance of the HAXM version loaded in the system.
extern HaxmVersion g_haxmVersion;

}
