/*
Various utility functions for dealing with Linux files and kernel modules.
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
#include <cstdlib>

#ifndef MODULE_DIR
#define MODULE_DIR "/lib/modules"
#endif

inline int __attribute__ ((pure)) native_endianness() {
    /* Encoding the endianness enums in a string and then reading that
     * string as a 32-bit int, returns the correct endianness automagically.
     */
    return (char) *((uint32_t *) ("\1\0\0\2"));
}

static inline void __swap_bytes(const void *src, void *dest, unsigned int size) {
    unsigned int i;
    for (i = 0; i < size; i++) {
        ((unsigned char *) dest)[i] = ((unsigned char *) src)[size - i - 1];
    }
}

template<typename T>
inline T endianSwap(T data, bool swap) {
    if (swap) {
        T result = data;
        __swap_bytes(&data, &result, sizeof(T));
        return result;
    }
    return data;
}

void getModulesPath(char **path);
void *readFile(const char *filename, unsigned long *size);
char *getModInfoParam(char *modinfo, size_t modinfoSize, const char *param);
char *mmapKernelModuleFile(const char *name, size_t *size);
