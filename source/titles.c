/* ============================================================
   titles.c — title loading (optimized), sort/filter,
               backup-dir helpers, title metadata
   ============================================================ */
#include "titles.h"
#include "globals.h"
#include "utils.h"

/* Forward declaration — defined in draw.c (avoids circular include) */
void drawLoadingScreen(int current, int total, const char *status);

/* ---- Sort / Filter ---- */

int compareTitlesByName(const void *a, const void *b) {
    return strcasecmp(((TitleInfo*)a)->name, ((TitleInfo*)b)->name);
}
int compareTitlesBySize(const void *a, const void *b) {
    const TitleInfo *ta = (const TitleInfo*)a, *tb = (const TitleInfo*)b;
    if (ta->size > tb->size) return -1;
    if (ta->size < tb->size) return  1;
    return 0;
}
int compareTitlesByID(const void *a, const void *b) {
    const TitleInfo *ta = (const TitleInfo*)a, *tb = (const TitleInfo*)b;
    if (ta->titleID < tb->titleID) return -1;
    if (ta->titleID > tb->titleID) return  1;
    return 0;
}

void sortTitles(void) {
    if (titleCount <= 0) return;
    if      (currentSortMode == SORT_BY_NAME)    qsort(titles, titleCount, sizeof(TitleInfo), compareTitlesByName);
    else if (currentSortMode == SORT_BY_SIZE)    qsort(titles, titleCount, sizeof(TitleInfo), compareTitlesBySize);
    else                                         qsort(titles, titleCount, sizeof(TitleInfo), compareTitlesByID);
    cursor = 0; scrollOffset = 0; needsRedraw = true;
}

void updateFilteredList(void) {
    filteredCount = 0;
    for (int i = 0; i < titleCount; i++) {
        u32 hi = (u32)(titles[i].titleID >> 32);
        bool show = false;
        if      (currentFilterMode == FILTER_ALL)     show = true;
        else if (currentFilterMode == FILTER_UPDATES && hi == 0x0004000E) show = true;
        else if (currentFilterMode == FILTER_DLC     && hi == 0x0004008C) show = true;
        if (show) filteredIndices[filteredCount++] = i;
    }
    if (cursor >= filteredCount && filteredCount > 0) cursor = filteredCount - 1;
    if (cursor < 0 && filteredCount > 0)              cursor = 0;
}

/* ---- Backup directory helpers ---- */

void getBackupDirName(u64 titleID, const char *titleName, char *outName, size_t outSize) {
    char safe[80];
    strncpy(safe, titleName, 79); safe[79] = '\0';
    for (char *p = safe; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ':' || *p == '*' || *p == '?' ||
            *p == '"' || *p == '<'  || *p == '>' || *p == '|' || *p == '\n' || *p == '\r')
            *p = '_';
    }
    if (strlen(safe) > 60) safe[60] = '\0';
    snprintf(outName, outSize, "%016llX-%s", (unsigned long long)titleID, safe);
}

bool findBackupDir(u64 titleID, char *outDir, size_t outSize) {
    char prefix[20];
    snprintf(prefix, sizeof(prefix), "%016llX", (unsigned long long)titleID);
    DIR *d = opendir(config.backupPath);
    if (!d) return false;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strncmp(entry->d_name, prefix, 16) == 0) {
            snprintf(outDir, outSize, "%s/%s", config.backupPath, entry->d_name);
            closedir(d);
            return true;
        }
    }
    closedir(d);
    return false;
}

bool getBackupLastDate(u64 titleID, char *outDate, size_t outSize) {
    char backupDir[512];
    if (!findBackupDir(titleID, backupDir, sizeof(backupDir))) return false;
    char infoPath[512];
    snprintf(infoPath, sizeof(infoPath), "%s/backup_info.txt", backupDir);
    FILE *f = fopen(infoPath, "r");
    if (!f) { snprintf(outDate, outSize, "??/??/?? ??:??"); return true; }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Backup Date:", 12) == 0) {
            char *d = line + 12;
            while (*d == ' ') d++;
            d[strcspn(d, "\r\n")] = 0;
            snprintf(outDate, outSize, "%s", d);
            fclose(f);
            return true;
        }
    }
    fclose(f);
    snprintf(outDate, outSize, "unknown");
    return true;
}

bool checkBackupExists(u64 titleID) {
    char backupDir[512];
    return findBackupDir(titleID, backupDir, sizeof(backupDir));
}

bool getSDFreeSpace(u64 *freeBytes, u64 *totalBytes) {
    struct statvfs st;
    if (statvfs("sdmc:/", &st) != 0) { *freeBytes = 0; *totalBytes = 0; return false; }
    *freeBytes  = (u64)st.f_bavail * st.f_bsize;
    *totalBytes = (u64)st.f_blocks * st.f_bsize;
    return true;
}

/* ---- Per-title metadata (used for detail views / post-load refresh) ---- */

void getTitleName(u64 titleID, FS_MediaType mediaType, char *outName, size_t outSize) {
    SMDH smdh;
    AM_TitleEntry te;
    if (R_FAILED(AM_GetTitleInfo(mediaType, 1, &titleID, &te))) {
        snprintf(outName, outSize, "Unknown [%016llX]", (unsigned long long)titleID);
        return;
    }
    Handle fh;
    u32 ap[] = {(u32)(titleID & 0xFFFFFFFF), (u32)((titleID >> 32) & 0xFFFFFFFF), mediaType, 0};
    static const u32 fp[] = {0, 0, 2, 0x6E6F6369, 0};
    FS_Path ap2 = {PATH_BINARY, 16, ap}, fp2 = {PATH_BINARY, 20, fp};
    if (R_SUCCEEDED(FSUSER_OpenFileDirectly(&fh, ARCHIVE_SAVEDATA_AND_CONTENT, ap2, fp2, FS_OPEN_READ, 0))) {
        u32 br = 0;
        FSFILE_Read(fh, &br, 0, &smdh, sizeof(SMDH));
        FSFILE_Close(fh);
        if (br >= sizeof(u32) && smdh.magic == 0x48444D53) {
            u8 lang = CFG_LANGUAGE_EN;
            CFGU_GetSystemLanguage(&lang);
            int order[3] = {(lang < 12) ? (int)lang : 1, 1, 0};
            bool found = false;
            for (int a = 0; a < 3 && !found; a++) {
                if (a > 0 && order[a] == order[0]) continue;
                ssize_t u = utf16_to_utf8((uint8_t*)outName, smdh.titles[order[a]].shortDescription, outSize - 1);
                if (u > 0) {
                    outName[u] = '\0';
                    bool ok = false;
                    for (int i = 0; i < u; i++) { if (outName[i] != ' ' && outName[i] != '\0') { ok = true; break; } }
                    if (ok) found = true;
                }
            }
            if (!found) {
                for (int i = 0; i < 12; i++) {
                    ssize_t u = utf16_to_utf8((uint8_t*)outName, smdh.titles[i].shortDescription, outSize - 1);
                    if (u > 0) { outName[u] = '\0'; bool ok = false; for (int j = 0; j < u; j++) { if (outName[j] != ' ' && outName[j] != '\0') { ok = true; break; } } if (ok) break; }
                }
            }
            char *s = outName, *d = outName;
            while (*s) { if (*s != '|' && *s != '<' && *s != '>' && *s != '"' && *s != '\\' && *s != '/' && *s != ':' && *s != '*' && *s != '?') *d++ = *s; s++; } *d = '\0';
            return;
        }
    }
    u32 hi = (u32)(titleID >> 32);
    const char *t = "";
    if (hi == 0x0004000E) t = " Upd"; else if (hi == 0x0004008C) t = " DLC";
    snprintf(outName, outSize, "Title%s [%016llX]", t, (unsigned long long)titleID);
}

u64 getTitleSize(u64 titleID, FS_MediaType mediaType) {
    u64 sz = 0; AM_TitleEntry e;
    if (R_SUCCEEDED(AM_GetTitleInfo(mediaType, 1, &titleID, &e))) sz = e.size;
    return sz;
}

void loadTitleIcon(TitleInfo *title) { title->iconLoaded = false; }

void getTitleInfo(TitleInfo *title) {
    AM_TitleEntry te;
    if (R_SUCCEEDED(AM_GetTitleInfo(title->mediaType, 1, &title->titleID, &te))) {
        title->version = te.version; title->size = te.size;
    } else { title->version = 0; title->size = 0; }
    if (title->size == 0) title->size = getTitleSize(title->titleID, title->mediaType);
    SMDH smdh; Handle fh;
    u32 ap[] = {(u32)(title->titleID & 0xFFFFFFFF), (u32)((title->titleID >> 32) & 0xFFFFFFFF), title->mediaType, 0};
    static const u32 fp[] = {0, 0, 2, 0x6E6F6369, 0};
    FS_Path ap2 = {PATH_BINARY, 16, ap}, fp2 = {PATH_BINARY, 20, fp};
    if (R_SUCCEEDED(FSUSER_OpenFileDirectly(&fh, ARCHIVE_SAVEDATA_AND_CONTENT, ap2, fp2, FS_OPEN_READ, 0))) {
        u32 br = 0; FSFILE_Read(fh, &br, 0, &smdh, sizeof(SMDH)); FSFILE_Close(fh);
        if (br >= sizeof(u32) && smdh.magic == 0x48444D53) {
            ssize_t u = utf16_to_utf8((uint8_t*)title->fullName, smdh.titles[LANGUAGE_ENGLISH].longDescription, sizeof(title->fullName) - 1);
            if (u < 0) u = 0; title->fullName[u] = '\0'; sanitizeName(title->fullName);
            if (title->fullName[0] == '\0' || strcmp(title->fullName, "Unknown Title") == 0) {
                u = utf16_to_utf8((uint8_t*)title->fullName, smdh.titles[LANGUAGE_ENGLISH].shortDescription, sizeof(title->fullName) - 1);
                if (u < 0) u = 0; title->fullName[u] = '\0'; sanitizeName(title->fullName);
            }
        }
    }
    if (title->fullName[0] == '\0') strncpy(title->fullName, title->name, sizeof(title->fullName) - 1);
    title->hasBackup = checkBackupExists(title->titleID);
}

/* ---- Optimized title loader ---- */

void loadTitles(void) {
    titleCount = 0;
    u32 cntSD = 0, cntNAND = 0;
    AM_GetTitleCount(MEDIATYPE_SD,   &cntSD);
    AM_GetTitleCount(MEDIATYPE_NAND, &cntNAND);
    u32 totalRaw = cntSD + cntNAND;
    if (totalRaw == 0) { drawLoadingScreen(0, 0, "No titles found"); return; }

    drawLoadingScreen(0, (int)totalRaw, "Allocating memory...");
    u64 *ids = (u64*)malloc(totalRaw * sizeof(u64));
    if (!ids) return;

    /* ------------------------------------------------------------------
     * Pass 1: collect all filtered IDs (skip system ranges) into two
     *         per-media arrays for a single batch AM_GetTitleInfo call each.
     * ------------------------------------------------------------------ */
    u64          sdIDs[MAX_TITLES],   nandIDs[MAX_TITLES];
    int          sdCount = 0,         nandCount = 0;
    u32 rc = 0;

    if (cntSD > 0) {
        drawLoadingScreen(0, (int)totalRaw, "Loading SD title list...");
        if (R_SUCCEEDED(AM_GetTitleList(&rc, MEDIATYPE_SD, cntSD, ids))) {
            for (u32 i = 0; i < rc && (sdCount + nandCount) < MAX_TITLES; i++) {
                u32 hi = (u32)(ids[i] >> 32);
                if (hi == 0x00040010 || hi == 0x00040030) continue;
                sdIDs[sdCount++] = ids[i];
            }
        }
    }
    if (cntNAND > 0) {
        drawLoadingScreen((int)cntSD, (int)totalRaw, "Loading NAND title list...");
        if (R_SUCCEEDED(AM_GetTitleList(&rc, MEDIATYPE_NAND, cntNAND, ids))) {
            for (u32 i = 0; i < rc && (sdCount + nandCount) < MAX_TITLES; i++) {
                u32 hi = (u32)(ids[i] >> 32);
                if (hi == 0x00040010 || hi == 0x00040030 || hi == 0x00040138) continue;
                if (hi != 0x00040000 && hi != 0x0004000E && hi != 0x0004008C) continue;
                nandIDs[nandCount++] = ids[i];
            }
        }
    }
    free(ids);

    int total = sdCount + nandCount;
    if (total == 0) { drawLoadingScreen(0, 0, "No user titles found"); return; }

    /* ------------------------------------------------------------------
     * Optimization A: batch AM_GetTitleInfo — one call per media type.
     * ------------------------------------------------------------------ */
    drawLoadingScreen(0, total, "Fetching title metadata...");
    AM_TitleEntry *sdEntries   = (sdCount   > 0) ? (AM_TitleEntry*)malloc(sdCount   * sizeof(AM_TitleEntry)) : NULL;
    AM_TitleEntry *nandEntries = (nandCount > 0) ? (AM_TitleEntry*)malloc(nandCount * sizeof(AM_TitleEntry)) : NULL;

    if (sdCount   > 0 && sdEntries)   AM_GetTitleInfo(MEDIATYPE_SD,   (u32)sdCount,   sdIDs,   sdEntries);
    if (nandCount > 0 && nandEntries) AM_GetTitleInfo(MEDIATYPE_NAND, (u32)nandCount, nandIDs, nandEntries);

    /* ------------------------------------------------------------------
     * Optimization B: cache backup directory listing once (avoids N opendir calls).
     * ------------------------------------------------------------------ */
    drawLoadingScreen(0, total, "Scanning backup directory...");
    char backupPrefixes[512][18]; /* 16 hex chars + null */
    int  bpCount = 0;
    {
        DIR *bd = opendir(config.backupPath);
        if (bd) {
            struct dirent *bde;
            while ((bde = readdir(bd)) != NULL && bpCount < 512) {
                if (strlen(bde->d_name) >= 16)
                    snprintf(backupPrefixes[bpCount++], 18, "%.16s", bde->d_name);
            }
            closedir(bd);
        }
    }

    /* ------------------------------------------------------------------
     * Optimization C: single SMDH open per title (merged name + fullName).
     * ------------------------------------------------------------------ */
    u8 sysLang = CFG_LANGUAGE_EN;
    CFGU_GetSystemLanguage(&sysLang);
    static const u32 FP_ICON[] = {0, 0, 2, 0x6E6F6369, 0};

    /* Process SD titles */
    for (int i = 0; i < sdCount && titleCount < MAX_TITLES; i++) {
        if (i % 10 == 0) {
            char s[64]; snprintf(s, sizeof(s), "SD %d/%d", i + 1, sdCount);
            drawLoadingScreen(i, total, s);
        }
        u64 tid = sdIDs[i];
        TitleInfo *t = &titles[titleCount];
        t->titleID    = tid;
        t->mediaType  = MEDIATYPE_SD;
        t->selected   = false;
        t->isValid    = true;
        t->iconLoaded = false;
        t->fullName[0] = '\0';
        t->size    = sdEntries ? sdEntries[i].size    : 0;
        t->version = sdEntries ? sdEntries[i].version : 0;

        /* Backup status from cache */
        char tidPfx[17]; snprintf(tidPfx, 17, "%016llX", (unsigned long long)tid);
        t->hasBackup = false;
        for (int j = 0; j < bpCount; j++) {
            if (strncmp(backupPrefixes[j], tidPfx, 16) == 0) { t->hasBackup = true; break; }
        }

        /* Single SMDH open */
        u32 ap[] = {(u32)(tid & 0xFFFFFFFF), (u32)((tid >> 32) & 0xFFFFFFFF), MEDIATYPE_SD, 0};
        FS_Path ap2 = {PATH_BINARY, 16, ap};
        FS_Path fp2 = {PATH_BINARY, 20, FP_ICON};
        Handle fh;
        bool nameSet = false;
        if (R_SUCCEEDED(FSUSER_OpenFileDirectly(&fh, ARCHIVE_SAVEDATA_AND_CONTENT, ap2, fp2, FS_OPEN_READ, 0))) {
            SMDH smdh; u32 br = 0;
            FSFILE_Read(fh, &br, 0, &smdh, sizeof(SMDH));
            FSFILE_Close(fh);
            if (br >= sizeof(u32) && smdh.magic == 0x48444D53) {
                int order[3] = {(sysLang < 12) ? (int)sysLang : 1, 1, 0};
                for (int a = 0; a < 3 && !nameSet; a++) {
                    if (a > 0 && order[a] == order[0]) continue;
                    ssize_t u = utf16_to_utf8((uint8_t*)t->name, smdh.titles[order[a]].shortDescription, sizeof(t->name) - 1);
                    if (u > 0) { t->name[u] = '\0'; bool ok = false; for (int k = 0; k < u; k++) { if (t->name[k] != ' ' && t->name[k] != '\0') { ok = true; break; } } if (ok) nameSet = true; }
                }
                if (!nameSet) {
                    for (int li = 0; li < 12; li++) {
                        ssize_t u = utf16_to_utf8((uint8_t*)t->name, smdh.titles[li].shortDescription, sizeof(t->name) - 1);
                        if (u > 0) { t->name[u] = '\0'; bool ok = false; for (int k = 0; k < u; k++) { if (t->name[k] != ' ' && t->name[k] != '\0') { ok = true; break; } } if (ok) { nameSet = true; break; } }
                    }
                }
                ssize_t u2 = utf16_to_utf8((uint8_t*)t->fullName, smdh.titles[LANGUAGE_ENGLISH].longDescription, sizeof(t->fullName) - 1);
                if (u2 < 0) u2 = 0; t->fullName[u2] = '\0'; sanitizeName(t->fullName);
                if (t->fullName[0] == '\0' || strcmp(t->fullName, "Unknown Title") == 0) {
                    u2 = utf16_to_utf8((uint8_t*)t->fullName, smdh.titles[LANGUAGE_ENGLISH].shortDescription, sizeof(t->fullName) - 1);
                    if (u2 < 0) u2 = 0; t->fullName[u2] = '\0'; sanitizeName(t->fullName);
                }
            }
        }
        if (!nameSet) {
            u32 hi = (u32)(tid >> 32);
            const char *sfx = (hi == 0x0004000E) ? " Upd" : (hi == 0x0004008C) ? " DLC" : "";
            snprintf(t->name, sizeof(t->name), "Title%s [%016llX]", sfx, (unsigned long long)tid);
        }
        /* Strip filesystem-unsafe chars */
        { char *s = t->name, *d = t->name; while (*s) { if (*s != '|' && *s != '<' && *s != '>' && *s != '"' && *s != '\\' && *s != '/' && *s != ':' && *s != '*' && *s != '?') *d++ = *s; s++; } *d = '\0'; }
        if (t->fullName[0] == '\0') strncpy(t->fullName, t->name, sizeof(t->fullName) - 1);
        titleCount++;
    }

    /* Process NAND titles (same pattern) */
    for (int i = 0; i < nandCount && titleCount < MAX_TITLES; i++) {
        if (i % 10 == 0) {
            char s[64]; snprintf(s, sizeof(s), "NAND %d/%d", i + 1, nandCount);
            drawLoadingScreen(sdCount + i, total, s);
        }
        u64 tid = nandIDs[i];
        TitleInfo *t = &titles[titleCount];
        t->titleID    = tid;
        t->mediaType  = MEDIATYPE_NAND;
        t->selected   = false;
        t->isValid    = true;
        t->iconLoaded = false;
        t->fullName[0] = '\0';
        t->size    = nandEntries ? nandEntries[i].size    : 0;
        t->version = nandEntries ? nandEntries[i].version : 0;

        char tidPfx[17]; snprintf(tidPfx, 17, "%016llX", (unsigned long long)tid);
        t->hasBackup = false;
        for (int j = 0; j < bpCount; j++) {
            if (strncmp(backupPrefixes[j], tidPfx, 16) == 0) { t->hasBackup = true; break; }
        }

        u32 ap[] = {(u32)(tid & 0xFFFFFFFF), (u32)((tid >> 32) & 0xFFFFFFFF), MEDIATYPE_NAND, 0};
        FS_Path ap2 = {PATH_BINARY, 16, ap};
        FS_Path fp2 = {PATH_BINARY, 20, FP_ICON};
        Handle fh;
        bool nameSet = false;
        if (R_SUCCEEDED(FSUSER_OpenFileDirectly(&fh, ARCHIVE_SAVEDATA_AND_CONTENT, ap2, fp2, FS_OPEN_READ, 0))) {
            SMDH smdh; u32 br = 0;
            FSFILE_Read(fh, &br, 0, &smdh, sizeof(SMDH));
            FSFILE_Close(fh);
            if (br >= sizeof(u32) && smdh.magic == 0x48444D53) {
                int order[3] = {(sysLang < 12) ? (int)sysLang : 1, 1, 0};
                for (int a = 0; a < 3 && !nameSet; a++) {
                    if (a > 0 && order[a] == order[0]) continue;
                    ssize_t u = utf16_to_utf8((uint8_t*)t->name, smdh.titles[order[a]].shortDescription, sizeof(t->name) - 1);
                    if (u > 0) { t->name[u] = '\0'; bool ok = false; for (int k = 0; k < u; k++) { if (t->name[k] != ' ' && t->name[k] != '\0') { ok = true; break; } } if (ok) nameSet = true; }
                }
                if (!nameSet) {
                    for (int li = 0; li < 12; li++) {
                        ssize_t u = utf16_to_utf8((uint8_t*)t->name, smdh.titles[li].shortDescription, sizeof(t->name) - 1);
                        if (u > 0) { t->name[u] = '\0'; bool ok = false; for (int k = 0; k < u; k++) { if (t->name[k] != ' ' && t->name[k] != '\0') { ok = true; break; } } if (ok) { nameSet = true; break; } }
                    }
                }
                ssize_t u2 = utf16_to_utf8((uint8_t*)t->fullName, smdh.titles[LANGUAGE_ENGLISH].longDescription, sizeof(t->fullName) - 1);
                if (u2 < 0) u2 = 0; t->fullName[u2] = '\0'; sanitizeName(t->fullName);
                if (t->fullName[0] == '\0' || strcmp(t->fullName, "Unknown Title") == 0) {
                    u2 = utf16_to_utf8((uint8_t*)t->fullName, smdh.titles[LANGUAGE_ENGLISH].shortDescription, sizeof(t->fullName) - 1);
                    if (u2 < 0) u2 = 0; t->fullName[u2] = '\0'; sanitizeName(t->fullName);
                }
            }
        }
        if (!nameSet) {
            u32 hi = (u32)(tid >> 32);
            const char *sfx = (hi == 0x0004000E) ? " Upd" : (hi == 0x0004008C) ? " DLC" : "";
            snprintf(t->name, sizeof(t->name), "Title%s [%016llX]", sfx, (unsigned long long)tid);
        }
        { char *s = t->name, *d = t->name; while (*s) { if (*s != '|' && *s != '<' && *s != '>' && *s != '"' && *s != '\\' && *s != '/' && *s != ':' && *s != '*' && *s != '?') *d++ = *s; s++; } *d = '\0'; }
        if (t->fullName[0] == '\0') strncpy(t->fullName, t->name, sizeof(t->fullName) - 1);
        titleCount++;
    }

    free(sdEntries);
    free(nandEntries);
    drawLoadingScreen(total, total, "Sorting...");
    sortTitles();
    needsRedraw = true;
}

void findRelatedTitles(u64 baseTitleID, int selfIdx, int *outIdx, int *outCount) {
    u32 uid = (u32)(baseTitleID & 0xFFFFFFFF);
    *outCount = 0;
    for (int i = 0; i < titleCount; i++) {
        if (i == selfIdx) continue;
        if ((u32)(titles[i].titleID & 0xFFFFFFFF) == uid &&
            titles[i].titleID != baseTitleID && titles[i].isValid) {
            outIdx[(*outCount)++] = i;
            if (*outCount >= 20) break;
        }
    }
}

