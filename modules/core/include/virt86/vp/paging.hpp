/*
Defines data structures related to x86 paging: PTE, PDE, PDPTE, PML4E.
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

namespace virt86 {

#define ENABLE_PAGING_ENTRY(x)  \
template<>                           \
struct EnablePagingEntry<x> {   \
    static const bool enable = true; \
};

template<typename Entry>
struct EnablePagingEntry {
    static const bool enable = false;
};

/**
 * PTE for 32-bit paging mode.
 */
struct PTE32 {
    uint32_t
        valid           : 1,
        write           : 1,
        owner           : 1,
        writeThrough    : 1,
        cacheDisable    : 1,
        accessed        : 1,
        dirty           : 1,
        PAT             : 1,
        global          : 1,
        guard           : 1,
        persist         : 1,
                        : 1,
        pageFrameNumber : 20;
};
ENABLE_PAGING_ENTRY(PTE32)

/**
 * PDE for 32-bit paging mode.
 */
union PDE32 {
    // Basic PDE
    struct {
        uint32_t
            valid           : 1,
            write           : 1,
            owner           : 1,
            writeThrough    : 1,
                            : 1,
            accessed        : 1,
            dirty           : 1,
            largePage       : 1,
            global          : 1,
            guard           : 1,
            persist         : 1,
                            : 1,
            pageFrameNumber : 20;
    };

    // PDE for 4 KiB pages
    struct {
        uint32_t
            valid           : 1, // = 1
            write           : 1,
            owner           : 1,
            writeThrough    : 1,
            cacheDisable    : 1,
            accessed        : 1,
            dirty           : 1,
            largePage       : 1, // = 0
            global          : 1,
            guard           : 1,
            persist         : 1,
                            : 1,
            pageFrameNumber : 20;
    } table;

    // PDE for 4 MiB pages
    struct {
        uint32_t
            valid           : 1, // = 1
            write           : 1,
            owner           : 1,
            writeThrough    : 1,
            cacheDisable    : 1,
            accessed        : 1,
            dirty           : 1,
            largePage       : 1, // = 1
            global          : 1,
            guard           : 1,
            persist         : 1,
                            : 1,
            PAT             : 1,
            addrHigh        : 8,
                            : 1,
            addrLow         : 10;
    } large;
};
ENABLE_PAGING_ENTRY(PDE32)

/**
 * PTE for PAE and 4-level paging modes.
 */
struct PTE64 {
    uint64_t
        valid           : 1,
        write           : 1,
        owner           : 1,
        writeThrough    : 1,
        cacheDisable    : 1,
        accessed        : 1,
        dirty           : 1,
        PAT             : 1,
        global          : 1,
        guard           : 1,
        persist         : 1,
                        : 1,
        address         : 40,
                        : 7,
        protectionKey   : 4,
        executeDisable  : 1;
};
ENABLE_PAGING_ENTRY(PTE64)

/**
 * PDE for PAE and 4-level paging modes.
 */
union PDE64 {
    // Basic PDE of PAE and 4-level paging modes.
    struct {
        uint64_t
            valid           : 1,
            write           : 1,
            owner           : 1,
            writeThrough    : 1,
            cacheDisable    : 1,
            accessed        : 1,
                            : 1,
            largePage       : 1,
                            : 1,
                            : 3,
                            : 51,
            executeDisable  : 1;
    };

    // PDE for 4 KiB pages of PAE and 4-level paging modes.
    struct {
        uint64_t
            valid           : 1, // = 1
            write           : 1,
            owner           : 1,
            writeThrough    : 1,
            cacheDisable    : 1,
            accessed        : 1,
                            : 1,
            largePage       : 1, // = 0
                            : 1,
                            : 3,
            address         : 40,
                            : 7,
            protectionKey   : 4,
            executeDisable  : 1;
    } table;

    // PDE for 2 MiB pages of PAE and 4-level paging modes.
    struct {
        uint64_t
            valid           : 1, // = 1
            write           : 1,
            owner           : 1,
            writeThrough    : 1,
            cacheDisable    : 1,
            accessed        : 1,
            dirty           : 1,
            largePage       : 1, // = 1
            global          : 1,
                            : 3,
            PAT             : 1,
                            : 8,
            address         : 31,
                            : 7,
            protectionKey   : 4,
            executeDisable  : 1;
    } large;
};
ENABLE_PAGING_ENTRY(PDE64)

/**
 * PDPTE for PAE and 4-level paging modes.
 */
union PDPTE {
    // Basic PDPTE for 4-level paging mode
    struct {
        uint64_t
            valid          : 1,
            write          : 1,
            owner          : 1,
            writeThrough   : 1,
            cacheDisable   : 1,
            accessed       : 1,
                           : 1,
            largePage      : 1,
                           : 4,
                           : 40,
                           : 11,
            executeDisable : 1;
    };

    // PDPTE for PAE and page directory level of 4-level paging mode
    struct {
        uint64_t
            valid          : 1, // = 1
            write          : 1,
            owner          : 1,
            writeThrough   : 1,
            cacheDisable   : 1,
            accessed       : 1,
                           : 1,
            largePage      : 1, // = 0
                           : 4,
            address        : 40,
                           : 11,
            executeDisable : 1;
    } table;

    // PDPTE for 1 GB page of 4-level paging mode
    struct {
        uint64_t
            valid          : 1, // = 1
            write          : 1,
            owner          : 1,
            writeThrough   : 1,
            cacheDisable   : 1,
            accessed       : 1,
            dirty          : 1,
            largePage      : 1, // = 1
            global         : 1,
                           : 3,
            PAT            : 1, 
                           : 17,
            address        : 22,
                           : 7,
            protectionKey  : 4,
            executeDisable : 1;
    } large;
};
ENABLE_PAGING_ENTRY(PDPTE)

/**
 * PML4E for 4-level paging mode.
 */
struct PML4E {
    uint64_t
        valid          : 1,
        write          : 1,
        owner          : 1,
        writeThrough   : 1,
        cacheDisable   : 1,
        accessed       : 1,
                       : 1,
                       : 1,
                       : 4,
        address        : 40,
                       : 11,
        executeDisable : 1;
};
ENABLE_PAGING_ENTRY(PML4E)

#undef ENABLE_PAGING_ENTRY

}
