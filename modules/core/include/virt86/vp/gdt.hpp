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

const uint8_t GDT_FL_GRANULARITY = (1 << 3);
const uint8_t GDT_FL_SIZE        = (1 << 2);
const uint8_t GDT_FL_LONG        = (1 << 1);

namespace virt86 {

struct GenericGDTDescriptor {
    union {
        struct {
            uint32_t _data1;
            uint32_t _data2    : 8,
                type           : 4,
                system         : 1,
                privilegeLevel : 2,
                present        : 1;
        } data;
        uint64_t descriptor;
    };
};

struct GDTDescriptor {
    union {
        struct {
            uint16_t limitLow : 16;
            uint16_t baseLow  : 16;
            uint8_t baseMid   : 8;
            union {
                struct {
                    uint8_t
                        accessed       : 1,
                                       : 1,
                                       : 1,
                        executable     : 1,  // 0 for data selector, 1 for code selector
                        system         : 1,  // = 1
                        privilegeLevel : 2,
                        present        : 1;
                } common;
                struct {
                    uint8_t
                        accessed       : 1,
                        readable       : 1,
                        conforming     : 1,
                        executable     : 1,  // 1 for code selector
                        system         : 1,  // = 1
                        privilegeLevel : 2,
                        present        : 1;
                } code;
                struct {
                    uint8_t
                        accessed       : 1,
                        writable       : 1,
                        direction      : 1,
                        executable     : 1,  // 0 for data selector
                        system         : 1,  // = 1
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

    uint32_t GetBase() const noexcept;
    uint32_t GetLimit() const noexcept;
    uint16_t GetAttributes() const noexcept;
};

struct LDTDescriptor {
    union {
        struct {
            uint16_t limitLow : 16;
            uint16_t baseLow : 16;
            uint8_t baseMid : 8;
            union {
                struct {
                    uint8_t
                        type : 4,  // = 0b0010
                        system : 1,  // = 0
                        privilegeLevel : 2,
                        present : 1;
                } data;
                uint8_t u8 : 8;
            } access;
            uint8_t limitHigh : 4;
            uint8_t flags : 4;
            uint8_t baseHigh : 8;

            uint32_t baseTop;
            uint32_t _reserved;
        } data;
        uint64_t descriptor[2];
    };

    void Set(uint64_t base, uint32_t limit, uint8_t access, uint8_t flags) noexcept;

    uint64_t GetBase() const noexcept;
    uint32_t GetLimit() const noexcept;
    uint16_t GetAttributes() const noexcept;
};

using TSSDescriptor = LDTDescriptor;  // data.access.data.type = 0b*0*1

// Call gates, interrupt gates and trap gates
struct NonTaskGateDescriptor {
    union {
        struct {
            uint16_t offsetLow;
            uint16_t csSelector;
            union {
                uint8_t paramCount : 5, : 3;           // call gate, reserved on IA-32e mode
                uint8_t interruptStackTable : 3, : 5;  // interrupt or trap gate, IA-32e mode only
            } params;
            union {
                struct {
                    uint8_t
                        type : 4,  // = 0b*100 (call gate), 0b*110 (interrupt gate), 0b*111 (trap gate)
                        system : 1,  // = 0
                        privilegeLevel : 2,
                        present : 1;
                } data;
                uint8_t u8;
            } access;
            uint16_t offsetHigh;

            uint32_t offsetTop;
            uint32_t _reserved;  // bits 8 through 12 must be zero on call gates
        } data;
        uint64_t descriptor[2];
    };

    void SetOffset(const uint64_t offset) noexcept;
    uint64_t GetOffset() const noexcept;
};

struct TaskGateDescriptor {
    union {
        struct {
            uint16_t _reserved1;
            uint16_t tssSelector;
            uint8_t _reserved2;
            union {
                struct {
                    uint8_t
                        type : 4,  // = 0b0101
                        system : 1,  // = 0
                        privilegeLevel : 2,
                        present : 1;
                } data;
                uint8_t u8;
            } access;
            uint16_t _reserved3;
        } data;
        uint64_t descriptor;
    };
};

union GDTEntry {
    GenericGDTDescriptor generic;
    GDTDescriptor gdt;
    LDTDescriptor ldt;
    TSSDescriptor tss;
    NonTaskGateDescriptor nonTaskGate;
    TaskGateDescriptor taskGate;
};

}
