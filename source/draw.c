/* ============================================================
   draw.c — all drawing / rendering functions
   ============================================================ */
#include "draw.h"
#include "globals.h"
#include "utils.h"
#include "titles.h"

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
    char hdr[64]; snprintf(hdr, sizeof(hdr), "Selected (%d)", selCount);
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
        char more[32]; snprintf(more, sizeof(more), "...and %d more", selCount - 10);
        dt(4, 22.0f + 10.0f * 20.0f, 0.5f, 0.52f, CLR_GRAY, more);
    }
}

/* ---- Internal: COMING SOON stub ---- */
static void _drawSoon(const char *feature) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, feature);
    dt(110, 95, 0.5f, 0.65f, CLR_YELLOW, "COMING SOON");
    dt(80, 135, 0.5f, 0.54f, CLR_GRAY, "Feature under development (v2.0).");
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "B / START: Back to menu");
}

/* ============================================================
   Loading / Progress screens
   ============================================================ */

void drawLoadingScreen(int current, int total, const char *status) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    { C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, "Loading..."); C2D_TextOptimize(&_t);
      C2D_DrawText(&_t, C2D_WithColor, 4, 4, 0.5f, 0.54f, 0.54f, CLR_WHITE); }
    { C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, "Loading titles..."); C2D_TextOptimize(&_t);
      C2D_DrawText(&_t, C2D_WithColor, 4, 28, 0.5f, 0.52f, 0.52f, CLR_WHITE); }
    if (status) {
        C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, status); C2D_TextOptimize(&_t);
        C2D_DrawText(&_t, C2D_WithColor, 4, 48, 0.5f, 0.48f, 0.48f, CLR_GRAY);
    }
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
        C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, pt); C2D_TextOptimize(&_t);
        C2D_DrawText(&_t, C2D_WithColor, bx+bw/2.0f-20.0f, by+25.0f, 0.5f, 0.48f, 0.48f, CLR_WHITE);
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
    { C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, "Installing CIA"); C2D_TextOptimize(&_t);
      C2D_DrawText(&_t, C2D_WithColor, 4, 4, 0.5f, 0.54f, 0.54f, CLR_WHITE); }
    int nameStart = (int)(strlen(ciaPath) - 1);
    while (nameStart > 0 && ciaPath[nameStart-1] != '/' && ciaPath[nameStart-1] != ':') nameStart--;
    char nb[52]; snprintf(nb, sizeof(nb), "%.50s", &ciaPath[nameStart]);
    { C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, nb); C2D_TextOptimize(&_t);
      C2D_DrawText(&_t, C2D_WithColor, 4, 28, 0.5f, 0.48f, 0.48f, CLR_GRAY); }
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
    { C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, ps); C2D_TextOptimize(&_t);
      C2D_DrawText(&_t, C2D_WithColor, bx+60.0f, by+25.0f, 0.5f, 0.48f, 0.48f, CLR_WHITE); }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    { C2D_Text _t; C2D_TextParse(&_t, dynamicBuf, "Do not power off the console."); C2D_TextOptimize(&_t);
      C2D_DrawText(&_t, C2D_WithColor, 4, 224, 0.5f, 0.48f, 0.48f, CLR_GRAY); }
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
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "3DS Fast Uninstall  " VERSION_STRING);
    static const char *items[] = {
        "Install CIA", "Backup Saves", "Uninstall Titles", "System Info", "Settings"
    };
    for (int i = 0; i < 5; i++) {
        float y = 60.0f + (float)i * 30.0f;
        if (i == menuCursor) C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 24, CLR_SELECTED);
        dt(30, y, 0.5f, 0.50f, (i == menuCursor) ? CLR_WHITE : CLR_GRAY, items[i]);
        if (i == menuCursor) dt(12, y, 0.5f, 0.54f, CLR_WHITE, ">");
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "START: Exit");
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    static const char *descLine1[] = {
        "Install .CIA files from the SD card.",
        "Backup save data for installed titles.",
        "Uninstall titles with optional backup.",
        "View info on games, updates and DLC.",
        "Configure application settings."
    };
    static const char *descLine2[] = {
        "Supports single file and batch install.",
        "Backup single title or entire library.",
        "Supports batch uninstall with X button.",
        "Includes backup, restore and delete.",
        ""
    };
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Description");
    dt(8, 30, 0.5f, 0.54f, CLR_WHITE, descLine1[menuCursor]);
    dt(8, 52, 0.5f, 0.52f, CLR_GRAY,  descLine2[menuCursor]);
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A: Select  START: Exit  SEL: Help");
}

/* ============================================================
   Uninstall screen
   ============================================================ */

void drawTouchControls(void) {
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    if (filteredCount == 0) {
        dt(8, 110, 0.5f, 0.54f, CLR_GRAY, "No titles to show.");
        return;
    }
    int idx = filteredIndices[cursor];
    TitleInfo *ti = &titles[idx];
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Title Details");
    char nm[46]; strncpy(nm, ti->fullName[0] ? ti->fullName : ti->name, 44); nm[44] = '\0';
    dt(4, 22, 0.5f, 0.52f, CLR_WHITE, nm);
    char tid[36]; snprintf(tid, sizeof(tid), "ID: %016llX", (unsigned long long)ti->titleID);
    dt(4, 42, 0.5f, 0.52f, CLR_GRAY, tid);
    char ver[16]; snprintf(ver, sizeof(ver), "v%u", ti->version);
    char sz[24];  formatSize(ti->size, sz, sizeof(sz));
    char vs[48];  snprintf(vs, sizeof(vs), "%s   %s", ver, sz);
    dt(4, 60, 0.5f, 0.52f, CLR_GRAY, vs);
    u32 hi = (u32)(ti->titleID >> 32);
    const char *type = (hi == 0x0004000E) ? "Update" : (hi == 0x0004008C) ? "DLC" : "Game";
    const char *loc  = (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND";
    char tl[32]; snprintf(tl, sizeof(tl), "%s  |  %s", type, loc);
    dt(4, 78, 0.5f, 0.52f, CLR_CYAN, tl);
    if (ti->hasBackup) {
        char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
        char bk[64]; snprintf(bk, sizeof(bk), "Backup: %s", dateBuf);
        dt(4, 96, 0.5f, 0.52f, CLR_GREEN, bk);
    } else {
        dt(4, 96, 0.5f, 0.52f, CLR_RED, "No backup");
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "B: Back  START: Exit  SELECT: Help");
}

void drawUI(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 36, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "Uninstall");
    int selCount = 0;
    for (int i = 0; i < titleCount; i++)
        if (titles[i].selected && titles[i].isValid) selCount++;
    static const char *sortNames[]   = { "Name", "Size", "ID" };
    static const char *filterNames[] = { "All", "Upd", "DLC" };
    char infoP1[36];
    snprintf(infoP1, sizeof(infoP1), "T:%-3d Sel:%-3d Sort:", filteredCount, selCount);
    float xi  = 4.0f + (float)strlen(infoP1) * 5.4f;
    const char *sn = sortNames[currentSortMode];
    float xi2 = xi  + (float)strlen(sn) * 5.4f;
    const char *fn = filterNames[currentFilterMode];
    char filtFull[8]; snprintf(filtFull, sizeof(filtFull), " [%s]", fn);
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
        const char *srcName = ti->name[0] ? ti->name : "Unknown";
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
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A: Sel  X: Delete  L/R: Sort  Y: Filt  B: Menu");
    drawTouchControls();
}

/* ============================================================
   File browser (Install CIA)
   ============================================================ */

void drawFileBrowserScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 36, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "Install CIA");
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
            char dirLine[52]; snprintf(dirLine, sizeof(dirLine), "[DIR] %s/", fe->name);
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
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A: Enter/Install  Y: All  B: Up  DX/SX: Page");
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "File Info");
    if (fileCount > 0 && fileCursor < fileCount) {
        FileEntry *fe = &fileEntries[fileCursor];
        if (fe->isDir) {
            if (strcmp(fe->name, "..") == 0) {
                dt(8, 26, 0.5f, 0.52f, CLR_CYAN, "Go to parent folder");
            } else {
                char dline[48]; snprintf(dline, sizeof(dline), "Folder: %.30s", fe->name);
                dt(8, 26, 0.5f, 0.52f, CLR_CYAN, dline);
            }
        } else {
            u64 tid = fe->titleID;
            char tidStr[36];
            if (tid) snprintf(tidStr, sizeof(tidStr), "ID: %016llX", (unsigned long long)tid);
            else     snprintf(tidStr, sizeof(tidStr), "ID: N/A");
            dt(8, 26, 0.5f, 0.54f, CLR_WHITE, tidStr);
            char szBuf[24]; formatSize(fe->size, szBuf, sizeof(szBuf));
            char szLine[36]; snprintf(szLine, sizeof(szLine), "Size: %s", szBuf);
            dt(8, 46, 0.5f, 0.54f, CLR_GRAY, szLine);
            u32 hi2 = (u32)(tid >> 32);
            const char *typeStr = (hi2 == 0x0004000E) ? "Type: Update  [^]" :
                                  (hi2 == 0x0004008C) ? "Type: DLC     [+]" : "Type: Game";
            u32 typeClr = (hi2 == 0x0004000E) ? CLR_CYAN : (hi2 == 0x0004008C) ? CLR_GREEN : CLR_GRAY;
            dt(8, 64, 0.5f, 0.52f, typeClr, typeStr);
        }
    } else {
        dt(8, 50, 0.5f, 0.52f, CLR_GRAY, "Empty folder");
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "All installs go to SD card");
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
    char hdr[56]; snprintf(hdr, sizeof(hdr), "Save Backups   Sel:%d", selCount);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, hdr);
    static const char *bkSortNames[] = { "Name", "Size", "ID" };
    char sortLbl[16]; snprintf(sortLbl, sizeof(sortLbl), "Sort:%s", bkSortNames[backupSortMode]);
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
        const char *srcName = t2->name[0] ? t2->name : "Unknown";
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
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A: Sel  X: Backup Sel  Y: Backup All  L/R: Sort  B: Menu");
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Title Details");
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
            char bkLine[52]; snprintf(bkLine, sizeof(bkLine), "Backup: %s", dateBuf);
            dt(4, 80, 0.5f, 0.52f, CLR_GREEN, bkLine);
        } else {
            dt(4, 80, 0.5f, 0.52f, CLR_RED, "No backup");
        }
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "X: Backup sel   Y: Backup all");
}

/* ============================================================
   System Info screen
   ============================================================ */

void drawSysInfoScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);

    if (sysInfoMode == SYSINFO_OVERVIEW) {
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
        dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "SYSTEM INFORMATION");
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
        const char *rowLabels[] = { "Games:   ", "Updates: ", "DLC:     " };
        int   rowCnt[] = { cntG, cntU, cntD };
        const char *rowSz[]  = { szBufG, szBufU, szBufD };
        for (int i = 0; i < 3; i++) {
            float y = 50.0f + (float)i * 36.0f;
            if (i == sysInfoCursor) C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 30, CLR_SELECTED);
            char line[64]; snprintf(line, sizeof(line), "  %-9s%3d    %s", rowLabels[i], rowCnt[i], rowSz[i]);
            dt(8, y + 6, 0.5f, 0.45f, (i == sysInfoCursor) ? CLR_WHITE : CLR_GRAY, line);
        }
        C2D_DrawRectSolid(8, 162, 0.5f, 384, 1, CLR_GRAY);
        u64 freeB=0, totalB=0; getSDFreeSpace(&freeB, &totalB);
        char szFree[24], szTot[24];
        formatSize(freeB,  szFree, sizeof(szFree));
        formatSize(totalB, szTot,  sizeof(szTot));
        char sdLine[64]; snprintf(sdLine, sizeof(sdLine), "  SD Free: %s / %s", szFree, szTot);
        dt(8, 170, 0.5f, 0.52f, CLR_CYAN, sdLine);
        C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A: Enter category   B: Menu");
        C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
        C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Overview");
        dt(8, 26, 0.5f, 0.52f, CLR_GRAY, "A: Open category list.");
        dt(8, 44, 0.5f, 0.52f, CLR_GRAY, "UP/DOWN: Select category.");
        dt(8, 62, 0.5f, 0.52f, CLR_GRAY, "B: Back to main menu.");
        C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "A: Open  Up/Down: Select  B: Menu");
    } else {
        static const char *siSortNames[] = { "Name", "Size", "ID" };
        const char *hdrLabel =
            (sysInfoMode == SYSINFO_GAMES)   ? "GAMES"   :
            (sysInfoMode == SYSINFO_UPDATES) ? "UPDATES" : "DLC";
        char hdr[40]; snprintf(hdr, sizeof(hdr), "%s (%d)", hdrLabel, sysInfoSubCount);
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 28, CLR_HEADER);
        dt(4, 6, 0.5f, 0.54f, CLR_WHITE, hdr);
        char sortLbl[16]; snprintf(sortLbl, sizeof(sortLbl), "Sort:%s", siSortNames[sysInfoSortMode]);
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
            char nm[34]; strncpy(nm, ti->name[0] ? ti->name : "Unknown", 32); nm[32] = '\0';
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
        dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A: Details  L/R: Sort  B: Back");
        C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
        C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Title Details");
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
                char bkLine[48]; snprintf(bkLine, sizeof(bkLine), "Backup: %s", dateBuf);
                dt(4, 78, 0.5f, 0.52f, CLR_GREEN, bkLine);
            } else {
                dt(4, 78, 0.5f, 0.52f, CLR_RED, "No backup");
            }
            dt(4, 98, 0.5f, 0.52f, CLR_CYAN, "A: Details & actions");
        }
        C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
        dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "A: Open  L/R: Sort  B: Back");
    }
}

/* ============================================================
   Settings screen
   ============================================================ */

void drawSettingsScreen(void) {
    C2D_TextBufClear(dynamicBuf);
    C2D_TargetClear(top, CLR_BG); C2D_SceneBegin(top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 22, CLR_HEADER);
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "SETTINGS");
    static const char *labels[] = {
        "Force Backup:", "Skip Uninstall Confirm:", "Force Restore:",
        "Skip Install Confirm:", "Backup Folder:"
    };
    const char *values[5] = {
        config.forceBackup          ? "ON" : "OFF",
        config.skipUninstallConfirm ? "ON" : "OFF",
        config.forceRestore         ? "ON" : "OFF",
        config.skipInstallConfirm   ? "ON" : "OFF",
        "<see bottom>"
    };
    for (int i = 0; i < 5; i++) {
        float y = 38.0f + (float)i * 32.0f;
        if (i == settingsCursor) C2D_DrawRectSolid(0, y - 2, 0.5f, 400, 26, CLR_SELECTED);
        if (i == settingsCursor) dt(12, y + 2, 0.5f, 0.50f, CLR_WHITE, ">");
        dt(28, y + 2, 0.5f, 0.50f, (i == settingsCursor) ? CLR_WHITE : CLR_GRAY, labels[i]);
        if (i < 4) dt(260, y + 2, 0.5f, 0.50f, CLR_CYAN, values[i]);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_WHITE, "A/R/L: Change  DX/SX: Change  B/START: Save & Back");
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Description");
    static const char *desc[5][3] = {
        { "Auto-backup saves before",          "every uninstall (no prompt).",         "Recommended: ON for safety." },
        { "Skip the uninstall prompt.",         "Title deletes immediately on X.",      "Recommended: OFF (always confirm)." },
        { "Auto-restore save data",             "after every successful CIA install.",  "Use only with up-to-date backups." },
        { "Skip the install prompt.",           "CIA installs immediately on A.",       "Recommended: OFF (always confirm)." },
        { "Folder for save backups.",           "Use Left/Right or L/R to cycle",       "through 5 available destinations." }
    };
    if (settingsCursor == 4) {
        dt(8, 28, 0.5f, 0.52f, CLR_WHITE, "Current path:");
        dt(8, 46, 0.5f, 0.44f, CLR_CYAN,  config.backupPath);
        dt(8, 66, 0.5f, 0.50f, CLR_GRAY,  desc[4][1]);
        dt(8, 84, 0.5f, 0.50f, CLR_GRAY,  desc[4][2]);
    } else {
        dt(8, 28, 0.5f, 0.52f, CLR_WHITE, desc[settingsCursor][0]);
        dt(8, 48, 0.5f, 0.52f, CLR_GRAY,  desc[settingsCursor][1]);
        dt(8, 68, 0.5f, 0.54f, CLR_CYAN,  desc[settingsCursor][2]);
    }
    C2D_DrawRectSolid(0, 204, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 206, 0.5f, 0.54f, CLR_GREEN, "Changes saved in real time.");
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "B/START: save & back to menu");
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
    dt(4, 4, 0.5f, 0.54f, CLR_WHITE, "Title Details");
    char nm[50]; strncpy(nm, ti->fullName[0] ? ti->fullName : ti->name, 48); nm[48] = '\0';
    dt(4, 28, 0.5f, 0.52f, CLR_WHITE, nm);
    char tidLine[36]; snprintf(tidLine, sizeof(tidLine), "ID: %016llX", (unsigned long long)ti->titleID);
    dt(4, 48, 0.5f, 0.52f, CLR_GRAY, tidLine);
    char szBuf[24]; formatSize(ti->size, szBuf, sizeof(szBuf));
    char szLine[52]; snprintf(szLine, sizeof(szLine), "v%u   %s   %s", ti->version, szBuf, (ti->mediaType == MEDIATYPE_SD) ? "SD" : "NAND");
    dt(4, 64, 0.5f, 0.52f, CLR_GRAY, szLine);
    if (ti->hasBackup) {
        char dateBuf[32]; getBackupLastDate(ti->titleID, dateBuf, sizeof(dateBuf));
        char bkLine[52]; snprintf(bkLine, sizeof(bkLine), "Backup: %s", dateBuf);
        dt(4, 80, 0.5f, 0.52f, CLR_GREEN, bkLine);
    } else {
        dt(4, 80, 0.5f, 0.52f, CLR_RED, "No backup");
    }
    u32 hi = (u32)(ti->titleID >> 32);
    int relIdx[20]; int relCount = 0;
    if (hi == 0x00040000) findRelatedTitles(ti->titleID, idx, relIdx, &relCount);
    float actY = 100.0f;
    if (relCount > 0) {
        dt(4, 96, 0.5f, 0.52f, CLR_CYAN, "Related:");
        for (int r = 0; r < relCount && r < 3; r++) {
            u32 rhi = (u32)(titles[relIdx[r]].titleID >> 32);
            char rl[42]; snprintf(rl, sizeof(rl), "  %s %.32s", (rhi == 0x0004000E) ? "[Upd]" : "[DLC]", titles[relIdx[r]].name);
            dt(4, 108.0f + (float)r * 13.0f, 0.5f, 0.52f, CLR_GRAY, rl);
        }
        actY = 148.0f;
    }
    static const char *actions[] = {
        "[A] Backup Save Data",
        "[Y] Restore Save Data",
        "[X] Delete Title (+ related)"
    };
    for (int a = 0; a < 3; a++) {
        float y = actY + (float)a * 19.0f;
        if (a == sysInfoDetail.cursor) C2D_DrawRectSolid(0, y - 1, 0.5f, 400, 18, CLR_SELECTED);
        dt(8, y, 0.5f, 0.38f, (a == sysInfoDetail.cursor) ? CLR_WHITE : CLR_GRAY, actions[a]);
    }
    C2D_DrawRectSolid(0, 222, 0.5f, 400, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.54f, CLR_WHITE, "Up/Down: Nav  A: Execute  B: Back");
    C2D_TargetClear(bottom, CLR_BG); C2D_SceneBegin(bottom);
    C2D_DrawRectSolid(0, 0, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 2, 0.5f, 0.54f, CLR_WHITE, "Action");
    static const char *actDesc[3][2] = {
        { "Save the title data",              "to the configured backup folder." },
        { "Restore data from backup",         "previously created." },
        { "Delete the title from the system", "including DLC, Updates and data." }
    };
    dt(8, 28, 0.5f, 0.52f, CLR_WHITE, actDesc[sysInfoDetail.cursor][0]);
    dt(8, 48, 0.5f, 0.52f, CLR_GRAY,  actDesc[sysInfoDetail.cursor][1]);
    C2D_DrawRectSolid(0, 222, 0.5f, 320, 18, CLR_HEADER);
    dt(4, 224, 0.5f, 0.52f, CLR_GRAY, "A: confirm action");
}

/* ============================================================
   Help overlay — drawn on the already-active top scene at z=0.7.
   Does NOT call C2D_SceneBegin or C2D_TargetClear.
   ============================================================ */

typedef struct { const char *key; const char *desc; } HelpEntry;

void drawHelpOverlay(AppState state) {
    static const HelpEntry mainMenuHelp[] = {
        { "Up/Down",  "Navigate menu" },
        { "A",        "Enter screen" },
        { "START",    "Exit app" },
        { NULL, NULL }
    };
    static const HelpEntry installHelp[] = {
        { "A",          "Enter folder / Install CIA" },
        { "Y",          "Install all in folder" },
        { "B",          "Go up one level" },
        { "Left/Right", "Page jump" },
        { "START",      "Back to menu" },
        { NULL, NULL }
    };
    static const HelpEntry backupHelp[] = {
        { "A",     "Select title" },
        { "X",     "Backup selected" },
        { "Y",     "Backup all titles" },
        { "L/R",   "Sort" },
        { "B",     "Back to menu" },
        { NULL, NULL }
    };
    static const HelpEntry uninstallHelp[] = {
        { "A",     "Toggle selection" },
        { "X",     "Start uninstall flow" },
        { "L/R",   "Cycle sort" },
        { "Y",     "Cycle filter" },
        { "B",     "Back to menu" },
        { NULL, NULL }
    };
    static const HelpEntry sysInfoHelp[] = {
        { "Up/Down", "Navigate" },
        { "A",       "Open category / details" },
        { "L/R",     "Sort (in category)" },
        { "B",       "Back" },
        { NULL, NULL }
    };
    static const HelpEntry settingsHelp[] = {
        { "Up/Down",          "Navigate" },
        { "A/Left/Right/L/R", "Change value" },
        { "B/START",          "Save and back" },
        { NULL, NULL }
    };

    static const char *titles_str[] = {
        "Main Menu Help",
        "Install CIA Help",
        "Backup Saves Help",
        "Uninstall Help",
        "System Info Help",
        "Settings Help"
    };
    static const HelpEntry *helpTables[] = {
        mainMenuHelp, installHelp, backupHelp,
        uninstallHelp, sysInfoHelp, settingsHelp
    };

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
    dt(bx + 8.0f, by + bh - 16.0f, 0.7f, 0.44f, CLR_GRAY, "Release SELECT to return.");
}
