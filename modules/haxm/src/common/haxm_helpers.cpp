/*
Implementations of helper functions for dealing with HAXM data structures.
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
#include "haxm_vp.hpp"
#include "haxm_vm.hpp"

namespace virt86::haxm {

void LoadSegment(RegValue& value, const segment_desc_t *segment) noexcept {
    value.segment.selector = segment->selector;
    value.segment.base = segment->base;
    value.segment.limit = segment->limit;
    value.segment.attributes.u16 = segment->ar;
}

void StoreSegment(const RegValue& value, segment_desc_t *segment) noexcept {
    segment->selector = value.segment.selector;
    segment->base = value.segment.base;
    segment->limit = value.segment.limit;
    segment->ar = value.segment.attributes.u16;
}

void LoadTable(RegValue& value, const segment_desc_t *table) noexcept {
    value.table.base = table->base;
    value.table.limit = table->limit;
}

void StoreTable(const RegValue& value, segment_desc_t *table) noexcept {
    table->base = value.table.base;
    table->limit = value.table.limit;
}

void LoadSTRegister(RegValue& value, uint8_t index, const fx_layout& fpuRegs) noexcept {
    value.st.significand = *reinterpret_cast<const uint64_t *>(&fpuRegs.st_mm[index][0]);
    value.st.exponentSign = *reinterpret_cast<const uint16_t *>(&fpuRegs.st_mm[index][8]);
}

void StoreSTRegister(const RegValue& value, uint8_t index, fx_layout& fpuRegs) noexcept {
    *reinterpret_cast<uint64_t *>(&fpuRegs.st_mm[index][0]) = value.st.significand;
    *reinterpret_cast<uint16_t *>(&fpuRegs.st_mm[index][8]) = value.st.exponentSign;
}

void LoadMMRegister(RegValue& value, uint8_t index, const fx_layout& fpuRegs) noexcept {
    value.mm.i64[0] = *reinterpret_cast<const int64_t *>(&fpuRegs.st_mm[index][0]);
}

void StoreMMRegister(const RegValue& value, uint8_t index, fx_layout& fpuRegs) noexcept {
    *reinterpret_cast<int64_t *>(&fpuRegs.st_mm[index][0]) = value.mm.i64[0];
}

bool LoadXMMRegister(RegValue& value, uint8_t index, const fx_layout& fpuRegs) noexcept {
    if (index < 8) {
        value.xmm.i64[0] = *reinterpret_cast<const int64_t *>(&fpuRegs.mmx_1[index][0]);
        value.xmm.i64[1] = *reinterpret_cast<const int64_t *>(&fpuRegs.mmx_1[index][8]);
        return true;
    }
    if (index < 16) {
        value.xmm.i64[0] = *reinterpret_cast<const int64_t *>(&fpuRegs.mmx_2[index - 8][0]);
        value.xmm.i64[1] = *reinterpret_cast<const int64_t *>(&fpuRegs.mmx_2[index - 8][8]);
        return true;
    }
    return false;
}

bool StoreXMMRegister(const RegValue& value, uint8_t index, fx_layout& fpuRegs) noexcept {
    if (index < 8) {
        *reinterpret_cast<int64_t *>(&fpuRegs.mmx_1[index][0]) = value.xmm.i64[0];
        *reinterpret_cast<int64_t *>(&fpuRegs.mmx_1[index][8]) = value.xmm.i64[1];
        return true;
    }
    if (index < 16) {
        *reinterpret_cast<int64_t *>(&fpuRegs.mmx_2[index - 8][0]) = value.xmm.i64[0];
        *reinterpret_cast<int64_t *>(&fpuRegs.mmx_2[index - 8][8]) = value.xmm.i64[1];
        return true;
    }
    return false;
}

}
