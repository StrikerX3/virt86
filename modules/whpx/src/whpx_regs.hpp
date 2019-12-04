/*
Declares helper functions for translating between virt86 and Windows Hypervisor
Platform data structures.
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
#include "virt86/vp/status.hpp"
#include "whpx_defs.hpp"

#include <cstdint>

namespace virt86::whpx {

VPOperationStatus TranslateRegisterName(const Reg reg, WHV_REGISTER_NAME& name) noexcept;
RegValue TranslateRegisterValue(const Reg reg, const WHV_REGISTER_VALUE& value) noexcept;
void TranslateRegisterValue(const Reg reg, const RegValue& value, WHV_REGISTER_VALUE& output) noexcept;
bool TranslateMSR(uint64_t msr, WHV_REGISTER_NAME& reg) noexcept;

}
