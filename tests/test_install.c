/*
 * test_install.c — tests for install.c's pure, host-testable logic:
 * getCIATitleID (binary CIA header parsing) and scanDirectory (folder
 * listing/classification). Both operate on real files via plain libc I/O
 * (fopen/fread/opendir), so they need no 3DS-specific stubbing at all —
 * unlike installCIA itself (AM_StartCiaInstall/AM_FinishCiaInstall), which
 * remains untested here (see CLAUDE.md's "Known gotchas").
 */
#include "../vendor/greatest.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "../../source/utils.c"
#include "../../source/install.c"

#define INSTALL_TEST_ROOT "/test/install_test"

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

static void putU32LE(u8 *buf, u32 val) {
    buf[0] = (u8)(val & 0xFF);
    buf[1] = (u8)((val >> 8) & 0xFF);
    buf[2] = (u8)((val >> 16) & 0xFF);
    buf[3] = (u8)((val >> 24) & 0xFF);
}

static void putU64BE(u8 *buf, u64 val) {
    for (int i = 0; i < 8; i++) buf[i] = (u8)((val >> ((7 - i) * 8)) & 0xFF);
}

/* Builds a minimal, self-consistent synthetic CIA header (not a real
   playable CIA, just enough for getCIATitleID's offset arithmetic to land
   on a title ID we control) with a 64-byte header/cert/ticket (already
   64-aligned, so ALIGN64 is a no-op) and an RSA-2048 TMD signature
   (sigType 0x00010004 -> sigSz=0x100, padSz=0x3C, install.c's default
   branch). Returns the total file size written. */
static size_t writeSyntheticCia(const char *path, u64 titleID) {
    u8 buf[700]; memset(buf, 0, sizeof(buf));
    putU32LE(&buf[0], 0x40);   /* hdrSz */
    putU32LE(&buf[8], 0x40);   /* certSz */
    putU32LE(&buf[12], 0x40);  /* tikSz */
    putU32LE(&buf[16], 0x200); /* tmdSz (must be >= 0x200) */
    u32 tmdOff = 0x40 + 0x40 + 0x40; /* 0xC0 = 192, already 64-aligned */
    putU32LE(&buf[tmdOff], 0x00010004); /* sigType: RSA_2048 */
    u32 bodyOff = tmdOff + 4 + 0x100 + 0x3C; /* 512 */
    putU64BE(&buf[bodyOff + 0x4C], titleID);
    size_t total = bodyOff + 0x4C + 8;

    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fwrite(buf, 1, total, f);
    fclose(f);
    return total;
}

/* ---- getCIATitleID ---- */

TEST test_getCIATitleID_parses_valid_header(void) {
    rmrf(INSTALL_TEST_ROOT);
    createDirectory(INSTALL_TEST_ROOT);
    char path[256]; snprintf(path, sizeof(path), "%s/game.cia", INSTALL_TEST_ROOT);
    u64 tid = 0x0004000012345678ULL;
    ASSERT(writeSyntheticCia(path, tid) > 0);

    ASSERT_EQ(getCIATitleID(path), tid);

    rmrf(INSTALL_TEST_ROOT);
    GREATEST_PASS();
}

TEST test_getCIATitleID_missing_file_returns_zero(void) {
    ASSERT_EQ(getCIATitleID("/test/install_test/does_not_exist.cia"), (u64)0);
    GREATEST_PASS();
}

TEST test_getCIATitleID_truncated_tmd_returns_zero(void) {
    rmrf(INSTALL_TEST_ROOT);
    createDirectory(INSTALL_TEST_ROOT);
    char path[256]; snprintf(path, sizeof(path), "%s/bad.cia", INSTALL_TEST_ROOT);
    u8 buf[32]; memset(buf, 0, sizeof(buf));
    putU32LE(&buf[0], 0x40);
    putU32LE(&buf[8], 0x40);
    putU32LE(&buf[12], 0x40);
    putU32LE(&buf[16], 0x10); /* tmdSz < 0x200: must be rejected */
    FILE *f = fopen(path, "wb");
    ASSERT(f != NULL);
    fwrite(buf, 1, sizeof(buf), f);
    fclose(f);

    ASSERT_EQ(getCIATitleID(path), (u64)0);

    rmrf(INSTALL_TEST_ROOT);
    GREATEST_PASS();
}

/* ---- scanDirectory ---- */

TEST test_scanDirectory_classifies_dirs_and_cia_files(void) {
    rmrf(INSTALL_TEST_ROOT);
    createDirectory(INSTALL_TEST_ROOT);
    char subdir[300]; snprintf(subdir, sizeof(subdir), "%s/SubFolder", INSTALL_TEST_ROOT);
    createDirectory(subdir);
    char ciaPath[300]; snprintf(ciaPath, sizeof(ciaPath), "%s/Game.cia", INSTALL_TEST_ROOT);
    writeSyntheticCia(ciaPath, 0x0004000000001111ULL);
    char txtPath[300]; snprintf(txtPath, sizeof(txtPath), "%s/notes.txt", INSTALL_TEST_ROOT);
    FILE *f = fopen(txtPath, "w"); ASSERT(f != NULL); fputs("hi", f); fclose(f);

    int n = scanDirectory(INSTALL_TEST_ROOT);
    ASSERT(n >= 2); /* SubFolder + Game.cia at least; notes.txt is not listed */

    bool sawDir = false, sawCia = false, sawTxt = false;
    for (int i = 0; i < fileCount; i++) {
        if (fileEntries[i].isDir && strcmp(fileEntries[i].name, "SubFolder") == 0) sawDir = true;
        if (fileEntries[i].isCIA && strcmp(fileEntries[i].name, "Game.cia") == 0) {
            sawCia = true;
            ASSERT_EQ(fileEntries[i].titleID, (u64)0x0004000000001111ULL);
            ASSERT(fileEntries[i].size > 0);
        }
        if (strcmp(fileEntries[i].name, "notes.txt") == 0) sawTxt = true;
    }
    ASSERT_EQ(sawDir, true);
    ASSERT_EQ(sawCia, true);
    ASSERT_EQ(sawTxt, false); /* only .cia files and directories are listed */

    rmrf(INSTALL_TEST_ROOT);
    GREATEST_PASS();
}

TEST test_scanDirectory_adds_dotdot_when_not_root(void) {
    rmrf(INSTALL_TEST_ROOT);
    createDirectory(INSTALL_TEST_ROOT);

    scanDirectory(INSTALL_TEST_ROOT);
    ASSERT(fileCount >= 1);
    ASSERT_STR_EQ(fileEntries[0].name, "..");
    ASSERT_EQ(fileEntries[0].isDir, true);

    rmrf(INSTALL_TEST_ROOT);
    GREATEST_PASS();
}

TEST test_scanDirectory_root_has_no_dotdot(void) {
    int n = scanDirectory("sdmc:/");
    (void)n;
    if (fileCount > 0) ASSERT(strcmp(fileEntries[0].name, "..") != 0);
    GREATEST_PASS();
}

SUITE(suite_install) {
    RUN_TEST(test_getCIATitleID_parses_valid_header);
    RUN_TEST(test_getCIATitleID_missing_file_returns_zero);
    RUN_TEST(test_getCIATitleID_truncated_tmd_returns_zero);
    RUN_TEST(test_scanDirectory_classifies_dirs_and_cia_files);
    RUN_TEST(test_scanDirectory_adds_dotdot_when_not_root);
    RUN_TEST(test_scanDirectory_root_has_no_dotdot);
}

int main(void) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(suite_install);
    GREATEST_MAIN_END();
}
