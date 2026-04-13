/* ============================================================
   input.c — input handlers and blocking flow controllers
   ============================================================ */
#include "input.h"
#include "globals.h"
#include "draw.h"
#include "titles.h"
#include "backup_restore.h"
#include "install.h"
#include "config.h"

/* ---- SysInfo sublist independent sort ---- */
static int sysInfoCmpName(const void *a, const void *b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    return strcasecmp(titles[ia].name, titles[ib].name);
}
static int sysInfoCmpSize(const void *a, const void *b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    if (titles[ia].size > titles[ib].size) return -1;
    if (titles[ia].size < titles[ib].size) return  1;
    return 0;
}
static int sysInfoCmpID(const void *a, const void *b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    if (titles[ia].titleID < titles[ib].titleID) return -1;
    if (titles[ia].titleID > titles[ib].titleID) return  1;
    return 0;
}
static void sysInfoSortSubList(void) {
    if (sysInfoSubCount <= 0) return;
    if      (sysInfoSortMode == SORT_BY_NAME)    qsort(sysInfoSubIndices, sysInfoSubCount, sizeof(int), sysInfoCmpName);
    else if (sysInfoSortMode == SORT_BY_SIZE)    qsort(sysInfoSubIndices, sysInfoSubCount, sizeof(int), sysInfoCmpSize);
    else                                         qsort(sysInfoSubIndices, sysInfoSubCount, sizeof(int), sysInfoCmpID);
}

/* ---- Backup list independent sort ---- */
static int backupCmpName(const void *a, const void *b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    return strcasecmp(titles[ia].name, titles[ib].name);
}
static int backupCmpSize(const void *a, const void *b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    if (titles[ia].size > titles[ib].size) return -1;
    if (titles[ia].size < titles[ib].size) return  1;
    return 0;
}
static int backupCmpID(const void *a, const void *b) {
    int ia = *(const int*)a, ib = *(const int*)b;
    if (titles[ia].titleID < titles[ib].titleID) return -1;
    if (titles[ia].titleID > titles[ib].titleID) return  1;
    return 0;
}
static void sortBackupList(void) {
    if (backupTitleCount <= 0) return;
    if      (backupSortMode == SORT_BY_NAME) qsort(backupIndices, backupTitleCount, sizeof(int), backupCmpName);
    else if (backupSortMode == SORT_BY_SIZE) qsort(backupIndices, backupTitleCount, sizeof(int), backupCmpSize);
    else                                     qsort(backupIndices, backupTitleCount, sizeof(int), backupCmpID);
}

/* ---- buildBackupList ---- */
void buildBackupList(void) {
    backupTitleCount = 0;
    for (int i = 0; i < titleCount; i++) {
        if (!titles[i].isValid) continue;
        u32 hi = (u32)(titles[i].titleID >> 32);
        if (hi == 0x0004000E || hi == 0x0004008C) continue;
        if (backupTitleCount < MAX_TITLES) backupIndices[backupTitleCount++] = i;
    }
    sortBackupList();
}

/* ============================================================
   Per-frame input handlers
   ============================================================ */

void handleMainMenuInput(void) {
    u32 keys = hidKeysDown();
    if (keys & KEY_DOWN) menuCursor = (menuCursor + 1) % 5;
    if (keys & KEY_UP)   menuCursor = (menuCursor + 4) % 5;
    if (keys & KEY_A) {
        switch (menuCursor) {
            case 0: appState = APP_INSTALL; break;
            case 1: appState = APP_BACKUP;  break;
            case 2:
                cursor = 0; scrollOffset = 0;
                if (titleCount > 0) updateFilteredList();
                appState = APP_UNINSTALL;
                break;
            case 3:
                sysInfoMode = SYSINFO_OVERVIEW; sysInfoCursor = 0;
                appState = APP_SYSINFO;
                break;
            case 4:
                settingsCursor = 0; appState = APP_SETTINGS;
                break;
        }
    }
}

void handleUninstallInput(void) {
    u32 keys = hidKeysDown();
    if (keys & KEY_DOWN) {
        if (cursor < filteredCount - 1) {
            cursor++;
            if (cursor >= scrollOffset + UNINSTALL_VISIBLE) scrollOffset = cursor - UNINSTALL_VISIBLE + 1;
        }
    }
    if (keys & KEY_UP) {
        if (cursor > 0) {
            cursor--;
            if (cursor < scrollOffset) scrollOffset = cursor;
        }
    }
    if (keys & KEY_RIGHT) {
        cursor += UNINSTALL_VISIBLE;
        if (cursor >= filteredCount) cursor = (filteredCount > 0) ? filteredCount - 1 : 0;
        if (cursor >= scrollOffset + UNINSTALL_VISIBLE) scrollOffset = cursor - UNINSTALL_VISIBLE + 1;
    }
    if (keys & KEY_LEFT) {
        cursor -= UNINSTALL_VISIBLE;
        if (cursor < 0) cursor = 0;
        if (cursor < scrollOffset) scrollOffset = cursor;
    }
    if (keys & KEY_L) { currentSortMode = (SortMode)((currentSortMode + 2) % 3); sortTitles(); updateFilteredList(); }
    if (keys & KEY_R) { currentSortMode = (SortMode)((currentSortMode + 1) % 3); sortTitles(); updateFilteredList(); }
    if (keys & KEY_Y) { currentFilterMode = (FilterMode)((currentFilterMode + 1) % 3); updateFilteredList(); }
    if ((keys & KEY_A) && filteredCount > 0) titles[filteredIndices[cursor]].selected ^= true;
    if (keys & KEY_B) { for (int i = 0; i < titleCount; i++) titles[i].selected = false; appState = APP_MAIN_MENU; }
}

void handleSysInfoInput(void) {
    u32 keys = hidKeysDown();
    if (sysInfoMode == SYSINFO_OVERVIEW) {
        if (keys & KEY_DOWN) sysInfoCursor = (sysInfoCursor + 1) % 3;
        if (keys & KEY_UP)   sysInfoCursor = (sysInfoCursor + 2) % 3;
        if (keys & KEY_B)    appState = APP_MAIN_MENU;
        if (keys & KEY_A) {
            u32 tHi = (sysInfoCursor == 0) ? 0x00040000u :
                      (sysInfoCursor == 1) ? 0x0004000Eu : 0x0004008Cu;
            sysInfoSubCount = 0;
            for (int i = 0; i < titleCount; i++) {
                if (titles[i].isValid && (u32)(titles[i].titleID >> 32) == tHi) {
                    sysInfoSubIndices[sysInfoSubCount++] = i;
                    if (sysInfoSubCount >= MAX_TITLES) break;
                }
            }
            sysInfoSubCursor = 0; sysInfoSubScrollOffset = 0;
            sysInfoSortSubList();
            sysInfoMode = (sysInfoCursor == 0) ? SYSINFO_GAMES :
                          (sysInfoCursor == 1) ? SYSINFO_UPDATES : SYSINFO_DLC;
        }
    } else {
        if (keys & KEY_DOWN) {
            if (sysInfoSubCursor < sysInfoSubCount - 1) {
                sysInfoSubCursor++;
                if (sysInfoSubCursor >= sysInfoSubScrollOffset + MAX_VISIBLE_TITLES)
                    sysInfoSubScrollOffset = sysInfoSubCursor - MAX_VISIBLE_TITLES + 1;
            }
        }
        if (keys & KEY_UP) {
            if (sysInfoSubCursor > 0) {
                sysInfoSubCursor--;
                if (sysInfoSubCursor < sysInfoSubScrollOffset) sysInfoSubScrollOffset = sysInfoSubCursor;
            }
        }
        if (keys & KEY_L) { sysInfoSortMode = (SortMode)((sysInfoSortMode + 2) % 3); sysInfoSortSubList(); sysInfoSubCursor = 0; sysInfoSubScrollOffset = 0; }
        if (keys & KEY_R) { sysInfoSortMode = (SortMode)((sysInfoSortMode + 1) % 3); sysInfoSortSubList(); sysInfoSubCursor = 0; sysInfoSubScrollOffset = 0; }
        if (keys & KEY_RIGHT) {
            sysInfoSubCursor += MAX_VISIBLE_TITLES;
            if (sysInfoSubCursor >= sysInfoSubCount) sysInfoSubCursor = (sysInfoSubCount > 0) ? sysInfoSubCount - 1 : 0;
            if (sysInfoSubCursor >= sysInfoSubScrollOffset + MAX_VISIBLE_TITLES)
                sysInfoSubScrollOffset = sysInfoSubCursor - MAX_VISIBLE_TITLES + 1;
        }
        if (keys & KEY_LEFT) {
            sysInfoSubCursor -= MAX_VISIBLE_TITLES;
            if (sysInfoSubCursor < 0) sysInfoSubCursor = 0;
            if (sysInfoSubCursor < sysInfoSubScrollOffset) sysInfoSubScrollOffset = sysInfoSubCursor;
        }
        if (keys & KEY_B) {
            sysInfoCursor = (sysInfoMode == SYSINFO_GAMES) ? 0 : (sysInfoMode == SYSINFO_UPDATES) ? 1 : 2;
            sysInfoMode = SYSINFO_OVERVIEW;
        }
    }
}

void handleSettingsInput(void) {
    u32 keys = hidKeysDown();
    if (keys & KEY_DOWN) settingsCursor = (settingsCursor + 1) % 5;
    if (keys & KEY_UP)   settingsCursor = (settingsCursor + 4) % 5;
    bool *boolPtrs[4] = {
        &config.forceBackup, &config.skipUninstallConfirm,
        &config.forceRestore, &config.skipInstallConfirm
    };
    int pidx = 0;
    for (int i = 0; i < (int)NUM_BACKUP_PATHS; i++)
        if (strcmp(config.backupPath, BACKUP_PATH_OPTIONS[i]) == 0) { pidx = i; break; }
    bool changed = false;
    if ((keys & KEY_A) || (keys & KEY_RIGHT) || (keys & KEY_R)) {
        if      (settingsCursor <= 3) { *boolPtrs[settingsCursor] ^= true; changed = true; }
        else if (settingsCursor == 4) { pidx = (pidx + 1) % (int)NUM_BACKUP_PATHS; snprintf(config.backupPath, sizeof(config.backupPath), "%s", BACKUP_PATH_OPTIONS[pidx]); changed = true; }
    }
    if ((keys & KEY_LEFT) || (keys & KEY_L)) {
        if      (settingsCursor <= 3) { *boolPtrs[settingsCursor] ^= true; changed = true; }
        else if (settingsCursor == 4) { pidx = (pidx + (int)NUM_BACKUP_PATHS - 1) % (int)NUM_BACKUP_PATHS; snprintf(config.backupPath, sizeof(config.backupPath), "%s", BACKUP_PATH_OPTIONS[pidx]); changed = true; }
    }
    if (changed) saveConfig();
    if ((keys & KEY_B) || (keys & KEY_START)) { saveConfig(); appState = APP_MAIN_MENU; }
}

/* ============================================================
   Blocking flows
   ============================================================ */

/* ---- Internal: navigate up one folder level ---- */
static void _goUpDir(void) {
    if (strcmp(currentPath, "sdmc:/") == 0) return;
    char parent[512]; strncpy(parent, currentPath, sizeof(parent) - 1); parent[sizeof(parent)-1] = '\0';
    char *lastSlash = strrchr(parent, '/');
    if (lastSlash) {
        *lastSlash = '\0';
        if (strlen(parent) <= 5) strncpy(parent, "sdmc:/", sizeof(parent) - 1);
    } else {
        strncpy(parent, "sdmc:/", sizeof(parent) - 1);
    }
    scanDirectory(parent);
    if (dirStackDepth > 0) {
        dirStackDepth--;
        fileCursor       = dirCursorStack[dirStackDepth];
        fileScrollOffset = dirScrollStack[dirStackDepth];
    } else {
        fileCursor = 0; fileScrollOffset = 0;
    }
}

/* ---- Internal: install all .cia files in a folder ---- */
static int _installFolderCIAs(const char *folderPath) {
    char ciaPaths[64][512]; int ciaCount = 0;
    DIR *d = opendir(folderPath);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL && ciaCount < 64) {
            if (ent->d_type == DT_DIR) continue;
            const char *ext = strrchr(ent->d_name, '.');
            if (!ext || strcasecmp(ext, ".cia") != 0) continue;
            snprintf(ciaPaths[ciaCount++], 512, "%s/%s", folderPath, ent->d_name);
        }
        closedir(d);
    }
    if (ciaCount == 0) return 0;
    FS_MediaType dest = MEDIATYPE_SD;
    int installed = 0, failed = 0;
    for (int i = 0; i < ciaCount; i++) {
        bool ok = installCIA(ciaPaths[i], dest);
        if (ok) {
            installed++;
            u64 tid = getCIATitleID(ciaPaths[i]);
            if (tid != 0) {
                char backupDir[512];
                if (findBackupDir(tid, backupDir, sizeof(backupDir))) {
                    TitleInfo tmp; memset(&tmp, 0, sizeof(tmp));
                    tmp.titleID = tid; tmp.mediaType = dest;
                    getTitleName(tid, dest, tmp.name, sizeof(tmp.name));
                    if (config.forceRestore) {
                        restoreSaveData(&tmp);
                        const char *rDl[] = { "", "  Save data restored", "  automatically.", "", "  A=OK" };
                        drawDialog(rDl, 5);
                        while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                    } else {
                        const char *fname = strrchr(ciaPaths[i], '/'); fname = fname ? fname + 1 : ciaPaths[i];
                        char fn2[42]; strncpy(fn2, fname, 40); fn2[40] = '\0';
                        char fnMsg[48]; snprintf(fnMsg, sizeof(fnMsg), "  %.40s", fn2);
                        const char *askDl[] = { fnMsg, "  Found backup for this title.", "  Restore save data?", "", "  A=Yes   B=No" };
                        bool doRestore = false;
                        while (aptMainLoop()) { drawDialog(askDl, 5); hidScanInput(); u32 rk = hidKeysDown(); if (rk & KEY_A) { doRestore = true; break; } if (rk & KEY_B) break; }
                        if (doRestore) {
                            restoreSaveData(&tmp);
                            const char *rDl[] = { "", "  Save data restored.", "", "  A=OK" };
                            drawDialog(rDl, 4);
                            while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                        }
                    }
                }
            }
        } else { failed++; }
    }
    char resMsg[64]; snprintf(resMsg, sizeof(resMsg), "  Installed: %d   Failed: %d", installed, failed);
    const char *resDl[] = { "", "  Batch install completed.", resMsg, "", "  A=Continue" };
    drawDialog(resDl, 5);
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
    if (installed > 0) titlesNeedRefresh = true;
    return installed;
}

void runInstallFlow(void) {
    scanDirectory("sdmc:/");
    fileCursor = 0; fileScrollOffset = 0; dirStackDepth = 0;
    while (aptMainLoop()) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawFileBrowserScreen();
        C3D_FrameEnd(0);
        hidScanInput();
        u32 k = hidKeysDown();
        if (k & KEY_START) return;
        if (k & KEY_DOWN) { if (fileCursor < fileCount - 1) { fileCursor++; if (fileCursor >= fileScrollOffset + FILE_BROWSER_VISIBLE) fileScrollOffset = fileCursor - FILE_BROWSER_VISIBLE + 1; } }
        if (k & KEY_UP)   { if (fileCursor > 0) { fileCursor--; if (fileCursor < fileScrollOffset) fileScrollOffset = fileCursor; } }
        if (k & KEY_RIGHT) { fileCursor += FILE_BROWSER_VISIBLE; if (fileCursor >= fileCount) fileCursor = fileCount - 1; if (fileCursor < 0) fileCursor = 0; if (fileCursor >= fileScrollOffset + FILE_BROWSER_VISIBLE) fileScrollOffset = fileCursor - FILE_BROWSER_VISIBLE + 1; }
        if (k & KEY_LEFT)  { fileCursor -= FILE_BROWSER_VISIBLE; if (fileCursor < 0) fileCursor = 0; if (fileCursor < fileScrollOffset) fileScrollOffset = fileCursor; }
        if (k & KEY_B) { if (strcmp(currentPath, "sdmc:/") == 0) return; _goUpDir(); continue; }
        if (k & KEY_Y) {
            int ciaInDir = 0;
            for (int i = 0; i < fileCount; i++) if (!fileEntries[i].isDir && fileEntries[i].isCIA) ciaInDir++;
            if (ciaInDir == 0) { const char *msg[] = { "", "  No CIA files in this folder.", "", "  A=Continue" }; drawDialog(msg, 4); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; } continue; }
            if (!config.skipInstallConfirm) {
                char cntMsg[52]; snprintf(cntMsg, sizeof(cntMsg), "  Install %d CIA files?", ciaInDir);
                const char *cfDl[] = { "", cntMsg, "  Destination: SD card", "", "  A=Yes   B=No" };
                bool confirmed = false;
                while (aptMainLoop()) { drawDialog(cfDl, 5); hidScanInput(); u32 ck = hidKeysDown(); if (ck & KEY_A) { confirmed = true; break; } if (ck & KEY_B) break; }
                if (!confirmed) continue;
            }
            char savedPath[512]; strncpy(savedPath, currentPath, sizeof(savedPath)-1);
            _installFolderCIAs(savedPath);
            return;
        }
        if (k & KEY_A) {
            if (fileCount == 0) continue;
            FileEntry *fe = &fileEntries[fileCursor];
            if (fe->isDir) {
                if (strcmp(fe->name, "..") == 0) { if (strcmp(currentPath, "sdmc:/") == 0) return; _goUpDir(); }
                else {
                    if (dirStackDepth < DIR_STACK_MAX) { dirCursorStack[dirStackDepth] = fileCursor; dirScrollStack[dirStackDepth] = fileScrollOffset; dirStackDepth++; }
                    char newPath[512];
                    if (strcmp(currentPath, "sdmc:/") == 0) snprintf(newPath, sizeof(newPath), "sdmc:/%s", fe->name);
                    else                                     snprintf(newPath, sizeof(newPath), "%s/%s", currentPath, fe->name);
                    scanDirectory(newPath); fileCursor = 0; fileScrollOffset = 0;
                }
            } else if (fe->isCIA) {
                char ciaPath[512];
                if (strcmp(currentPath, "sdmc:/") == 0) snprintf(ciaPath, sizeof(ciaPath), "sdmc:/%s", fe->name);
                else                                     snprintf(ciaPath, sizeof(ciaPath), "%s/%s", currentPath, fe->name);
                u64 tid = getCIATitleID(ciaPath);
                FS_MediaType dest = MEDIATYPE_SD;
                if (!config.skipInstallConfirm) {
                    char tidStr[36]; if (tid) snprintf(tidStr, sizeof(tidStr), "  ID: %016llX", (unsigned long long)tid); else snprintf(tidStr, sizeof(tidStr), "  ID: N/A");
                    char fnShort[40]; strncpy(fnShort, fe->name, 38); fnShort[38] = '\0';
                    char fileMsg[44]; snprintf(fileMsg, sizeof(fileMsg), "  %.38s", fnShort);
                    const char *cfDl[] = { "  Install?", fileMsg, tidStr, "  Destination: SD card", "", "  A=Yes   B=No" };
                    bool confirmed = false;
                    while (aptMainLoop()) { drawDialog(cfDl, 6); hidScanInput(); u32 ck = hidKeysDown(); if (ck & KEY_A) { confirmed = true; break; } if (ck & KEY_B) break; }
                    if (!confirmed) continue;
                }
                bool ok = installCIA(ciaPath, dest);
                if (ok) {
                    titlesNeedRefresh = true;
                    char fn2[42]; strncpy(fn2, fe->name, 40); fn2[40] = '\0';
                    char okMsg[48]; snprintf(okMsg, sizeof(okMsg), "  %.40s", fn2);
                    const char *okDl[] = { "  Installation complete!", okMsg, "", "  A=Continue" };
                    drawDialog(okDl, 4);
                    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                    if (tid != 0) {
                        char backupDir[512];
                        if (findBackupDir(tid, backupDir, sizeof(backupDir))) {
                            TitleInfo tmp; memset(&tmp, 0, sizeof(tmp));
                            tmp.titleID = tid; tmp.mediaType = dest;
                            getTitleName(tid, dest, tmp.name, sizeof(tmp.name));
                            if (config.forceRestore) {
                                restoreSaveData(&tmp);
                                const char *rDl[] = { "", "  Save data restored", "  automatically.", "", "  A=OK" };
                                drawDialog(rDl, 5);
                                while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                            } else {
                                const char *askDl[] = { "", "  Found backup for this title.", "  Restore save data?", "", "  A=Yes   B=No" };
                                bool doRestore = false;
                                while (aptMainLoop()) { drawDialog(askDl, 5); hidScanInput(); u32 rk = hidKeysDown(); if (rk & KEY_A) { doRestore = true; break; } if (rk & KEY_B) break; }
                                if (doRestore) { restoreSaveData(&tmp); const char *rDl[] = { "", "  Save data restored.", "", "  A=OK" }; drawDialog(rDl, 4); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; } }
                            }
                        }
                    }
                } else {
                    const char *errDl[] = { "", "  Error during installation.", "  Check that the CIA file is valid.", "", "  A=Continue" };
                    drawDialog(errDl, 5);
                    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
                }
            }
        }
    }
}

void runBackupFlow(void) {
    for (int i = 0; i < backupTitleCount; i++) titles[backupIndices[i]].selected = false;
    backupCursor = 0; backupScrollOffset = 0;
    while (aptMainLoop()) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawBackupScreen();
        C3D_FrameEnd(0);
        hidScanInput();
        u32 k = hidKeysDown();
        if ((k & KEY_B) || (k & KEY_START)) return;
        if (k & KEY_L) { backupSortMode = (SortMode)((backupSortMode + 2) % 3); sortBackupList(); backupCursor = 0; backupScrollOffset = 0; }
        if (k & KEY_R) { backupSortMode = (SortMode)((backupSortMode + 1) % 3); sortBackupList(); backupCursor = 0; backupScrollOffset = 0; }
        if (k & KEY_DOWN) { if (backupCursor < backupTitleCount - 1) { backupCursor++; if (backupCursor >= backupScrollOffset + BACKUP_VISIBLE) backupScrollOffset = backupCursor - BACKUP_VISIBLE + 1; } }
        if (k & KEY_UP)   { if (backupCursor > 0) { backupCursor--; if (backupCursor < backupScrollOffset) backupScrollOffset = backupCursor; } }
        if (k & KEY_RIGHT) { backupCursor += BACKUP_VISIBLE; if (backupCursor >= backupTitleCount) backupCursor = (backupTitleCount > 0) ? backupTitleCount - 1 : 0; if (backupCursor >= backupScrollOffset + BACKUP_VISIBLE) backupScrollOffset = backupCursor - BACKUP_VISIBLE + 1; }
        if (k & KEY_LEFT)  { backupCursor -= BACKUP_VISIBLE; if (backupCursor < 0) backupCursor = 0; if (backupCursor < backupScrollOffset) backupScrollOffset = backupCursor; }
        if (k & KEY_A) titles[backupIndices[backupCursor]].selected ^= true;
        if (k & KEY_X) {
            int selCount = 0;
            for (int i = 0; i < backupTitleCount; i++) if (titles[backupIndices[i]].selected && titles[backupIndices[i]].isValid) selCount++;
            if (selCount == 0) { const char *msg[] = { "", "  No title selected.", "  Select titles with A.", "", "  Press A to continue." }; drawDialog(msg, 5); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; } continue; }
            char cntMsg[56]; snprintf(cntMsg, sizeof(cntMsg), "  Backup %d selected titles?", selCount);
            const char *cfDl[] = { "", cntMsg, "", "  A=Yes   B=No" };
            bool confirmed = false;
            while (aptMainLoop()) { drawDialog(cfDl, 4); hidScanInput(); u32 ck = hidKeysDown(); if (ck & KEY_A) { confirmed = true; break; } if (ck & KEY_B) break; }
            if (!confirmed) continue;
            int done = 0;
            for (int i = 0; i < backupTitleCount; i++) {
                TitleInfo *ti = &titles[backupIndices[i]]; if (!ti->selected || !ti->isValid) continue;
                drawLoadingScreen(done, selCount, ti->name); backupSaveData(ti); ti->hasBackup = true; done++;
            }
            char doneMsg[56]; snprintf(doneMsg, sizeof(doneMsg), "  Backup completed for %d titles.", done);
            const char *doneDl[] = { "", doneMsg, "", "  A=Continue" };
            drawDialog(doneDl, 4); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
            return;
        }
        if (k & KEY_Y) {
            int validCount = backupTitleCount;
            char cntMsg[56]; snprintf(cntMsg, sizeof(cntMsg), "  Backup ALL titles (%d)?", validCount);
            const char *cfDl[] = { "", cntMsg, "  This may take a while.", "", "  A=Yes   B=No" };
            bool confirmed = false;
            while (aptMainLoop()) { drawDialog(cfDl, 5); hidScanInput(); u32 ck = hidKeysDown(); if (ck & KEY_A) { confirmed = true; break; } if (ck & KEY_B) break; }
            if (!confirmed) continue;
            int done = 0;
            for (int i = 0; i < backupTitleCount; i++) {
                TitleInfo *ti = &titles[backupIndices[i]]; if (!ti->isValid) continue;
                drawLoadingScreen(done, validCount, ti->name); backupSaveData(ti); ti->hasBackup = true; done++;
            }
            char doneMsg[56]; snprintf(doneMsg, sizeof(doneMsg), "  Backup completed for %d titles.", done);
            const char *doneDl[] = { "", doneMsg, "", "  A=Continue" };
            drawDialog(doneDl, 4); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
            return;
        }
    }
}

void runUninstallDeleteFlow(void) {
    int selectedCount = 0;
    for (int i = 0; i < titleCount; i++) if (titles[i].selected && titles[i].isValid) selectedCount++;
    if (selectedCount == 0) {
        const char *msg[] = { "", "  No title selected.", "", "  Select at least one title with A.", "", "  Press A to continue." };
        drawDialog(msg, 6); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
        return;
    }
    /* Find related titles */
    int pendingRelated[MAX_TITLES]; int pendingCount = 0;
    for (int i = 0; i < titleCount; i++) {
        if (!titles[i].selected || !titles[i].isValid) continue;
        if ((u32)(titles[i].titleID >> 32) != 0x00040000) continue;
        int relIdx[20]; int relCount = 0;
        findRelatedTitles(titles[i].titleID, i, relIdx, &relCount);
        for (int r = 0; r < relCount; r++) {
            int ri = relIdx[r];
            if (!titles[ri].selected && titles[ri].isValid) {
                bool dup = false;
                for (int p = 0; p < pendingCount; p++) if (pendingRelated[p] == ri) { dup = true; break; }
                if (!dup && pendingCount < MAX_TITLES) pendingRelated[pendingCount++] = ri;
            }
        }
    }
    if (pendingCount > 0) {
        char relMsg[64]; snprintf(relMsg, sizeof(relMsg), "  Found %d related titles (DLC/Update).", pendingCount);
        const char *dl[] = { "", relMsg, "", "  A=Include  B=Skip  START=Cancel" };
        bool addRelated = false, cancelAll = false;
        while (aptMainLoop()) { drawDialogWithSelectedList(dl, 4); hidScanInput(); u32 k = hidKeysDown(); if (k & KEY_A) { addRelated = true; break; } if (k & KEY_B) break; if (k & KEY_START) { cancelAll = true; break; } }
        if (cancelAll) { for (int i = 0; i < titleCount; i++) titles[i].selected = false; return; }
        if (addRelated) { for (int p = 0; p < pendingCount; p++) titles[pendingRelated[p]].selected = true; selectedCount += pendingCount; }
    }
    /* Backup? */
    bool doBackup = false;
    char chosenPath[256]; snprintf(chosenPath, sizeof(chosenPath), "%s", config.backupPath);
    if (config.forceBackup) {
        doBackup = true;
    } else {
        const char *bkDl[] = { "", "  Backup save data?", "", "  A=Yes   B=No   START=Cancel" };
        while (aptMainLoop()) { drawDialogWithSelectedList(bkDl, 4); hidScanInput(); u32 k = hidKeysDown(); if (k & KEY_A) { doBackup = true; break; } if (k & KEY_B) break; if (k & KEY_START) { for (int i = 0; i < titleCount; i++) titles[i].selected = false; return; } }
    }
    /* Choose backup path */
    if (doBackup) {
        char pathLine[80]; snprintf(pathLine, sizeof(pathLine), "  %.46s", config.backupPath);
        const char *pd[] = { "", pathLine, "", "  A=Use default   Y=Choose other" };
        bool choosePath = false;
        while (aptMainLoop()) { drawDialogWithSelectedList(pd, 4); hidScanInput(); u32 k = hidKeysDown(); if (k & KEY_A) break; if (k & KEY_Y) { choosePath = true; break; } }
        if (choosePath) {
            int pidx = 0;
            for (int i = 0; i < (int)NUM_BACKUP_PATHS; i++) if (strcmp(config.backupPath, BACKUP_PATH_OPTIONS[i]) == 0) { pidx = i; break; }
            while (aptMainLoop()) {
                char pl[64]; snprintf(pl, sizeof(pl), "  %.52s", BACKUP_PATH_OPTIONS[pidx]);
                const char *ppd[] = { "  Select backup folder:", pl, "", "  Up/Down=Change   A=Confirm   B=Cancel" };
                drawDialogWithSelectedList(ppd, 4);
                hidScanInput(); u32 k = hidKeysDown();
                if (k & KEY_DOWN) pidx = (pidx + 1) % (int)NUM_BACKUP_PATHS;
                if (k & KEY_UP)   pidx = (pidx + (int)NUM_BACKUP_PATHS - 1) % (int)NUM_BACKUP_PATHS;
                if (k & KEY_A) { snprintf(chosenPath, sizeof(chosenPath), "%s", BACKUP_PATH_OPTIONS[pidx]); break; }
                if (k & KEY_B)  break;
            }
        }
    }
    /* Confirm */
    if (!config.skipUninstallConfirm) {
        char cntMsg[64]; snprintf(cntMsg, sizeof(cntMsg), "  Delete %d titles?", selectedCount);
        char bkMsg[64]; if (doBackup) snprintf(bkMsg, sizeof(bkMsg), "  Backup to: %.36s", chosenPath); else snprintf(bkMsg, sizeof(bkMsg), "  No backup.");
        const char *cfDl[] = { "", cntMsg, bkMsg, "", "  A=Confirm   B=Cancel" };
        bool confirmed = false;
        while (aptMainLoop()) { drawDialogWithSelectedList(cfDl, 5); hidScanInput(); u32 k = hidKeysDown(); if (k & KEY_A) { confirmed = true; break; } if (k & KEY_B) { for (int i = 0; i < titleCount; i++) titles[i].selected = false; return; } }
        if (!confirmed) { for (int i = 0; i < titleCount; i++) titles[i].selected = false; return; }
    }
    /* Execute backup + delete */
    for (int i = 0; i < titleCount; i++) {
        if (!titles[i].selected || !titles[i].isValid) continue;
        if (doBackup) backupSaveDataToPath(&titles[i], chosenPath);
        deleteTitle(&titles[i]);
    }
    titleCount = 0; cursor = 0; scrollOffset = 0;
    for (int i = 0; i < MAX_TITLES; i++) titles[i].selected = false;
    appState = APP_MAIN_MENU;
}

void runSysInfoDetailFlow(void) {
    int idx = sysInfoDetailIdx;
    sysInfoDetailCursor = 0;
    while (aptMainLoop()) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        drawTitleDetails();
        C3D_FrameEnd(0);
        hidScanInput();
        u32 k = hidKeysDown();
        if (k & KEY_DOWN) sysInfoDetailCursor = (sysInfoDetailCursor + 1) % 3;
        if (k & KEY_UP)   sysInfoDetailCursor = (sysInfoDetailCursor + 2) % 3;
        if (k & KEY_B)    break;
        if (k & KEY_A) {
            if (sysInfoDetailCursor == 0) {
                backupSaveData(&titles[idx]); titles[idx].hasBackup = true;
                const char *msg[] = { "", "  Backup completed.", "", "  A=OK" };
                drawDialog(msg, 4); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
            } else if (sysInfoDetailCursor == 1) {
                if (!titles[idx].hasBackup) { const char *msg[] = { "", "  No backup available.", "", "  A=OK" }; drawDialog(msg, 4); while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; } continue; }
                bool ok = restoreSaveData(&titles[idx]);
                if (ok) { const char *msg[] = { "", "  Save data restored.", "", "  A=OK" }; drawDialog(msg, 4); }
                else    { const char *msg[] = { "", "  Restore error.", "", "  A=OK" };       drawDialog(msg, 4); }
                while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
            } else {
                titles[idx].selected = true;
                runUninstallDeleteFlow();
                if (titleCount == 0) { sysInfoMode = SYSINFO_OVERVIEW; break; }
                titles[idx].selected = false;
            }
        }
    }
}

