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
#include "virt86/sys/linux/util.h"

#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

void getModulesPath(char **path) {
    struct utsname kernel{};
    uname(&kernel);

    asprintf(path, "%s/%s", MODULE_DIR, kernel.release);
}

void *readFile(const char *filename, unsigned long *size) {
    int fd = open(filename, O_RDONLY, 0);
    if (fd < 0) {
        return nullptr;
    }

    struct stat st{};
    int ret = fstat(fd, &st);
    if (ret < 0) {
        close(fd);
        return nullptr;
    }

    *size = st.st_size;
    void *data = mmap(nullptr, *size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        data = nullptr;
    }
    close(fd);

    return data;
}

char *getModInfoParam(char *modinfo, size_t modinfoSize, const char *param) {
    auto paramLen = strlen(param);

    auto sectionEnd = modinfo + modinfoSize;
    for (auto p = modinfo; p < sectionEnd;) {
        auto eq = strchr(p, '=');
        if (eq != nullptr && eq - p == paramLen && strncmp(p, param, paramLen) == 0) {
            char *result;
            asprintf(&result, "%s", eq + 1);
            return result;
        }
        p += strlen(p) + 1;
    }

    return nullptr;
}

static char *nextLine(char *p, const char *end) {
    char *eol = (char *) memchr(p, '\n', end - p);
    if (eol) {
        return eol + 1;
    }
    return (char *) end + 1;
}

/* - and _ are equivalent, and expect suffix. */
static int name_matches(const char *line, const char *end, const char *modname) {
    unsigned int i;
    char *p;

    /* Ignore comment lines */
    if (line[strspn(line, "\t ")] == '#')
        return 0;

    /* Find last / before colon. */
    p = (char *) memchr(line, ':', end - line);
    if (!p)
        return 0;
    while (p > line) {
        if (*p == '/') {
            p++;
            break;
        }
        p--;
    }

    for (i = 0; modname[i]; i++) {
        /* Module names can't have colons. */
        if (modname[i] == ':')
            continue;
        if (modname[i] == p[i])
            continue;
        if (modname[i] == '_' && p[i] == '-')
            continue;
        if (modname[i] == '-' && p[i] == '_')
            continue;
        return 0;
    }
    /* Must match all the way to the extension */
    return (p[i] == '.');
}

char *mmapKernelModuleFile(const char *name, size_t *size) {
    char *modulesPath;
    char *moduleDepPath;
    getModulesPath(&modulesPath);
    asprintf(&moduleDepPath, "%s/%s", modulesPath, "modules.dep");

    size_t depSize;
    char *data = (char *) readFile(moduleDepPath, &depSize);
    if (data == nullptr) {
        free(moduleDepPath);
        free(modulesPath);
        return nullptr;
    }
    free(moduleDepPath);

    for (char *p = data; p < data + depSize; p = nextLine(p, data + depSize)) {
        if (name_matches(p, data + depSize, name)) {
            const char *dir;
            if ('/' == p[0]) {
                dir = "";
            } else {
                dir = modulesPath;
            }

            int namelen = strcspn(p, ":");
            char *filename;
            if (strlen(dir)) {
                asprintf(&filename, "%s/%s", dir, p);
                filename[namelen + strlen(dir) + 1] = '\0';
            } else {
                filename = strndup(p, namelen);
            }
            munmap(data, depSize);
            free(modulesPath);

            char *kmData = (char *) readFile(filename, size);
            free(filename);

            return kmData;
        }
    }
    munmap(data, depSize);
    free(modulesPath);
    return nullptr;
}