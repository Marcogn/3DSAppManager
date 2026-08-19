/*
 * test_backup.c — tests for backup_restore.c (backup/restore/copy logic)
 * and titles.c's backup-directory helpers, using the fake FS_Archive
 * backend in shims/fake_fs.h to actually exercise archive traversal on
 * host. This is the module that shipped the v2.3.1 "backup silently
 * succeeds without copying anything" bug — see CHANGELOG.md and
 * CLAUDE.md's "Backup and deletion: invariants to preserve".
 *
 * Uses -DCONFIG_PATH and -DDEFAULT_BACKUP_PATH overrides (see Makefile),
 * plus its own /test/br_test and /test/br_restore roots on the real host
 * filesystem for backup/restore destination and source content.
 */
#include "../vendor/greatest.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "../../source/utils.c"
#include "../../source/titles.c"
#include "../../source/config.c"
#include "../../source/backup_restore.c"

#define BR_TEST_ROOT    "/test/br_test"
#define BR_RESTORE_ROOT "/test/br_restore"

/* ---- helpers ---- */

static void rmrf(const char *path) {
    DIR *d = opendir(path);
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            char sub[600]; snprintf(sub, sizeof(sub), "%s/%s", path, ent->d_name);
            struct stat st;
            if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode)) rmrf(sub);
            else remove(sub);
        }
        closedir(d);
    }
    rmdir(path);
}

static bool dirExists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool readWholeFile(const char *path, char *buf, size_t bufSize) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    size_t n = fread(buf, 1, bufSize - 1, f);
    fclose(f);
    buf[n] = '\0';
    return true;
}

static TitleInfo makeTitle(u64 tid, const char *name) {
    TitleInfo t; memset(&t, 0, sizeof(t));
    t.titleID = tid;
    t.mediaType = MEDIATYPE_SD;
    t.isValid = true;
    snprintf(t.name, sizeof(t.name), "%s", name);
    return t;
}

/* Computes the exact on-disk backup dir path backupSaveDataToPath would
   use, so tests can inspect its contents directly. */
static void expectedBackupDir(const char *root, u64 tid, const char *name, char *out, size_t outSize) {
    char dirName[320];
    getBackupDirName(tid, name, dirName, sizeof(dirName));
    snprintf(out, outSize, "%s/%s", root, dirName);
}

/* ---- backupSaveDataToPath ---- */

TEST test_backup_success_with_savedata_files(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset();
    FakeFsArchive *sa = fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false);
    fakeFsAddFile(sa, "/", "save.dat", "HELLO", 5);

    TitleInfo t = makeTitle(0x0004000012345678ULL, "TestGame");
    bool ok = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(t.hasBackup, true);

    char dir[512]; expectedBackupDir(BR_TEST_ROOT, t.titleID, "TestGame", dir, sizeof(dir));
    char savePath[600]; snprintf(savePath, sizeof(savePath), "%s/savedata/save.dat", dir);
    char content[64]; memset(content, 0, sizeof(content));
    ASSERT_EQ(readWholeFile(savePath, content, sizeof(content)), true);
    ASSERT_STR_EQ(content, "HELLO");

    char infoPath[600]; snprintf(infoPath, sizeof(infoPath), "%s/backup_info.txt", dir);
    char info[512]; memset(info, 0, sizeof(info));
    ASSERT_EQ(readWholeFile(infoPath, info, sizeof(info)), true);
    ASSERT(strstr(info, "TestGame") != NULL);

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

TEST test_backup_empty_savedata_still_succeeds(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset();
    fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false); /* opens fine, no files: never launched */

    TitleInfo t = makeTitle(0x0004000000000001ULL, "NeverPlayed");
    bool ok = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(t.hasBackup, true);

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

TEST test_backup_completely_unreachable_fails_and_cleans_up(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset(); /* nothing registered: savedata archive can't open, no extdata id */

    TitleInfo t = makeTitle(0x0004000000000002ULL, "NoArchive");
    bool ok = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok, false);
    ASSERT_EQ(t.hasBackup, false);

    char dir[512]; expectedBackupDir(BR_TEST_ROOT, t.titleID, "NoArchive", dir, sizeof(dir));
    ASSERT_EQ(dirExists(dir), false); /* stub folder must not linger */

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

/* This is the core regression test for the v2.3.1 fix: an archive that
   OPENS successfully but fails mid-copy must be reported as a failure,
   not masked as success just because the open succeeded. */
TEST test_backup_copy_failure_not_masked_by_open_success(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset();
    FakeFsArchive *sa = fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false);
    FakeFsNode *f = fakeFsAddFile(sa, "/", "save.dat", "DATA", 4);
    f->readFails = true; /* archive opens fine, but this file can't actually be read */

    TitleInfo t = makeTitle(0x0004000000000003ULL, "CorruptSave");
    bool ok = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok, false);
    ASSERT_EQ(t.hasBackup, false);

    char dir[512]; expectedBackupDir(BR_TEST_ROOT, t.titleID, "CorruptSave", dir, sizeof(dir));
    ASSERT_EQ(dirExists(dir), false); /* fresh failed attempt must be cleaned up */

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

/* A transient failure on a re-backup attempt must never destroy or alter
   an existing valid backup (folder, files, or backup_info.txt). */
TEST test_backup_reattempt_failure_preserves_prior_backup(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset();
    FakeFsArchive *sa = fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false);
    fakeFsAddFile(sa, "/", "save.dat", "ORIGINAL", 8);

    TitleInfo t = makeTitle(0x0004000000000004ULL, "ReattemptGame");
    ASSERT_EQ(backupSaveDataToPath(&t, BR_TEST_ROOT), true);

    char dir[512]; expectedBackupDir(BR_TEST_ROOT, t.titleID, "ReattemptGame", dir, sizeof(dir));
    char infoPath[600]; snprintf(infoPath, sizeof(infoPath), "%s/backup_info.txt", dir);
    char origInfo[512]; memset(origInfo, 0, sizeof(origInfo));
    ASSERT_EQ(readWholeFile(infoPath, origInfo, sizeof(origInfo)), true);

    /* Second attempt over the same backupDir fails entirely. */
    fakeFsReset();
    fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, true); /* archive can't even open this time */
    t.hasBackup = true; /* reflects the real app's state before a re-backup attempt */
    bool ok2 = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok2, false);

    /* Prior backup must survive completely untouched. */
    ASSERT_EQ(dirExists(dir), true);
    char infoAfter[512]; memset(infoAfter, 0, sizeof(infoAfter));
    ASSERT_EQ(readWholeFile(infoPath, infoAfter, sizeof(infoAfter)), true);
    ASSERT_STR_EQ(origInfo, infoAfter);
    char savePath[600]; snprintf(savePath, sizeof(savePath), "%s/savedata/save.dat", dir);
    char saveContent[64]; memset(saveContent, 0, sizeof(saveContent));
    ASSERT_EQ(readWholeFile(savePath, saveContent, sizeof(saveContent)), true);
    ASSERT_STR_EQ(saveContent, "ORIGINAL");

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

/* A file at/above MAX_FILE_SIZE is a deliberate policy skip, not a
   failure — it must not sink an otherwise-successful backup. */
TEST test_backup_oversized_file_skipped_not_failed(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset();
    FakeFsArchive *sa = fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false);
    fakeFsAddFile(sa, "/", "normal.dat", "SMALL", 5);
    fakeFsAddOversizedFile(sa, "/", "huge.dat", (u64)MAX_FILE_SIZE + 1);

    TitleInfo t = makeTitle(0x0004000000000005ULL, "OversizedGame");
    bool ok = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(t.hasBackup, true);

    char dir[512]; expectedBackupDir(BR_TEST_ROOT, t.titleID, "OversizedGame", dir, sizeof(dir));
    char normalPath[600]; snprintf(normalPath, sizeof(normalPath), "%s/savedata/normal.dat", dir);
    char hugePath[600]; snprintf(hugePath, sizeof(hugePath), "%s/savedata/huge.dat", dir);
    char content[64]; memset(content, 0, sizeof(content));
    ASSERT_EQ(readWholeFile(normalPath, content, sizeof(content)), true);
    ASSERT_STR_EQ(content, "SMALL");
    struct stat hst;
    ASSERT_EQ(stat(hugePath, &hst), -1); /* the oversized file must not have been written */

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

/* A listed subdirectory that fails to open is a real failure, not a
   legitimately-empty directory — must propagate, not be swallowed. */
TEST test_backup_nested_dir_open_failure_propagates(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset();
    FakeFsArchive *sa = fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false);
    FakeFsNode *sub = fakeFsAddDir(sa, "/", "sub");
    sub->openFails = true;

    TitleInfo t = makeTitle(0x0004000000000006ULL, "BrokenSubdir");
    bool ok = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok, false);
    ASSERT_EQ(t.hasBackup, false);

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

/* extdata alone (no savedata archive at all) is still a valid, successful
   backup — the multi-archive OR logic must keep working. */
TEST test_backup_extdata_only_still_succeeds(void) {
    rmrf(BR_TEST_ROOT);
    fakeFsReset();
    /* ARCHIVE_USER_SAVEDATA intentionally not registered: open fails. */
    fakeAmSetExtDataId(0x0004000000000007ULL, 0xABCD);
    FakeFsArchive *ea = fakeFsRegisterArchive(ARCHIVE_EXTDATA, false);
    fakeFsAddFile(ea, "/", "ext.dat", "EXTRA", 5);

    TitleInfo t = makeTitle(0x0004000000000007ULL, "ExtdataOnly");
    bool ok = backupSaveDataToPath(&t, BR_TEST_ROOT);
    ASSERT_EQ(ok, true);
    ASSERT_EQ(t.hasBackup, true);

    char dir[512]; expectedBackupDir(BR_TEST_ROOT, t.titleID, "ExtdataOnly", dir, sizeof(dir));
    char extPath[600]; snprintf(extPath, sizeof(extPath), "%s/extdata/ext.dat", dir);
    char content[64]; memset(content, 0, sizeof(content));
    ASSERT_EQ(readWholeFile(extPath, content, sizeof(content)), true);
    ASSERT_STR_EQ(content, "EXTRA");

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

/* ---- restoreSaveData / restoreDirectory ---- */

TEST test_restore_writes_files_into_archive(void) {
    rmrf(BR_RESTORE_ROOT);
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", BR_RESTORE_ROOT);

    TitleInfo t = makeTitle(0x0004000000000008ULL, "RestoreGame");
    char dirName[320]; getBackupDirName(t.titleID, "RestoreGame", dirName, sizeof(dirName));
    char dir[512]; snprintf(dir, sizeof(dir), "%s/%s", BR_RESTORE_ROOT, dirName);
    char saveDir[600]; snprintf(saveDir, sizeof(saveDir), "%s/savedata", dir);
    createDirectory(saveDir);
    char filePath[700]; snprintf(filePath, sizeof(filePath), "%s/restored.dat", saveDir);
    FILE *f = fopen(filePath, "wb");
    ASSERT(f != NULL);
    fwrite("RESTORED", 1, 8, f);
    fclose(f);

    fakeFsReset();
    FakeFsArchive *sa = fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false);

    bool ok = restoreSaveData(&t);
    ASSERT_EQ(ok, true);

    FakeFsNode *written = fakeFsFindNode(sa, "/restored.dat");
    ASSERT(written != NULL);
    ASSERT_EQ(written->size, (u64)8);
    ASSERT_EQ(memcmp(written->data, "RESTORED", 8), 0);

    rmrf(BR_RESTORE_ROOT);
    GREATEST_PASS();
}

TEST test_restore_no_backup_found_returns_false(void) {
    rmrf(BR_RESTORE_ROOT);
    createDirectory(BR_RESTORE_ROOT);
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", BR_RESTORE_ROOT);

    fakeFsReset();
    fakeFsRegisterArchive(ARCHIVE_USER_SAVEDATA, false);

    TitleInfo t = makeTitle(0x0004000000000009ULL, "NeverBackedUp");
    ASSERT_EQ(restoreSaveData(&t), false);

    rmrf(BR_RESTORE_ROOT);
    GREATEST_PASS();
}

/* ---- titles.c backup-dir helpers (pure host FS, no fake archive needed) ---- */

TEST test_getBackupDirName_format(void) {
    char out[320];
    getBackupDirName(0x000400000ABCDEF1ULL, "My Game", out, sizeof(out));
    ASSERT_STR_EQ(out, "000400000ABCDEF1-My Game");
    GREATEST_PASS();
}

TEST test_findBackupDir_and_checkBackupExists(void) {
    rmrf(BR_TEST_ROOT);
    createDirectory(BR_TEST_ROOT);
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", BR_TEST_ROOT);

    u64 tid = 0x000400000000BEEFULL;
    ASSERT_EQ(checkBackupExists(tid), false);

    char dirName[320]; getBackupDirName(tid, "BeefGame", dirName, sizeof(dirName));
    char dir[512]; snprintf(dir, sizeof(dir), "%s/%s", BR_TEST_ROOT, dirName);
    createDirectory(dir);

    ASSERT_EQ(checkBackupExists(tid), true);
    char found[512];
    ASSERT_EQ(findBackupDir(tid, found, sizeof(found)), true);
    ASSERT_STR_EQ(found, dir);

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

TEST test_getBackupLastDate_reads_info_file(void) {
    rmrf(BR_TEST_ROOT);
    createDirectory(BR_TEST_ROOT);
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", BR_TEST_ROOT);

    u64 tid = 0x0004000000000CAFEULL;
    char dirName[320]; getBackupDirName(tid, "CafeGame", dirName, sizeof(dirName));
    char dir[512]; snprintf(dir, sizeof(dir), "%s/%s", BR_TEST_ROOT, dirName);
    createDirectory(dir);
    char infoPath[600]; snprintf(infoPath, sizeof(infoPath), "%s/backup_info.txt", dir);
    FILE *f = fopen(infoPath, "w");
    ASSERT(f != NULL);
    fprintf(f, "Title ID: %016llX\n", (unsigned long long)tid);
    fprintf(f, "Backup Date: 25/12/2025 10:30\n");
    fclose(f);

    char date[32];
    ASSERT_EQ(getBackupLastDate(tid, date, sizeof(date)), true);
    ASSERT_STR_EQ(date, "25/12/2025 10:30");

    rmrf(BR_TEST_ROOT);
    GREATEST_PASS();
}

SUITE(suite_backup) {
    RUN_TEST(test_backup_success_with_savedata_files);
    RUN_TEST(test_backup_empty_savedata_still_succeeds);
    RUN_TEST(test_backup_completely_unreachable_fails_and_cleans_up);
    RUN_TEST(test_backup_copy_failure_not_masked_by_open_success);
    RUN_TEST(test_backup_reattempt_failure_preserves_prior_backup);
    RUN_TEST(test_backup_oversized_file_skipped_not_failed);
    RUN_TEST(test_backup_nested_dir_open_failure_propagates);
    RUN_TEST(test_backup_extdata_only_still_succeeds);
    RUN_TEST(test_restore_writes_files_into_archive);
    RUN_TEST(test_restore_no_backup_found_returns_false);
    RUN_TEST(test_getBackupDirName_format);
    RUN_TEST(test_findBackupDir_and_checkBackupExists);
    RUN_TEST(test_getBackupLastDate_reads_info_file);
}

int main(void) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(suite_backup);
    GREATEST_MAIN_END();
}
