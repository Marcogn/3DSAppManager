/* ============================================================
   utils.c — general-purpose utility functions
   ============================================================ */
#include "utils.h"

void createDirectory(const char *path) {
    char tmp[256];
    char *p = NULL;
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = 0; mkdir(tmp, 0777); *p = '/'; }
    }
    mkdir(tmp, 0777);
}

void sanitizeName(char *name) {
    if (!name) return;
    char *src = name, *dst = name;
    while (*src) {
        if ((unsigned char)*src >= 32 && (unsigned char)*src <= 126) {
            if (*src != '|' && *src != '<' && *src != '>' && *src != '"' &&
                *src != '\\' && *src != '/' && *src != ':' && *src != '*' && *src != '?')
                *dst++ = *src;
            else
                *dst++ = ' ';
        }
        src++;
    }
    *dst = '\0';
    while (dst > name && *(dst - 1) == ' ') *(--dst) = '\0';
    if (name[0] == '\0') strcpy(name, "Unknown Title");
}

void formatSize(u64 size, char *buf, size_t bufSize) {
    if (size >= (u64)1024 * 1024 * 1024)
        snprintf(buf, bufSize, "%.2f GB", (float)size / (1024.0f * 1024.0f * 1024.0f));
    else if (size >= 1024 * 1024)
        snprintf(buf, bufSize, "%.2f MB", (float)size / (1024.0f * 1024.0f));
    else if (size >= 1024)
        snprintf(buf, bufSize, "%.2f KB", (float)size / 1024.0f);
    else
        snprintf(buf, bufSize, "%llu B", (unsigned long long)size);
}

