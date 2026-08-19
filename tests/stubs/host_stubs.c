/*
 * stubs/host_stubs.c — stub implementations for 3DS API functions used by titles.c.
 * These provide link-time symbols so the test binary links without libctru.
 */
#include "../../source/types.h"
#include "../../source/draw.h"

/* draw.c forward decl used by titles.c */
void drawLoadingScreen(int current, int total, const char *status) {
    (void)current; (void)total; (void)status;
}

/* draw.c forward decl used by backup_restore.c (deleteTitle) */
void drawDialog(const char **lines, int lineCount) {
    (void)lines; (void)lineCount;
}

/* draw.c decl used by install.c (installCIA) */
void drawInstallProgressScreen(const char *ciaPath, long done, long total) {
    (void)ciaPath; (void)done; (void)total;
}

/* config.c stubs — BACKUP_PATH_OPTIONS is already defined in config.c, included in tests */
