/*
Interrupt Descriptor Table method implementations.
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
#include "virt86/vp/idt.hpp"

namespace virt86 {

void IDTEntry::Set(uint32_t offset, uint16_t selector, IDTType type, uint8_t attributes) noexcept {
    this->descriptor = 0ULL;

    this->data.offsetLow = offset & 0xFFFF;
    this->data.offsetHigh = (offset >> 16);

    this->data.selector = selector;
    
    this->data.type = static_cast<uint8_t>(type) & 0xF;
    this->data.storageSegment = attributes & 0x1;
    this->data.privilegeLevel = (attributes >> 1) & 0x3;
    this->data.present = (attributes >> 3) & 0x1;
}

uint32_t IDTEntry::GetOffset() noexcept {
    return static_cast<uint32_t>(data.offsetLow) | (static_cast<uint32_t>(data.offsetHigh) << 16);
}

void IDTEntry::SetOffset(uint32_t offset) noexcept {
    this->data.offsetLow = offset & 0xFFFF;
    this->data.offsetHigh = (offset >> 16);
}

}
