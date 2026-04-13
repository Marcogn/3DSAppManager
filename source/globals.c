/* ============================================================
   globals.c — definitions of all shared global state
   ============================================================ */
#include "globals.h"

TitleInfo titles[MAX_TITLES];
int       titleCount      = 0;
int       cursor          = 0;
int       scrollOffset    = 0;
bool      needsRedraw     = true;
bool      titlesNeedRefresh = false;

Config config;

C3D_RenderTarget *top    = NULL;
C3D_RenderTarget *bottom = NULL;
C2D_TextBuf       dynamicBuf;

SortMode   currentSortMode    = SORT_BY_NAME;
FilterMode currentFilterMode  = FILTER_ALL;
int        filteredIndices[MAX_TITLES];
int        filteredCount  = 0;

AppState appState   = APP_MAIN_MENU;
int      menuCursor = 0;

FileEntry fileEntries[MAX_FILES];
int       fileCount        = 0;
int       fileCursor       = 0;
int       fileScrollOffset = 0;
char      currentPath[512];
int       dirCursorStack[DIR_STACK_MAX];
int       dirScrollStack[DIR_STACK_MAX];
int       dirStackDepth = 0;

int      backupCursor       = 0;
int      backupScrollOffset = 0;
SortMode backupSortMode     = SORT_BY_NAME;
int      backupIndices[MAX_TITLES];
int      backupTitleCount   = 0;

SysInfoMode sysInfoMode            = SYSINFO_OVERVIEW;
int         sysInfoCursor          = 0;
int         sysInfoSubCursor       = 0;
int         sysInfoSubScrollOffset = 0;
int         sysInfoSubIndices[MAX_TITLES];
int         sysInfoSubCount        = 0;
SortMode    sysInfoSortMode        = SORT_BY_NAME;
int         sysInfoDetailIdx       = 0;
int         sysInfoDetailCursor    = 0;

int settingsCursor = 0;

