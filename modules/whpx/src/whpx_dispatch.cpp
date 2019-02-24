/*
Loads the Windows Hypervisor Platform libraries dynamically and builds the
function dispatch table.
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
#include "whpx_dispatch.hpp"

namespace virt86::whpx {

#define LOAD_LIB(name) do { \
    m_h##name = LoadLibrary(#name ".dll"); \
    if (m_h##name == NULL) { \
        goto fail; \
    } \
} while (0)

#define LOAD_FUNC(returnType, name, parameters) \
    name = (name##_t) GetProcAddress(hmodule, #name); \
    if (!name) { \
        goto fail; \
    }

bool WhpxDispatch::Load() noexcept {
    if (m_loaded) {
        return true;
    }
    
    LOAD_LIB(WinHvPlatform);
    LOAD_LIB(WinHvEmulation);

    HMODULE hmodule;

    hmodule = m_hWinHvPlatform; WHPX_PLATFORM_FUNCTIONS(LOAD_FUNC);
    hmodule = m_hWinHvEmulation; WHPX_EMULATION_FUNCTIONS(LOAD_FUNC);

    m_loaded = true;
    return true;

fail:
    if (m_hWinHvPlatform) {
        FreeLibrary(m_hWinHvPlatform);
        m_hWinHvPlatform = NULL;
    }
    if (m_hWinHvEmulation) {
        FreeLibrary(m_hWinHvEmulation);
        m_hWinHvEmulation = NULL;
    }

    return false;
}

}
