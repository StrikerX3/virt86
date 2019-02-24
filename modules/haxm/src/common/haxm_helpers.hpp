/*
Declares helper functions for translating between virt86 and HAXM data
structures.
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

#include "virt86/vp/regs.hpp"

namespace virt86::haxm {

void LoadSegment(RegValue& value, const segment_desc_t *segment) noexcept;
void StoreSegment(const RegValue& value, segment_desc_t *segment) noexcept;

void LoadTable(RegValue& value, const segment_desc_t *table) noexcept;
void StoreTable(const RegValue& value, segment_desc_t *table) noexcept;
    
void LoadSTRegister(RegValue& value, uint8_t index, const fx_layout& fpuRegs) noexcept;
void StoreSTRegister(const RegValue& value, uint8_t index, fx_layout& fpuRegs) noexcept;

void LoadMMRegister(RegValue& value, uint8_t index, const fx_layout& fpuRegs) noexcept;
void StoreMMRegister(const RegValue& value, uint8_t index, fx_layout& fpuRegs) noexcept;

bool LoadXMMRegister(RegValue& value, uint8_t index, const fx_layout& fpuRegs) noexcept;
bool StoreXMMRegister(const RegValue& value, uint8_t index, fx_layout& fpuRegs) noexcept;

}
