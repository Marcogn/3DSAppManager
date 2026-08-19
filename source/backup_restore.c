/* ============================================================
   backup_restore.c — save backup, restore, and title deletion
   ============================================================ */
#include "backup_restore.h"
#include "globals.h"
#include "utils.h"
#include "titles.h"
#include <unistd.h>

/* Forward declaration — defined in draw.c */
void drawDialog(const char **lines, int lineCount);

/* Recursively removes a directory tree this module just created for a
   failed backup attempt. Only ever called on a backupDir that did not
   exist before this attempt, so it can never contain real save data. */
static void removeCreatedTree(const char *path) {
    DIR *d = opendir(path);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            char sub[600]; snprintf(sub, sizeof(sub), "%s/%s", path, ent->d_name);
            struct stat st;
            if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode)) removeCreatedTree(sub);
            else remove(sub);
        }
        closedir(d);
    }
    rmdir(path);
}

/* Recursively copies `srcPath` from `archive` into `dstPath` on the SD card.
   Returns false if ANY entry that existed could not actually be read from
   the archive or written to the SD card (e.g. SD full, a corrupt archive
   read, or the directory itself failing to open after being listed by its
   parent) — this is a real backup failure, not to be reported as success.
   Returns true otherwise, including the trivial case of a directory that
   opens fine but is empty (a title that was never launched yet has no
   save — FSDIR_Read simply returns zero entries, which is not a failure).
   A file at or above MAX_FILE_SIZE is deliberately skipped (not a failure
   of the archive/SD card, just a policy cap on this app's side). */
bool copyDirectory(FS_Archive archive, const char *srcPath, const char *dstPath) {
    Handle dh; FS_Path fsp = fsMakePath(PATH_ASCII, srcPath);
    if (R_FAILED(FSUSER_OpenDirectory(&dh, archive, fsp))) return false;
    createDirectory(dstPath);
    bool ok = true;
    FS_DirectoryEntry ent[32]; u32 er = 0;
    while (true) {
        if (R_FAILED(FSDIR_Read(dh, &er, 32, ent))) { ok = false; break; }
        if (er == 0) break;
        for (u32 i = 0; i < er; i++) {
            char nm[256]; utf16_to_utf8((uint8_t*)nm, ent[i].name, 255); nm[255] = '\0';
            char sf[512], df[512];
            snprintf(sf, sizeof(sf), "%s/%s", srcPath, nm);
            snprintf(df, sizeof(df), "%s/%s", dstPath, nm);
            if (ent[i].attributes & FS_ATTRIBUTE_DIRECTORY) {
                if (!copyDirectory(archive, sf, df)) ok = false;
            } else {
                bool fileOk = false;
                Handle fh; FS_Path fp2 = fsMakePath(PATH_ASCII, sf);
                if (R_SUCCEEDED(FSUSER_OpenFile(&fh, archive, fp2, FS_OPEN_READ, 0))) {
                    u64 fsz = 0; FSFILE_GetSize(fh, &fsz);
                    if (fsz == 0 || fsz >= MAX_FILE_SIZE) {
                        fileOk = true; /* nothing to write, or deliberately skipped as too large */
                    } else {
                        void *buf = malloc(fsz);
                        if (buf) {
                            u32 br = 0;
                            if (R_SUCCEEDED(FSFILE_Read(fh, &br, 0, buf, fsz)) && br == fsz) {
                                FILE *out = fopen(df, "wb");
                                if (out) {
                                    size_t written = fwrite(buf, 1, br, out);
                                    fclose(out);
                                    if (written == (size_t)br) fileOk = true;
                                }
                            }
                            free(buf);
                        }
                    }
                    FSFILE_Close(fh);
                }
                if (!fileOk) ok = false;
            }
        }
    }
    FSDIR_Close(dh);
    return ok;
}

bool backupArchive(FS_Archive archive, const char *basePath, const char *archiveName) {
    char p[512]; snprintf(p, sizeof(p), "%s/%s", basePath, archiveName);
    createDirectory(p);
    return copyDirectory(archive, "/", p);
}

bool backupSaveDataToPath(TitleInfo *title, const char *backupPath) {
    createDirectory(backupPath);
    char dirName[320];
    char cleanName[80]; strncpy(cleanName, title->name, 79); cleanName[79] = '\0';
    char *p1 = strstr(cleanName, " Upd"); if (p1) *p1 = '\0';
    char *p2 = strstr(cleanName, " DLC"); if (p2) *p2 = '\0';
    getBackupDirName(title->titleID, cleanName, dirName, sizeof(dirName));
    char backupDir[512];
    snprintf(backupDir, sizeof(backupDir), "%s/%s", backupPath, dirName);
    /* Remember whether this is a fresh backup or a re-attempt over an
       existing one, so a failed re-attempt never wipes prior real data. */
    struct stat preStat; bool backupDirPreexisted = (stat(backupDir, &preStat) == 0 && S_ISDIR(preStat.st_mode));
    createDirectory(backupDir);
    /* createDirectory wraps mkdir which may fail silently (e.g. SD full);
       stat confirms the directory was actually created on the SD card. */
    struct stat bst; if (stat(backupDir, &bst) != 0 || !S_ISDIR(bst.st_mode)) return false;

    /* Track, per archive, whether it was reachable at all AND whether
       everything found in it actually got copied to the SD card — opening
       an archive but then failing mid-copy (e.g. SD fills up) must NOT be
       reported as success just because the open succeeded. A title whose
       archive opens but is empty (never launched yet) is a valid,
       successful backup of "nothing". */
    bool savedataArchiveOk = false, savedataCopyOk = true;
    bool extdataArchiveOk  = false, extdataCopyOk  = true;
    bool bossArchiveOk     = false, bossCopyOk     = true;

    u32 ap[] = {(u32)(title->titleID & 0xFFFFFFFF), (u32)((title->titleID >> 32) & 0xFFFFFFFF), title->mediaType, 0};
    FS_Path bap = {PATH_BINARY, 16, ap};
    FS_Archive sa;
    if (R_SUCCEEDED(FSUSER_OpenArchive(&sa, ARCHIVE_USER_SAVEDATA, bap))) {
        savedataArchiveOk = true;
        savedataCopyOk = backupArchive(sa, backupDir, "savedata");
        FSUSER_CloseArchive(sa);
    }
    u64 extID = 0;
    if (R_SUCCEEDED(AM_GetTitleExtDataId(&extID, title->mediaType, title->titleID)) && extID != 0) {
        FS_ExtSaveDataInfo ei = {.mediaType = title->mediaType, .unknown = 0, .reserved1 = 0, .saveId = extID, .reserved2 = 0};
        FS_Path ep = {PATH_BINARY, sizeof(FS_ExtSaveDataInfo), &ei};
        FS_Archive ea;
        if (R_SUCCEEDED(FSUSER_OpenArchive(&ea, ARCHIVE_EXTDATA, ep))) {
            extdataArchiveOk = true;
            extdataCopyOk = backupArchive(ea, backupDir, "extdata");
            FSUSER_CloseArchive(ea);
        }
        if (R_SUCCEEDED(FSUSER_OpenArchive(&ea, ARCHIVE_BOSS_EXTDATA, ep))) {
            bossArchiveOk = true;
            bossCopyOk = backupArchive(ea, backupDir, "boss_extdata");
            FSUSER_CloseArchive(ea);
        }
    }

    bool anyArchiveReachable = savedataArchiveOk || extdataArchiveOk || bossArchiveOk;
    bool anyCopyFailure = (savedataArchiveOk && !savedataCopyOk) ||
                          (extdataArchiveOk  && !extdataCopyOk)  ||
                          (bossArchiveOk     && !bossCopyOk);

    if (!anyArchiveReachable || anyCopyFailure) {
        /* Either nothing could be backed up at all, or something was
           reachable but failed to actually copy (e.g. SD full mid-write).
           If this was a brand-new backupDir, remove the whole stub tree we
           just created (however deeply nested) so it doesn't linger and get
           mistaken for a real backup later (the [*] indicator and restore
           flow only check for folder existence). If backupDir already
           existed — a re-backup attempt on top of a prior successful one —
           it is NOT deleted, and backup_info.txt / any per-archive folder
           this attempt never reached are left alone. NOTE this is not a
           full rollback: any individual file this attempt did manage to
           overwrite before the failure keeps its new (possibly incomplete)
           content — the same in-place-overwrite behavior every backup
           attempt has always had here — so a partially-failed re-backup can
           still leave the save subtree in a mixed old/new state even though
           the folder and metadata survive. */
        if (!backupDirPreexisted) removeCreatedTree(backupDir);
        title->hasBackup = backupDirPreexisted;
        return false;
    }

    /* Only write backup_info.txt once the attempt is known to have fully
       succeeded — otherwise a failed re-backup on top of a prior valid one
       could overwrite its "Backup Date" with today's date despite having
       copied nothing new. */
    time_t now = time(NULL); struct tm *tm2 = localtime(&now);
    char dateBuf[32] = "??/??/?? ??:??";
    if (tm2) snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d %02d:%02d",
        tm2->tm_mday, tm2->tm_mon + 1, tm2->tm_year + 1900, tm2->tm_hour, tm2->tm_min);
    char infoPath[512]; snprintf(infoPath, sizeof(infoPath), "%s/backup_info.txt", backupDir);
    FILE *info = fopen(infoPath, "w");
    if (info) {
        fprintf(info, "Title ID: %016llX\n",  (unsigned long long)title->titleID);
        fprintf(info, "Title Name: %s\n",      title->name);
        fprintf(info, "Media Type: %s\n",      title->mediaType == MEDIATYPE_SD ? "SD" : "NAND");
        fprintf(info, "Backup Date: %s\n",     dateBuf);
        fprintf(info, "Backup Path: %s\n",     backupPath);
        fclose(info);
    }

    title->hasBackup = true;
    return true;
}

bool backupSaveData(TitleInfo *title) {
    return backupSaveDataToPath(title, config.backupPath);
}

void restoreDirectory(FS_Archive archive, const char *srcPath, const char *dstPath) {
    DIR *d = opendir(srcPath); if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        char sf[512], df[512];
        snprintf(sf, sizeof(sf), "%s/%s", srcPath, ent->d_name);
        snprintf(df, sizeof(df), "%s/%s", dstPath, ent->d_name);
        struct stat st;
        if (stat(sf, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            FS_Path fp2 = fsMakePath(PATH_ASCII, df);
            FSUSER_CreateDirectory(archive, fp2, 0);
            restoreDirectory(archive, sf, df);
        } else {
            FILE *inf = fopen(sf, "rb"); if (!inf) continue;
            fseek(inf, 0, SEEK_END); long fsz = ftell(inf); fseek(inf, 0, SEEK_SET);
            if (fsz > 0 && fsz < MAX_FILE_SIZE) {
                void *buf = malloc(fsz);
                if (buf) {
                    if (fread(buf, 1, fsz, inf) == (size_t)fsz) {
                        FS_Path fp2 = fsMakePath(PATH_ASCII, df); Handle fh2;
                        if (R_SUCCEEDED(FSUSER_OpenFile(&fh2, archive, fp2, FS_OPEN_WRITE | FS_OPEN_CREATE, 0))) {
                            u32 bw = 0; FSFILE_Write(fh2, &bw, 0, buf, fsz, FS_WRITE_FLUSH); FSFILE_Close(fh2);
                        }
                    }
                    free(buf);
                }
            }
            fclose(inf);
        }
    }
    closedir(d);
}

bool restoreSaveData(TitleInfo *title) {
    char backupDir[512];
    if (!findBackupDir(title->titleID, backupDir, sizeof(backupDir))) return false;
    char savedataPath[512]; snprintf(savedataPath, sizeof(savedataPath), "%s/savedata", backupDir);
    struct stat st; if (stat(savedataPath, &st) != 0) return false;
    u32 ap[] = {(u32)(title->titleID & 0xFFFFFFFF), (u32)((title->titleID >> 32) & 0xFFFFFFFF), title->mediaType, 0};
    FS_Path bap = {PATH_BINARY, 16, ap};
    FS_Archive sa;
    if (R_FAILED(FSUSER_OpenArchive(&sa, ARCHIVE_USER_SAVEDATA, bap))) return false;
    restoreDirectory(sa, savedataPath, "/");
    FSUSER_CloseArchive(sa);
    return true;
}

void deleteTitleCompletely(TitleInfo *title) {
    u64 extID = 0;
    if (R_SUCCEEDED(AM_GetTitleExtDataId(&extID, title->mediaType, title->titleID)) && extID != 0) {
        FS_ExtSaveDataInfo ei = {.mediaType = title->mediaType, .unknown = 0, .reserved1 = 0, .saveId = extID, .reserved2 = 0};
        FSUSER_DeleteExtSaveData(ei);
        FS_Archive ba; FS_Path ep = {PATH_BINARY, sizeof(FS_ExtSaveDataInfo), &ei};
        if (R_SUCCEEDED(FSUSER_OpenArchive(&ba, ARCHIVE_BOSS_EXTDATA, ep))) FSUSER_CloseArchive(ba);
    }
    AM_DeleteTitle(title->mediaType, title->titleID);
}

void deleteTitle(TitleInfo *title) {
    deleteTitleCompletely(title);
    AM_TitleEntry te;
    Result res = AM_GetTitleInfo(title->mediaType, 1, &title->titleID, &te);
    if (R_FAILED(res) || res == 0xC8A04478) {
        title->isValid = false;
        const char *sl[] = {"", " Successfully deleted:", title->name, "", "All data removed.", "", "Press A to continue..."};
        drawDialog(sl, 7);
    } else {
        const char *fl[] = {"", " Failed to delete:", title->name, "", "Title may still be present.", "", "Press A to continue..."};
        drawDialog(fl, 7);
    }
    while (aptMainLoop()) { hidScanInput(); if (hidKeysDown() & KEY_A) break; }
}

