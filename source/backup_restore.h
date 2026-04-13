#pragma once
/* ============================================================
   backup_restore.h — save backup, restore, and title deletion
   ============================================================ */
#include "types.h"

void copyDirectory      (FS_Archive archive, const char *srcPath, const char *dstPath);
void backupArchive      (FS_Archive archive, const char *basePath, const char *archiveName);
void backupSaveDataToPath(TitleInfo *title, const char *backupPath);
void backupSaveData     (TitleInfo *title);
void restoreDirectory   (FS_Archive archive, const char *srcPath, const char *dstPath);
bool restoreSaveData    (TitleInfo *title);
void deleteTitleCompletely(TitleInfo *title);
void deleteTitle        (TitleInfo *title);

