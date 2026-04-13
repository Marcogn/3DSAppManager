#pragma once
/* ============================================================
   install.h — CIA file scanning and installation
   ============================================================ */
#include "types.h"

u64  getCIATitleID(const char *ciaPath);
int  scanDirectory(const char *path);
bool installCIA   (const char *ciaPath, FS_MediaType dest);

