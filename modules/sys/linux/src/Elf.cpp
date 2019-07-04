/*
Basic ELF file reader implementation.
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
#include "virt86/sys/linux/Elf.h"

Elf::Elf(const char *data, const size_t size, bool swapEndianness) noexcept
        : m_size(size), m_swapEndianness(swapEndianness) {
    m_data = (char *) malloc(size);
    memcpy(m_data, data, size);
}

Elf::~Elf() noexcept {
    free(m_data);
}

Elf *Elf::Load(const char *data, const size_t size) noexcept {
    if (size < EI_CLASS) {
        return nullptr;
    }

    auto ident = (const char *) data;
    if (memcmp(ident, ELFMAG, SELFMAG) != 0) {
        return nullptr;
    }

    if (ident[EI_DATA] == 0 || ident[EI_DATA] > 2) {
        return nullptr;
    }

    bool enSwap = (native_endianness() != ident[EI_DATA]);

    switch (ident[EI_CLASS]) {
        case ELFCLASS32: {
            return new Elf32(data, size, enSwap);
        }
        case ELFCLASS64: {
            return new Elf64(data, size, enSwap);
        }
        default:
            return nullptr;
    }
}

// ----- Elf32 ----------------------------------------------------------------

Elf32::Elf32(const char *data, const size_t size, bool swapEndianness) noexcept
        : Elf(data, size, swapEndianness) {
    m_header = (Elf32_Ehdr *) m_data;
    auto stringTableHeader = (Elf32_Shdr *) ((char *) m_data + enSwap(m_header->e_shoff) +
                                             enSwap(m_header->e_shstrndx) *
                                             enSwap(m_header->e_shentsize));
    m_stringTable = (char *) m_data + enSwap(stringTableHeader->sh_offset);
}

Elf32_Shdr *Elf32::GetSection(const uint32_t index) noexcept {
    return (Elf32_Shdr *) ((char *) m_data + enSwap(m_header->e_shoff) +
                           index * enSwap(m_header->e_shentsize));
}

void *Elf32::GetSectionData(const char *sectionName, size_t *size) noexcept {
    for (int i = 0; i < m_header->e_shnum; i++) {
        auto sectionHeader = GetSection(i);
        auto sectionNameStr = &m_stringTable[enSwap(sectionHeader->sh_name)];
        if (strcmp(sectionNameStr, sectionName) == 0) {
            *size = enSwap(sectionHeader->sh_size);
            return (char *) m_data + enSwap(sectionHeader->sh_offset);
        }
    }
    return nullptr;
}

// ----- Elf64 ----------------------------------------------------------------

Elf64::Elf64(const char *data, const size_t size, bool swapEndianness) noexcept
        : Elf(data, size, swapEndianness) {
    m_header = (Elf64_Ehdr *) m_data;
    auto stringTableHeader = (Elf64_Shdr *) ((char *) m_data + enSwap(m_header->e_shoff) +
                                             enSwap(m_header->e_shstrndx) *
                                             enSwap(m_header->e_shentsize));
    m_stringTable = (char *) m_data + enSwap(stringTableHeader->sh_offset);
}

Elf64_Shdr *Elf64::GetSection(const uint32_t index) noexcept {
    return (Elf64_Shdr *) ((char *) m_data + enSwap(m_header->e_shoff) +
                           index * enSwap(m_header->e_shentsize));
}

void *Elf64::GetSectionData(const char *sectionName, size_t *size) noexcept {
    for (int i = 0; i < m_header->e_shnum; i++) {
        auto sectionHeader = GetSection(i);
        auto sectionNameStr = &m_stringTable[enSwap(sectionHeader->sh_name)];
        if (strcmp(sectionNameStr, sectionName) == 0) {
            *size = enSwap(sectionHeader->sh_size);
            return (char *) m_data + enSwap(sectionHeader->sh_offset);
        }
    }
    return nullptr;
}
