/* ============================================================
   draw.c — all drawing / rendering functions
   ============================================================ */
#include "draw.h"
#include "globals.h"
#include "utils.h"
#include "titles.h"
#include "lang.h"

/* ---- Internal helper: draw text through dynamicBuf ---- */
static void dt(float x, float y, float z, float s, u32 c, const char *str) {
    C2D_Text t;
    C2D_TextParse(&t, dynamicBuf, str);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, z, s, s, c);
}

/* ---- Internal: render dialog box on current top scene ---- */
static void _renderDialogBox(const char **lines, int lineCount) {
    float bh = (float)lineCount * 20.0f + 30.0f;
    if (bh < 60.0f) bh = 60.0f;
    float bw = 340.0f;
    float bx = (400.0f - bw) / 2.0f;
    float by = (240.0f - bh) / 2.0f;
    C2D_DrawRectSolid(bx,       by,        0.5f, bw, bh, C2D_Color32(10,10,20,245));
    C2D_DrawRectSolid(bx,       by,        0.5f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,       by+bh-2,   0.5f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,       by,        0.5f, 2,  bh, CLR_WHITE);
    C2D_DrawRectSolid(bx+bw-2,  by,        0.5f, 2,  bh, CLR_WHITE);
    for (int i = 0; i < lineCount; i++) {
        if (lines[i] && lines[i][0])
            dt(bx + 12.0f, by + 12.0f + (float)i * 20.0f, 0.5f, 0.54f, CLR_WHITE, lines[i]);
    }
}

/* ---- Internal: render selected-titles list on current bottom scene ---- */
static void _renderSelectedList(void) {
    int selCount = 0;
    for (int i = 0; i < titleCount; i++)
        if (titles[i].selected && titles[i].isValid) selCount++;
    char hdr[64]; snprintf(hdr, sizeof(hdr), T(STR_SELECTED_FMT), selCount);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_RED);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, hdr);
    int shown = 0;
    for (int i = 0; i < titleCount && shown < 10; i++) {
        if (!titles[i].selected || !titles[i].isValid) continue;
        u32 hi = (u32)(titles[i].titleID >> 32);
        const char *sym = (hi == 0x0004000E) ? "^" : (hi == 0x0004008C) ? "+" : " ";
        char shortName[30]; strncpy(shortName, titles[i].name, 28); shortName[28] = '\0';
        char line[48]; snprintf(line, sizeof(line), "%s %s", sym, shortName);
        dt(4, 22.0f + (float)shown * 20.0f, 0.5f, 0.52f, CLR_WHITE, line);
        shown++;
    }
    if (selCount > 10) {
        char more[32]; snprintf(more, sizeof(more), T(STR_AND_N_MORE_FMT), selCount - 10);
        dt(4, 22.0f + 10.0f * 20.0f, 0.5f, 0.52f, CLR_GRAY, more);
    }
}

/* ============================================================
   Loading / Progress screens
   ============================================================ */

void drawLoadingScreen(int current, int total, const char *status) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, T(STR_LOADING));
    dt(4, 28, 0.5f, 0.52f, CLR_WHITE, T(STR_LOADING_TITLES));
    if (status) dt(4, 48, 0.5f, 0.48f, CLR_GRAY, status);
    if (total > 0) {
        float p = (float)current / (float)total;
        float bw = 300.0f, bh = 20.0f, bx = 50.0f, by = 90.0f;
        C2D_DrawRectSolid(bx,      by,      0.5f, bw,   bh, C2D_Color32(50,50,50,255));
        C2D_DrawRectSolid(bx,      by,      0.5f, bw*p, bh, CLR_CYAN);
        C2D_DrawRectSolid(bx,      by,      0.5f, bw,   2,  CLR_WHITE);
        C2D_DrawRectSolid(bx,      by+bh-2, 0.5f, bw,   2,  CLR_WHITE);
        C2D_DrawRectSolid(bx,      by,      0.5f, 2,    bh, CLR_WHITE);
        C2D_DrawRectSolid(bx+bw-2, by,      0.5f, 2,    bh, CLR_WHITE);
        char pt[64]; snprintf(pt, sizeof(pt), "%d / %d", current, total);
        dt(bx+bw/2.0f-20.0f, by+25.0f, 0.5f, 0.48f, CLR_WHITE, pt);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C3D_FrameEnd(0);
}

/* Called from install.c inside its own C3D_FrameBegin/End — NO frame management here */
void drawInstallProgressScreen(const char *ciaPath, long done, long total) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, T(STR_INSTALL_HEADER));
    int nameStart = (int)(strlen(ciaPath) - 1);
    while (nameStart > 0 && ciaPath[nameStart-1] != '/' && ciaPath[nameStart-1] != ':') nameStart--;
    char nb[52]; snprintf(nb, sizeof(nb), "%.50s", &ciaPath[nameStart]);
    dt(4, 28, 0.5f, 0.48f, CLR_GRAY, nb);
    float p = (total > 0) ? (float)done / (float)total : 0.0f;
    float bw = 300.0f, bh = 20.0f, bx = 50.0f, by = 70.0f;
    C2D_DrawRectSolid(bx,      by,      0.5f, bw,   bh, C2D_Color32(50,50,50,255));
    C2D_DrawRectSolid(bx,      by,      0.5f, bw*p, bh, CLR_GREEN);
    C2D_DrawRectSolid(bx,      by,      0.5f, bw,   2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,      by+bh-2, 0.5f, bw,   2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,      by,      0.5f, 2,    bh, CLR_WHITE);
    C2D_DrawRectSolid(bx+bw-2, by,      0.5f, 2,    bh, CLR_WHITE);
    char szDone[32], szTotal[32];
    formatSize((u64)done,  szDone,  sizeof(szDone));
    formatSize((u64)total, szTotal, sizeof(szTotal));
    char ps[64]; snprintf(ps, sizeof(ps), "%s / %s", szDone, szTotal);
    dt(bx+60.0f, by+25.0f, 0.5f, 0.48f, CLR_WHITE, ps);
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.48f, CLR_GRAY, T(STR_DO_NOT_POWER_OFF));
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
}

/* ============================================================
   Dialog screens (manage own C3D frames)
   ============================================================ */

void drawDialog(const char **lines, int lineCount) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG);    C2D_SceneBegin(top);
    _renderDialogBox(lines, lineCount);
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C3D_FrameEnd(0);
}

void drawSelectedTitlesList(void) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG);    C2D_SceneBegin(top);
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    _renderSelectedList();
    C3D_FrameEnd(0);
}

void drawDialogWithSelectedList(const char **lines, int lineCount) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG);    C2D_SceneBegin(top);
    _renderDialogBox(lines, lineCount);
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    _renderSelectedList();
    C3D_FrameEnd(0);
}

/* ============================================================
   Main menu
   ============================================================ */

void drawMainMenu(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    char titleLine[64]; snprintf(titleLine, sizeof(titleLine), T(STR_APP_TITLE_FMT), VERSION_STRING);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, titleLine);
    const char *items[] = {
        T(STR_MENU_INSTALL), T(STR_MENU_BACKUP), T(STR_MENU_UNINSTALL), T(STR_MENU_SYSINFO), T(STR_MENU_SETTINGS)
    };
    for (int i = 0; i < 5; i++) {
        float y = 60.0f + (float)i * 30.0f;
        if (i == menuCursor) C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 24, CLR_SELECTED);
        dt(30, y, 0.5f, 0.50f, (i == menuCursor) ? CLR_WHITE : CLR_GRAY, items[i]);
        if (i == menuCursor) dt(12, y, 0.5f, 0.54f, CLR_WHITE, ">");
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_MENU_FOOTER_TOP));
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    const char *descLine1[] = {
        T(STR_MENU_DESC_INSTALL_1), T(STR_MENU_DESC_BACKUP_1), T(STR_MENU_DESC_UNINSTALL_1),
        T(STR_MENU_DESC_SYSINFO_1), T(STR_MENU_DESC_SETTINGS_1)
    };
    const char *descLine2[] = {
        T(STR_MENU_DESC_INSTALL_2), T(STR_MENU_DESC_BACKUP_2), T(STR_MENU_DESC_UNINSTALL_2),
        T(STR_MENU_DESC_SYSINFO_2), ""
    };
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_MENU_DESCRIPTION));
    dt(8, 30, 0.5f, 0.54f, CLR_WHITE, descLine1[menuCursor]);
    dt(8, 52, 0.5f, 0.52f, CLR_GRAY,  descLine2[menuCursor]);
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_MENU_FOOTER_BOTTOM));
}

/* ============================================================
   Uninstall screen
   ============================================================ */

void drawTouchControls(void) {
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    if (filteredCount == 0) {
        dt(8, 110, 0.5f, 0.54f, CLR_GRAY, T(STR_NO_TITLES));
        return;
    }
    int idx = filteredIndices[cursor];
    TitleInfo *ti = &titles[idx];
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_TITLE_DETAILS));
    char nm[46]; strncpy(nm, ti->fullName[0] ? ti->fullName : ti->name, 44); nm[44] = '\0';
    dt(4, 22, 0.5f, 0.52f, CLR_WHITE, nm);
    char tid[36]; snprintf(tid, sizeof(tid), "ID: %016llX", (unsigned long long)ti->titleID);
    dt(4, 42, 0.5f, 0.52f, CLR_GRAY, tid);
    char ver[16]; snprintf(ver, sizeof(ver), "v%u", ti->version);
    char sz[24];  formatSize(ti->size, sz, sizeof(sz));
    char vs[48];  snprintf(vs, sizeof(vs), "%s   %s", ver, sz);
    dt(4, 60, 0.5f, 0.52f, CLR_GRAY, vs);
    u32 hi = (u32)(ti->titleID >> 32);
    const char *type = (hi == 0x0004000E) ? T(STR_TYPE_UPDATE) : (hi == 0x0004008C) ? T(STR_TYPE_DLC) : T(STR_TYPE_GAME);
    const char *loc  = (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND";
    char tl[32]; snprintf(tl, sizeof(tl), "%s  |  %s", type, loc);
    dt(4, 78, 0.5f, 0.52f, CLR_CYAN, tl);
    if (ti->hasBackup) {
        char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
        char bk[64]; snprintf(bk, sizeof(bk), T(STR_BACKUP_FMT), dateBuf);
        dt(4, 96, 0.5f, 0.52f, CLR_GREEN, bk);
    } else {
        dt(4, 96, 0.5f, 0.52f, CLR_RED, T(STR_NO_BACKUP));
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, T(STR_FOOTER_BACK_EXIT_HELP));
}

void drawUI(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 36, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, T(STR_UNINSTALL_HEADER));
    int selCount = 0;
    for (int i = 0; i < titleCount; i++)
        if (titles[i].selected && titles[i].isValid) selCount++;
    const char *sortNames[]   = { T(STR_SORT_NAME), T(STR_SORT_SIZE), T(STR_SORT_ID) };
    const char *filterNames[] = { T(STR_FILTER_ALL), T(STR_FILTER_UPD), T(STR_FILTER_DLC) };
    char infoP1[40];
    snprintf(infoP1, sizeof(infoP1), T(STR_UNINSTALL_INFO_FMT), filteredCount, selCount);
    float xi  = 4.0f + (float)strlen(infoP1) * 5.4f;
    const char *sn = sortNames[currentSortMode];
    float xi2 = xi  + (float)strlen(sn) * 5.4f;
    const char *fn = filterNames[currentFilterMode];
    char filtFull[12]; snprintf(filtFull, sizeof(filtFull), " [%s]", fn);
    u32 filtClr = (currentFilterMode != FILTER_ALL) ? CLR_YELLOW : CLR_GRAY;
    dt(4,   22, 0.5f, 0.40f, CLR_GRAY,   infoP1);
    dt(xi,  22, 0.5f, 0.40f, CLR_YELLOW, sn);
    dt(xi2, 22, 0.5f, 0.40f, filtClr,    filtFull);
    for (int i = 0; i < UNINSTALL_VISIBLE; i++) {
        int fi = scrollOffset + i;
        if (fi >= filteredCount) break;
        int idx = filteredIndices[fi];
        TitleInfo *ti = &titles[idx];
        float y = 40.0f + (float)i * 14.5f;
        bool isCursor = (fi == cursor);
        if (isCursor) C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
        u32 txtColor = ti->selected ? CLR_YELLOW : (isCursor ? CLR_WHITE : CLR_GRAY);
        dt(3, y, 0.5f, 0.38f, txtColor, ti->selected ? "[X]" : "[ ]");
        const char *srcName = ti->name[0] ? ti->name : T(STR_UNKNOWN);
        char nm[35]; strncpy(nm, srcName, 32); nm[32] = '\0';
        if (strlen(srcName) > 32) { nm[29]='.'; nm[30]='.'; nm[31]='.'; nm[32]='\0'; }
        dt(25, y, 0.5f, 0.38f, txtColor, nm);
        u32 hi = (u32)(ti->titleID >> 32);
        if      (hi == 0x0004000E) dt(240, y, 0.5f, 0.44f, CLR_CYAN,  "^");
        else if (hi == 0x0004008C) dt(240, y, 0.5f, 0.44f, CLR_GREEN, "+");
        char fullID[20]; snprintf(fullID, sizeof(fullID), "%016llX", (unsigned long long)ti->titleID);
        dt(292.0f, y, 0.5f, 0.38f, CLR_GRAY, fullID);
    }
    if (filteredCount > UNINSTALL_VISIBLE) {
        char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", cursor + 1, filteredCount);
        dt(340, 225, 0.5f, 0.52f, CLR_GRAY, sc);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_FOOTER_BACK_EXIT_HELP));
    drawTouchControls();
}

/* ============================================================
   File browser (Install CIA)
   ============================================================ */

void drawFileBrowserScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 36, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, T(STR_INSTALL_HEADER));
    const char *dispPath = currentPath;
    char pathBuf[52];
    int plen = (int)strlen(currentPath);
    if (plen > 48) { snprintf(pathBuf, sizeof(pathBuf), "...%s", currentPath + plen - 44); dispPath = pathBuf; }
    dt(4, 22, 0.5f, 0.40f, CLR_GRAY, dispPath);
    for (int i = 0; i < FILE_BROWSER_VISIBLE; i++) {
        int fi = fileScrollOffset + i;
        if (fi >= fileCount) break;
        float y = 40.0f + (float)i * 14.5f;
        bool isCursor = (fi == fileCursor);
        if (isCursor) C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
        FileEntry *fe = &fileEntries[fi];
        if (fe->isDir) {
            char dirLine[52]; snprintf(dirLine, sizeof(dirLine), T(STR_DIR_PREFIX_FMT), fe->name);
            if (strlen(dirLine) > 46) { dirLine[43]='.'; dirLine[44]='.'; dirLine[45]='.'; dirLine[46]='\0'; }
            dt(4, y, 0.5f, 0.38f, isCursor ? CLR_WHITE : CLR_CYAN, dirLine);
        } else {
            char nm[46]; strncpy(nm, fe->name, 44); nm[44] = '\0';
            if (strlen(fe->name) > 44) { nm[41]='.'; nm[42]='.'; nm[43]='.'; nm[44]='\0'; }
            dt(4, y, 0.5f, 0.38f, isCursor ? CLR_WHITE : CLR_GRAY, nm);
            u32 hi = (u32)(fe->titleID >> 32);
            if      (hi == 0x0004000E) dt(316, y, 0.5f, 0.44f, CLR_CYAN,  "^");
            else if (hi == 0x0004008C) dt(316, y, 0.5f, 0.44f, CLR_GREEN, "+");
            char szBuf[16]; formatSize(fe->size, szBuf, sizeof(szBuf));
            char szLine[20]; snprintf(szLine, sizeof(szLine), "[%s]", szBuf);
            float szX = 396.0f - (float)strlen(szLine) * 5.7f;
            dt(szX, y, 0.5f, 0.38f, CLR_GRAY, szLine);
        }
    }
    if (fileCount > MAX_VISIBLE_TITLES) {
        char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", fileCursor+1, fileCount);
        dt(340, 225, 0.5f, 0.52f, CLR_GRAY, sc);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_FOOTER_BACK_EXIT_HELP));
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_FILE_INFO));
    if (fileCount > 0 && fileCursor < fileCount) {
        FileEntry *fe = &fileEntries[fileCursor];
        if (fe->isDir) {
            if (strcmp(fe->name, "..") == 0) {
                dt(8, 26, 0.5f, 0.52f, CLR_CYAN, T(STR_GO_TO_PARENT));
            } else {
                char dline[48]; snprintf(dline, sizeof(dline), T(STR_FOLDER_FMT), fe->name);
                dt(8, 26, 0.5f, 0.52f, CLR_CYAN, dline);
            }
        } else {
            u64 tid = fe->titleID;
            char tidStr[36];
            if (tid) snprintf(tidStr, sizeof(tidStr), T(STR_FILE_ID_FMT), (unsigned long long)tid);
            else     snprintf(tidStr, sizeof(tidStr), "%s", T(STR_FILE_ID_NA));
            dt(8, 26, 0.5f, 0.54f, CLR_WHITE, tidStr);
            char szBuf[24]; formatSize(fe->size, szBuf, sizeof(szBuf));
            char szLine[36]; snprintf(szLine, sizeof(szLine), T(STR_FILE_SIZE_FMT), szBuf);
            dt(8, 46, 0.5f, 0.54f, CLR_GRAY, szLine);
            u32 hi2 = (u32)(tid >> 32);
            const char *typeStr = (hi2 == 0x0004000E) ? T(STR_TYPE_UPDATE_BADGE) :
                                  (hi2 == 0x0004008C) ? T(STR_TYPE_DLC_BADGE) : T(STR_TYPE_GAME_LABEL);
            u32 typeClr = (hi2 == 0x0004000E) ? CLR_CYAN : (hi2 == 0x0004008C) ? CLR_GREEN : CLR_GRAY;
            dt(8, 64, 0.5f, 0.52f, typeClr, typeStr);
        }
    } else {
        dt(8, 50, 0.5f, 0.52f, CLR_GRAY, T(STR_EMPTY_FOLDER));
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, T(STR_FOOTER_BACK_EXIT_HELP));
}

/* ============================================================
   Backup screen
   ============================================================ */

void drawBackupScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    int selCount = 0;
    for (int i = 0; i < backupTitleCount; i++)
        if (titles[backupIndices[i]].selected) selCount++;
    char hdr[64]; snprintf(hdr, sizeof(hdr), T(STR_BACKUP_HEADER_FMT), selCount);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, hdr);
    const char *bkSortNames[] = { T(STR_SORT_NAME), T(STR_SORT_SIZE), T(STR_SORT_ID) };
    char sortLbl[24]; snprintf(sortLbl, sizeof(sortLbl), T(STR_SORT_FMT), bkSortNames[backupSortMode]);
    float slX = 393.0f - (float)strlen(sortLbl) * 7.0f;
    dt(slX, 4, 0.5f, 0.52f, CLR_YELLOW, sortLbl);
    for (int i = 0; i < BACKUP_VISIBLE; i++) {
        int bi = backupScrollOffset + i;
        if (bi >= backupTitleCount) break;
        TitleInfo *t2 = &titles[backupIndices[bi]];
        float y = 26.0f + (float)i * 14.5f;
        bool isCursor = (bi == backupCursor);
        if (isCursor) C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
        const char *cb; u32 cbColor;
        if (t2->selected)       { cb = "[X]"; cbColor = CLR_YELLOW; }
        else if (t2->hasBackup) { cb = "[*]"; cbColor = CLR_BACKUP_OK; }
        else                    { cb = "[ ]"; cbColor = isCursor ? CLR_WHITE : CLR_GRAY; }
        dt(3, y, 0.5f, 0.38f, cbColor, cb);
        const char *srcName = t2->name[0] ? t2->name : T(STR_UNKNOWN);
        char nm[35]; strncpy(nm, srcName, 32); nm[32] = '\0';
        if (strlen(srcName) > 32) { nm[29]='.'; nm[30]='.'; nm[31]='.'; nm[32]='\0'; }
        u32 nameColor = t2->selected ? CLR_YELLOW : (isCursor ? CLR_WHITE : CLR_GRAY);
        dt(25, y, 0.5f, 0.38f, nameColor, nm);
        u32 hi = (u32)(t2->titleID >> 32);
        if      (hi == 0x0004000E) dt(240, y, 0.5f, 0.44f, CLR_CYAN,  "^");
        else if (hi == 0x0004008C) dt(240, y, 0.5f, 0.44f, CLR_GREEN, "+");
        char tidBuf[20]; snprintf(tidBuf, sizeof(tidBuf), "%016llX", (unsigned long long)t2->titleID);
        dt(292.0f, y, 0.5f, 0.38f, CLR_GRAY, tidBuf);
    }
    if (backupTitleCount > BACKUP_VISIBLE) {
        char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", backupCursor+1, backupTitleCount);
        dt(340, 225, 0.5f, 0.52f, CLR_GRAY, sc);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_FOOTER_BACK_EXIT_HELP));
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_TITLE_DETAILS));
    if (backupCursor < backupTitleCount) {
        TitleInfo *ti = &titles[backupIndices[backupCursor]];
        char nm2[46]; strncpy(nm2, ti->fullName[0] ? ti->fullName : ti->name, 44); nm2[44] = '\0';
        dt(4, 22, 0.5f, 0.52f, CLR_WHITE, nm2);
        char tidStr[36]; snprintf(tidStr, sizeof(tidStr), "ID: %016llX", (unsigned long long)ti->titleID);
        dt(4, 42, 0.5f, 0.52f, CLR_GRAY, tidStr);
        char szBuf[24]; formatSize(ti->size, szBuf, sizeof(szBuf));
        char locLine[52];
        snprintf(locLine, sizeof(locLine), "v%u   %s   %s", ti->version, szBuf, (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND");
        dt(4, 60, 0.5f, 0.52f, CLR_GRAY, locLine);
        if (ti->hasBackup) {
            char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
            char bkLine[52]; snprintf(bkLine, sizeof(bkLine), T(STR_BACKUP_FMT), dateBuf);
            dt(4, 80, 0.5f, 0.52f, CLR_GREEN, bkLine);
        } else {
            dt(4, 80, 0.5f, 0.52f, CLR_RED, T(STR_NO_BACKUP));
        }
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, T(STR_FOOTER_BACK_EXIT_HELP));
}

/* ============================================================
   System Info screen
   ============================================================ */

void drawSysInfoScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);

    if (sysInfoMode == SYSINFO_OVERVIEW) {
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
        dt(4, 4, 0.5f, 0.54f, CLR_WHITE, T(STR_SYSINFO_HEADER));
        int cntG=0, cntU=0, cntD=0;
        u64 szG=0, szU=0, szD=0;
        for (int i = 0; i < titleCount; i++) {
            if (!titles[i].isValid) continue;
            u32 hi = (u32)(titles[i].titleID >> 32);
            if      (hi == 0x00040000) { cntG++; szG += titles[i].size; }
            else if (hi == 0x0004000E) { cntU++; szU += titles[i].size; }
            else if (hi == 0x0004008C) { cntD++; szD += titles[i].size; }
        }
        char szBufG[24], szBufU[24], szBufD[24];
        formatSize(szG, szBufG, sizeof(szBufG));
        formatSize(szU, szBufU, sizeof(szBufU));
        formatSize(szD, szBufD, sizeof(szBufD));
        /* Label/count/size are drawn as three independently-positioned
           columns (not a single %-9s-padded string) so translated labels
           of any length never misalign the count/size columns. */
        const char *rowLabels[] = { T(STR_SYSINFO_ROW_GAMES), T(STR_SYSINFO_ROW_UPDATES), T(STR_SYSINFO_ROW_DLC) };
        int   rowCnt[] = { cntG, cntU, cntD };
        const char *rowSz[]  = { szBufG, szBufU, szBufD };
        for (int i = 0; i < 3; i++) {
            float y = 50.0f + (float)i * 36.0f;
            if (i == sysInfoCursor) C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 30, CLR_SELECTED);
            u32 rowColor = (i == sysInfoCursor) ? CLR_WHITE : CLR_GRAY;
            dt(24, y + 6, 0.5f, 0.45f, rowColor, rowLabels[i]);
            char cntStr[8]; snprintf(cntStr, sizeof(cntStr), "%3d", rowCnt[i]);
            dt(180, y + 6, 0.5f, 0.45f, rowColor, cntStr);
            dt(230, y + 6, 0.5f, 0.45f, rowColor, rowSz[i]);
        }
        C2D_DrawRectSolid(8, 162, 0.5f, 384, 1, CLR_GRAY);
        u64 freeB=0, totalB=0; getSDFreeSpace(&freeB, &totalB);
        char szFree[24], szTot[24];
        formatSize(freeB,  szFree, sizeof(szFree));
        formatSize(totalB, szTot,  sizeof(szTot));
        char sdLine[64]; snprintf(sdLine, sizeof(sdLine), T(STR_SYSINFO_SD_FREE_FMT), szFree, szTot);
        dt(8, 170, 0.5f, 0.52f, CLR_CYAN, sdLine);
        C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_FOOTER_BACK_EXIT_HELP));
        C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
        C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_SYSINFO_OVERVIEW));
        dt(8, 26, 0.5f, 0.52f, CLR_GRAY, T(STR_SYSINFO_HELP_1));
        dt(8, 44, 0.5f, 0.52f, CLR_GRAY, T(STR_SYSINFO_HELP_2));
        dt(8, 62, 0.5f, 0.52f, CLR_GRAY, T(STR_SYSINFO_HELP_3));
        C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_GRAY, T(STR_FOOTER_BACK_EXIT_HELP));
    } else {
        const char *siSortNames[] = { T(STR_SORT_NAME), T(STR_SORT_SIZE), T(STR_SORT_ID) };
        const char *hdrLabel =
            (sysInfoMode == SYSINFO_GAMES)   ? T(STR_SYSINFO_GAMES_HDR)   :
            (sysInfoMode == SYSINFO_UPDATES) ? T(STR_SYSINFO_UPDATES_HDR) : T(STR_SYSINFO_DLC_HDR);
        char hdr[48]; snprintf(hdr, sizeof(hdr), T(STR_SYSINFO_HDR_COUNT_FMT), hdrLabel, sysInfoSubCount);
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 28, CLR_HEADER);
        dt(4, 6, 0.5f, 0.54f, CLR_WHITE, hdr);
        char sortLbl[24]; snprintf(sortLbl, sizeof(sortLbl), T(STR_SORT_FMT), siSortNames[sysInfoSortMode]);
        float slX = 393.0f - (float)strlen(sortLbl) * 7.0f;
        dt(slX, 6, 0.5f, 0.52f, CLR_YELLOW, sortLbl);
        for (int i = 0; i < MAX_VISIBLE_TITLES; i++) {
            int si = sysInfoSubScrollOffset + i;
            if (si >= sysInfoSubCount) break;
            int idx = sysInfoSubIndices[si];
            TitleInfo *ti = &titles[idx];
            float y = 32.0f + (float)i * 14.5f;
            bool isCursor = (si == sysInfoSubCursor);
            if (isCursor) C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 15, CLR_SELECTED);
            char nm[34]; strncpy(nm, ti->name[0] ? ti->name : T(STR_UNKNOWN), 32); nm[32] = '\0';
            if (strlen(ti->name) > 32) { nm[29]='.'; nm[30]='.'; nm[31]='.'; nm[32]='\0'; }
            char szBuf[16]; formatSize(ti->size, szBuf, sizeof(szBuf));
            char szLine[20]; snprintf(szLine, sizeof(szLine), "[%s]", szBuf);
            u32 clr = isCursor ? CLR_WHITE : CLR_GRAY;
            dt(4, y, 0.5f, 0.38f, clr, nm);
            u32 thi = (u32)(ti->titleID >> 32);
            if      (thi == 0x0004000E) dt(240, y, 0.5f, 0.44f, CLR_CYAN,  "^");
            else if (thi == 0x0004008C) dt(240, y, 0.5f, 0.44f, CLR_GREEN, "+");
            float szX = 396.0f - (float)strlen(szLine) * 5.7f;
            dt(szX, y, 0.5f, 0.38f, CLR_GRAY, szLine);
        }
        if (sysInfoSubCount > MAX_VISIBLE_TITLES) {
            char sc[16]; snprintf(sc, sizeof(sc), "%d/%d", sysInfoSubCursor+1, sysInfoSubCount);
            dt(340, 225, 0.5f, 0.52f, CLR_GRAY, sc);
        }
        C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_FOOTER_BACK_EXIT_HELP));
        C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
        C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_TITLE_DETAILS));
        if (sysInfoSubCount > 0 && sysInfoSubCursor < sysInfoSubCount) {
            int idx = sysInfoSubIndices[sysInfoSubCursor];
            TitleInfo *ti = &titles[idx];
            char nm2[46]; strncpy(nm2, ti->fullName[0] ? ti->fullName : ti->name, 44); nm2[44] = '\0';
            dt(4, 22, 0.5f, 0.52f, CLR_WHITE, nm2);
            char tidStr[36]; snprintf(tidStr, sizeof(tidStr), "ID: %016llX", (unsigned long long)ti->titleID);
            dt(4, 42, 0.5f, 0.52f, CLR_GRAY, tidStr);
            char szBuf[24]; formatSize(ti->size, szBuf, sizeof(szBuf));
            char loc[52]; snprintf(loc, sizeof(loc), "v%u   %s   %s", ti->version, szBuf, (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND");
            dt(4, 60, 0.5f, 0.52f, CLR_GRAY, loc);
            if (ti->hasBackup) {
                char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
                char bkLine[48]; snprintf(bkLine, sizeof(bkLine), T(STR_BACKUP_FMT), dateBuf);
                dt(4, 78, 0.5f, 0.52f, CLR_GREEN, bkLine);
            } else {
                dt(4, 78, 0.5f, 0.52f, CLR_RED, T(STR_NO_BACKUP));
            }
            dt(4, 98, 0.5f, 0.52f, CLR_CYAN, T(STR_SYSINFO_A_DETAILS));
        }
        C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_GRAY, T(STR_FOOTER_BACK_EXIT_HELP));
    }
}

/* ============================================================
   Settings screen
   ============================================================ */

void drawSettingsScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, T(STR_SETTINGS_HEADER));
    const char *labels[] = {
        T(STR_SETTINGS_FORCE_BACKUP), T(STR_SETTINGS_SKIP_UNINSTALL), T(STR_SETTINGS_FORCE_RESTORE),
        T(STR_SETTINGS_SKIP_INSTALL), T(STR_SETTINGS_BACKUP_FOLDER), T(STR_SETTINGS_LANGUAGE)
    };
    const char *onOff[2] = { T(STR_SETTINGS_OFF), T(STR_SETTINGS_ON) };
    const char *values[6] = {
        onOff[config.forceBackup ? 1 : 0],
        onOff[config.skipUninstallConfirm ? 1 : 0],
        onOff[config.forceRestore ? 1 : 0],
        onOff[config.skipInstallConfirm ? 1 : 0],
        T(STR_SETTINGS_SEE_BOTTOM),
        config.language == LANG_IT ? "IT" : "EN"
    };
    /* 6 rows now that Language was added: tighter spacing (30px instead of
       32px, starting at y=34 instead of 38) keeps the last row's highlight
       clear of the y=222 footer bar. */
    for (int i = 0; i < 6; i++) {
        float y = 34.0f + (float)i * 30.0f;
        if (i == settingsCursor) C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 26, CLR_SELECTED);
        if (i == settingsCursor) dt(12, y + 2, 0.5f, 0.50f, CLR_WHITE, ">");
        dt(28, y + 2, 0.5f, 0.50f, (i == settingsCursor) ? CLR_WHITE : CLR_GRAY, labels[i]);
        if (i != 4) dt(260, y + 2, 0.5f, 0.50f, CLR_CYAN, values[i]);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, T(STR_FOOTER_BACK_EXIT_HELP));
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_SETTINGS_DESC_HEADER));
    const char *desc[6][3] = {
        { T(STR_SETTINGS_DESC_FORCEBK_1),        T(STR_SETTINGS_DESC_FORCEBK_2),        T(STR_SETTINGS_DESC_FORCEBK_3) },
        { T(STR_SETTINGS_DESC_SKIPUNINST_1),     T(STR_SETTINGS_DESC_SKIPUNINST_2),     T(STR_SETTINGS_DESC_SKIPUNINST_3) },
        { T(STR_SETTINGS_DESC_FORCERESTORE_1),   T(STR_SETTINGS_DESC_FORCERESTORE_2),   T(STR_SETTINGS_DESC_FORCERESTORE_3) },
        { T(STR_SETTINGS_DESC_SKIPINSTALL_1),    T(STR_SETTINGS_DESC_SKIPINSTALL_2),    T(STR_SETTINGS_DESC_SKIPINSTALL_3) },
        { T(STR_SETTINGS_DESC_BACKUPFOLDER_1),   T(STR_SETTINGS_DESC_BACKUPFOLDER_2),   T(STR_SETTINGS_DESC_BACKUPFOLDER_3) },
        { T(STR_SETTINGS_DESC_LANGUAGE_1),       T(STR_SETTINGS_DESC_LANGUAGE_2),       T(STR_SETTINGS_DESC_LANGUAGE_3) }
    };
    if (settingsCursor == 4) {
        dt(8, 28, 0.5f, 0.52f, CLR_WHITE, T(STR_SETTINGS_CURRENT_PATH));
        dt(8, 46, 0.5f, 0.44f, CLR_CYAN,  config.backupPath);
        dt(8, 66, 0.5f, 0.50f, CLR_GRAY,  desc[4][1]);
        dt(8, 84, 0.5f, 0.50f, CLR_GRAY,  desc[4][2]);
    } else {
        dt(8, 28, 0.5f, 0.52f, CLR_WHITE, desc[settingsCursor][0]);
        dt(8, 48, 0.5f, 0.52f, CLR_GRAY,  desc[settingsCursor][1]);
        dt(8, 68, 0.5f, 0.54f, CLR_CYAN,  desc[settingsCursor][2]);
    }
    C2D_DrawRectSolid(0, 204, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 206, 0.5f, 0.54f, CLR_GREEN, T(STR_SETTINGS_SAVED_REALTIME));
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, T(STR_FOOTER_BACK_EXIT_HELP));
}

/* ============================================================
   Title Details (SysInfo flow)
   ============================================================ */

void drawTitleDetails(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    int idx = sysInfoDetail.idx;
    TitleInfo *ti = &titles[idx];
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, T(STR_TITLE_DETAILS));
    char nm[50]; strncpy(nm, ti->fullName[0] ? ti->fullName : ti->name, 48); nm[48] = '\0';
    dt(4, 28, 0.5f, 0.52f, CLR_WHITE, nm);
    char tidLine[36]; snprintf(tidLine, sizeof(tidLine), "ID: %016llX", (unsigned long long)ti->titleID);
    dt(4, 48, 0.5f, 0.52f, CLR_GRAY, tidLine);
    char szBuf[24]; formatSize(ti->size, szBuf, sizeof(szBuf));
    char szLine[52]; snprintf(szLine, sizeof(szLine), "v%u   %s   %s", ti->version, szBuf, (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND");
    dt(4, 64, 0.5f, 0.52f, CLR_GRAY, szLine);
    if (ti->hasBackup) {
        char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
        char bkLine[52]; snprintf(bkLine, sizeof(bkLine), T(STR_BACKUP_FMT), dateBuf);
        dt(4, 80, 0.5f, 0.52f, CLR_GREEN, bkLine);
    } else {
        dt(4, 80, 0.5f, 0.52f, CLR_RED, T(STR_NO_BACKUP));
    }
    u32 hi = (u32)(ti->titleID >> 32);
    int relIdx[20]; int relCount = 0;
    if (hi == 0x00040000) findRelatedTitles(ti->titleID, idx, relIdx, &relCount);
    float actY = 100.0f;
    if (relCount > 0) {
        dt(4, 96, 0.5f, 0.52f, CLR_CYAN, T(STR_DETAIL_RELATED));
        for (int r = 0; r < relCount && r < 3; r++) {
            u32 rhi = (u32)(titles[relIdx[r]].titleID >> 32);
            char rl[42]; snprintf(rl, sizeof(rl), (rhi == 0x0004000E) ? T(STR_DETAIL_RELATED_UPD_FMT) : T(STR_DETAIL_RELATED_DLC_FMT), titles[relIdx[r]].name);
            dt(4, 108.0f + (float)r * 13.0f, 0.5f, 0.52f, CLR_GRAY, rl);
        }
        actY = 148.0f;
    }
    const char *actions[] = {
        T(STR_DETAIL_ACTION_BACKUP), T(STR_DETAIL_ACTION_RESTORE), T(STR_DETAIL_ACTION_DELETE)
    };
    for (int a = 0; a < 3; a++) {
        float y = actY + (float)a * 19.0f;
        if (a == sysInfoDetail.cursor) C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 18, CLR_SELECTED);
        dt(8, y, 0.5f, 0.38f, (a == sysInfoDetail.cursor) ? CLR_WHITE : CLR_GRAY, actions[a]);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.54f, CLR_WHITE, T(STR_DETAIL_FOOTER_NAV));
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, T(STR_DETAIL_ACTION_HEADER));
    const char *actDesc[3][2] = {
        { T(STR_DETAIL_DESC_BACKUP_1),  T(STR_DETAIL_DESC_BACKUP_2) },
        { T(STR_DETAIL_DESC_RESTORE_1), T(STR_DETAIL_DESC_RESTORE_2) },
        { T(STR_DETAIL_DESC_DELETE_1),  T(STR_DETAIL_DESC_DELETE_2) }
    };
    dt(8, 28, 0.5f, 0.52f, CLR_WHITE, actDesc[sysInfoDetail.cursor][0]);
    dt(8, 48, 0.5f, 0.52f, CLR_GRAY,  actDesc[sysInfoDetail.cursor][1]);
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, T(STR_DETAIL_CONFIRM_ACTION));
}

/* ============================================================
   Help overlay — drawn on the already-active top scene at z=0.7.
   Does NOT call C2D_SceneBegin or C2D_TargetClear.
   ============================================================ */

typedef struct { const char *key; const char *desc; } HelpEntry;

void drawHelpOverlay(AppState state) {
    HelpEntry mainMenuHelp[] = {
        { T(STR_HELP_KEY_UPDOWN), T(STR_HELP_DESC_NAV_MENU) },
        { "A",                    T(STR_HELP_DESC_ENTER_SCREEN) },
        { "START",                T(STR_HELP_DESC_EXIT_APP) },
        { NULL, NULL }
    };
    HelpEntry installHelp[] = {
        { "A",                        T(STR_HELP_DESC_ENTER_INSTALL) },
        { "Y",                        T(STR_HELP_DESC_INSTALL_ALL) },
        { "B",                        T(STR_HELP_DESC_GO_UP) },
        { T(STR_HELP_KEY_LEFTRIGHT),  T(STR_HELP_DESC_PAGE_JUMP) },
        { "START",                    T(STR_HELP_DESC_BACK_MENU) },
        { NULL, NULL }
    };
    HelpEntry backupHelp[] = {
        { "A",     T(STR_HELP_DESC_SELECT_TITLE) },
        { "X",     T(STR_HELP_DESC_BACKUP_SELECTED) },
        { "Y",     T(STR_HELP_DESC_BACKUP_ALL) },
        { "L/R",   T(STR_HELP_DESC_SORT) },
        { "B",     T(STR_HELP_DESC_BACK_MENU) },
        { NULL, NULL }
    };
    HelpEntry uninstallHelp[] = {
        { "A",     T(STR_HELP_DESC_TOGGLE_SELECTION) },
        { "X",     T(STR_HELP_DESC_START_UNINSTALL) },
        { "L/R",   T(STR_HELP_DESC_CYCLE_SORT) },
        { "Y",     T(STR_HELP_DESC_CYCLE_FILTER) },
        { "B",     T(STR_HELP_DESC_BACK_MENU) },
        { NULL, NULL }
    };
    HelpEntry sysInfoHelp[] = {
        { T(STR_HELP_KEY_UPDOWN), T(STR_HELP_DESC_NAVIGATE) },
        { "A",                    T(STR_HELP_DESC_OPEN_CATEGORY) },
        { "L/R",                  T(STR_HELP_DESC_SORT_CATEGORY) },
        { "B",                    T(STR_HELP_DESC_BACK) },
        { NULL, NULL }
    };
    HelpEntry settingsHelp[] = {
        { T(STR_HELP_KEY_UPDOWN),         T(STR_HELP_DESC_NAVIGATE) },
        { T(STR_HELP_KEY_A_LR_LEFTRIGHT), T(STR_HELP_DESC_CHANGE_VALUE) },
        { T(STR_HELP_KEY_B_START),        T(STR_HELP_DESC_SAVE_BACK) },
        { NULL, NULL }
    };

    const char *titles_str[] = {
        T(STR_HELP_TITLE_MAINMENU), T(STR_HELP_TITLE_INSTALL), T(STR_HELP_TITLE_BACKUP),
        T(STR_HELP_TITLE_UNINSTALL), T(STR_HELP_TITLE_SYSINFO), T(STR_HELP_TITLE_SETTINGS)
    };
    HelpEntry *helpTables[] = {
        mainMenuHelp, installHelp, backupHelp,
        uninstallHelp, sysInfoHelp, settingsHelp
    };

    /* indexed by AppState: MAIN_MENU=0 INSTALL=1 BACKUP=2 UNINSTALL=3 SYSINFO=4 SETTINGS=5 */
    const char *screenHints[] = {
        T(STR_HELP_HINT_MAINMENU), T(STR_HELP_HINT_INSTALL), T(STR_HELP_HINT_BACKUP),
        T(STR_HELP_HINT_UNINSTALL), T(STR_HELP_HINT_SYSINFO), T(STR_HELP_HINT_SETTINGS)
    };
    _Static_assert(sizeof(screenHints)/sizeof(screenHints[0]) == 6,
                   "screenHints must have one entry per AppState");

    int si = (int)state;
    if (si < 0 || si >= 6) si = 0;
    const HelpEntry *entries = helpTables[si];
    const char *title_str    = titles_str[si];

    float bw = 320.0f, bh = 180.0f;
    float bx = (400.0f - bw) / 2.0f;
    float by = (240.0f - bh) / 2.0f;
    C2D_DrawRectSolid(bx,      by,       0.7f, bw, bh, C2D_Color32(10,10,20,245));
    C2D_DrawRectSolid(bx,      by,       0.7f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,      by+bh-2,  0.7f, bw, 2,  CLR_WHITE);
    C2D_DrawRectSolid(bx,      by,       0.7f, 2,  bh, CLR_WHITE);
    C2D_DrawRectSolid(bx+bw-2, by,       0.7f, 2,  bh, CLR_WHITE);
    dt(bx + 8.0f, by + 6.0f,  0.7f, 0.54f, CLR_CYAN, title_str);
    C2D_DrawRectSolid(bx, by + 22.0f, 0.7f, bw, 1, CLR_GRAY);
    float ey = by + 28.0f;
    for (int i = 0; entries[i].key != NULL; i++, ey += 18.0f) {
        dt(bx + 8.0f,  ey, 0.7f, 0.44f, CLR_YELLOW, entries[i].key);
        dt(bx + 90.0f, ey, 0.7f, 0.44f, CLR_WHITE,  entries[i].desc);
    }
    C2D_DrawRectSolid(bx, by + bh - 20.0f, 0.7f, bw, 1, CLR_GRAY);
    dt(bx + 8.0f, by + bh - 16.0f, 0.7f, 0.44f, CLR_GRAY, T(STR_RELEASE_SELECT_RETURN));
    const char *hint = screenHints[si];
    if (hint[0]) {
        C2D_DrawRectSolid(bx, by + bh - 40.0f, 0.7f, bw, 1, C2D_Color32(80,80,120,200));
        dt(bx + 8.0f, by + bh - 36.0f, 0.7f, 0.40f, CLR_CYAN, hint);
    }
}
