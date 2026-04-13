#pragma once
/* ============================================================
   globals.h — extern declarations for all shared global state
   ============================================================ */
#include "types.h"

/* Title list */
extern TitleInfo titles[MAX_TITLES];
extern int       titleCount;
extern int       cursor;
extern int       scrollOffset;
extern bool      needsRedraw;
extern bool      titlesNeedRefresh;

/* Config */
extern Config config;

/* Render targets */
extern C3D_RenderTarget *top;
extern C3D_RenderTarget *bottom;
extern C2D_TextBuf       dynamicBuf;

/* Sort / filter (Uninstall) */
extern SortMode   currentSortMode;
extern FilterMode currentFilterMode;
extern int        filteredIndices[MAX_TITLES];
extern int        filteredCount;

/* App state */
extern AppState appState;
extern int      menuCursor;

/* File browser */
extern FileEntry fileEntries[MAX_FILES];
extern int       fileCount;
extern int       fileCursor;
extern int       fileScrollOffset;
extern char      currentPath[512];
extern int       dirCursorStack[DIR_STACK_MAX];
extern int       dirScrollStack[DIR_STACK_MAX];
extern int       dirStackDepth;

/* Backup flow */
extern int      backupCursor;
extern int      backupScrollOffset;
extern SortMode backupSortMode;
extern int      backupIndices[MAX_TITLES];
extern int      backupTitleCount;

/* System Info */
extern SysInfoMode sysInfoMode;
extern int         sysInfoCursor;
extern int         sysInfoSubCursor;
extern int         sysInfoSubScrollOffset;
extern int         sysInfoSubIndices[MAX_TITLES];
extern int         sysInfoSubCount;
extern SortMode    sysInfoSortMode;
extern int         sysInfoDetailIdx;
extern int         sysInfoDetailCursor;

/* Settings */
extern int settingsCursor;

