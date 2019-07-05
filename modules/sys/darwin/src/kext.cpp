/*
Basic macOS kext handling functions.
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
#include "virt86/sys/darwin/kext.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace virt86::sys::darwin {

char *getKextVersion(const char *kextName) {
    // Cheaty version: run kextstat command and look for the desired kext
    char *cmd;
    asprintf(&cmd, "/usr/sbin/kextstat | /usr/bin/grep -F %s", kextName);
    FILE *fp = popen(cmd, "r");
    free(cmd);

    // Read command output
    char result[256];
    fread(result, sizeof(result), 1, fp);
    pclose(fp);

    // Find kext name in the line
    char *p = strstr(result, kextName);
    if (p == NULL) {
        return NULL;
    }

    // Extract version, which comes after the kext name in parenthesis
    p = strchr(p, '(');
    if (p == NULL) {
        return NULL;
    }
    p++;

    char *end = strchr(p, ')');
    size_t verLen = end - p;
    
    // Allocate new string with a copy of the version
    char *ver = (char *)malloc(verLen + 1);
    strncpy(ver, p, verLen);
    ver[verLen] = '\0';

    return ver;
}

}
