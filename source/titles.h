#pragma once
/* ============================================================
   titles.h — title loading, sort/filter, backup-dir helpers
   ============================================================ */
#include "types.h"

/* Sort / filter */
int  compareTitlesByName(const void *a, const void *b);
int  compareTitlesBySize(const void *a, const void *b);
int  compareTitlesByID  (const void *a, const void *b);
void sortTitles(void);
void updateFilteredList(void);

/* Backup directory helpers */
void getBackupDirName (u64 titleID, const char *titleName, char *outName, size_t outSize);
bool findBackupDir    (u64 titleID, char *outDir, size_t outSize);
bool getBackupLastDate(u64 titleID, char *outDate, size_t outSize);
bool checkBackupExists(u64 titleID);
bool getSDFreeSpace   (u64 *freeBytes, u64 *totalBytes);

/* Title info */
void getTitleName (u64 titleID, FS_MediaType mediaType, char *outName, size_t outSize);
void getTitleInfo (TitleInfo *title);
u64  getTitleSize (u64 titleID, FS_MediaType mediaType);
void loadTitleIcon(TitleInfo *title);

/* Pure helpers (no global state) */
bool titlePassesSafetyFilter(u64 titleID, FS_MediaType mediaType);
bool smdhSelectName(const SMDH *smdh, u8 sysLang, char *outName, size_t outSize);

/* Title loading (optimized) */
void loadTitles(void);
void findRelatedTitles(u64 baseTitleID, int selfIdx, int *outIdx, int *outCount);

