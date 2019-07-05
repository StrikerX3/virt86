/*
Basic ELF file reader declaration.
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

#include <elf.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "util.h"

/**
 * Parses basic information from an ELF file.
 *
 * An instance of this class may be retrieved with the Load static method,
 * which automatically determines the bit width of the file. If the ELF file
 * has an unknown format or cannot be read, returns nullptr.
 */
class Elf {
public:
    virtual ~Elf() noexcept;

    /**
     * Loads the ELF file from the specified block of memory.
     *
     * Returns nullptr if the data is not an ELF binary or has an unknown
     * format.
     */
    static Elf *Load(const char *data, const size_t size) noexcept;

    /**
     * Retrieves a pointer to the data from the specified section, or nullptr
     * if the section could not be found. The size of the section, if found, is
     * written to the size pointer.
     */
    virtual void *GetSectionData(const char *sectionName, size_t *size) noexcept = 0;

protected:
    Elf(const char *data, const size_t size, bool swapEndianness) noexcept;

    char *m_data;
    const size_t m_size;
    bool m_swapEndianness;

    template<typename T>
    T enSwap(T data) { return endianSwap(data, m_swapEndianness); }
};

class Elf32 : public Elf {
public:
    void *GetSectionData(const char *sectionName, size_t *size) noexcept override;

protected:
    Elf32(const char *data, const size_t size, bool swapEndianness) noexcept;

    Elf32_Shdr *GetSection(const uint32_t index) noexcept;

    Elf32_Ehdr *m_header;
    char *m_stringTable;

    friend class Elf;
};

class Elf64 : public Elf {
    void *GetSectionData(const char *sectionName, size_t *size) noexcept override;

protected:
    Elf64(const char *data, const size_t size, bool swapEndianness) noexcept;

    Elf64_Shdr *GetSection(const uint32_t index) noexcept;

    Elf64_Ehdr *m_header;
    char *m_stringTable;

    friend class Elf;
};
