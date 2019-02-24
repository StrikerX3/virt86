/*
Specifies I/O callback function types.
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

#include <cstdint>
#include <cstddef>

namespace virt86 {

using IOReadFunc_t = uint32_t(*)(void *context, uint16_t port, size_t size);
using IOWriteFunc_t = void(*)(void *context, uint16_t port, size_t size, uint32_t value);

using MMIOReadFunc_t = uint64_t(*)(void *context, uint64_t address, size_t size);
using MMIOWriteFunc_t = void(*)(void *context, uint64_t address, size_t size, uint64_t value);

struct IOHandlers {
    IOReadFunc_t IOReadFunc;
    IOWriteFunc_t IOWriteFunc;

    MMIOReadFunc_t MMIOReadFunc;
    MMIOWriteFunc_t MMIOWriteFunc;

    uint32_t IORead(uint16_t port, size_t size) const noexcept { return IOReadFunc(context, port, size); }
    void IOWrite(uint16_t port, size_t size, uint32_t value) const noexcept { return IOWriteFunc(context, port, size, value); }

    uint64_t MMIORead(uint64_t address, size_t size) const noexcept { return MMIOReadFunc(context, address, size); }
    void MMIOWrite(uint64_t address, size_t size, uint64_t value) const noexcept { return MMIOWriteFunc(context, address, size, value); }

    void *context;
};

}
