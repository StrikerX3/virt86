/*
Defines the Global Descriptor Table data structure.
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

#define GDT_FL_GRANULARITY  (1 << 3)
#define GDT_FL_SIZE         (1 << 2)
#define GDT_FL_LONG         (1 << 1)

namespace virt86 {

struct GDTEntry {
    union {
        struct {
            uint16_t limitLow : 16;
            uint16_t baseLow  : 16;
            uint8_t baseMid   : 8;
            union {
                struct {
                    uint8_t
                        accessed       : 1,
                        readable       : 1,
                        conforming     : 1,
                        executable     : 1,  // 1 for code selector
                        type           : 1,
                        privilegeLevel : 2,
                        present        : 1;
                } code;
                struct {
                    uint8_t
                        accessed       : 1,
                        writable       : 1,
                        direction      : 1,
                        executable     : 1,  // 0 for data selector
                        type           : 1,
                        privilegeLevel : 2,
                        present        : 1;
                } data;
                uint8_t u8 : 8;
            } access;
            uint8_t limitHigh : 4;
            uint8_t flags : 4;
            uint8_t baseHigh : 8;
        } data;
        uint64_t descriptor;
    };

    void Set(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) noexcept;

    uint32_t GetBase() noexcept;
    uint32_t GetLimit() noexcept;
};

using LDTEntry = GDTEntry;

}
