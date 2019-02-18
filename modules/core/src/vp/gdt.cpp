/*
Global Descriptor Table method implementations.
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
#include "virt86/vp/gdt.hpp"

namespace virt86 {

void GDTEntry::Set(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
	this->descriptor = 0ULL;

	this->data.baseLow = base & 0xFFFF;
	this->data.baseMid = (base >> 16) & 0xFF;
	this->data.baseHigh = (base >> 24);

	this->data.limitLow = limit & 0xFFFF;
	this->data.limitHigh = (limit >> 16) & 0xF;
	
	this->data.access.u8 = access;
	this->data.flags = flags & 0xF;
}

uint32_t GDTEntry::GetBase() {
    return ((data.baseLow) | (data.baseMid << 16) | (data.baseHigh << 24));
}

uint32_t GDTEntry::GetLimit() {
    uint32_t limit = ((data.limitLow) | (data.limitHigh << 16));
    // If we use 4 KB pages, extend the limit to reflect that
    if (data.flags & GDT_FL_GRANULARITY) {
        limit = (limit << 12) | 0xfff;
    }
    return limit;
}

}
