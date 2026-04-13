#pragma once
/* ============================================================
   utils.h — general-purpose utility functions
   ============================================================ */
#include "types.h"

void createDirectory(const char *path);
void sanitizeName(char *name);
void formatSize(u64 size, char *buf, size_t bufSize);

