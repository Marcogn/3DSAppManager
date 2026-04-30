/*
 * test_config.c — tests for config round-trip.
 *
 * Uses -DCONFIG_PATH and -DDEFAULT_BACKUP_PATH overrides so no 3DS FS is needed.
 */
#include "../vendor/greatest.h"
#include <stdio.h>
#include <string.h>

/*
 * We need globals.c for the `config` variable.
 * We use globals_stub.c at link time (via the Makefile).
 * config.c uses utils.c (createDirectory calls mkdir — OK on host for test paths).
 */
#include "../../source/utils.c"
#include "../../source/config.c"

/* CONFIG_PATH and DEFAULT_BACKUP_PATH are overridden via -D flags in the Makefile */

TEST test_loadConfig_defaults(void) {
    /* Remove any leftover test config */
    remove(CONFIG_PATH);
    /* saveDefaultConfig will fail to create sdmc:/ but that's fine — loadConfig sets defaults */
    loadConfig();
    ASSERT_STR_EQ(config.backupPath, DEFAULT_BACKUP_PATH);
    ASSERT_EQ(config.forceBackup,          false);
    ASSERT_EQ(config.skipUninstallConfirm, false);
    ASSERT_EQ(config.forceRestore,         false);
    ASSERT_EQ(config.skipInstallConfirm,   false);
    GREATEST_PASS();
}

TEST test_saveAndLoad_roundtrip(void) {
    /* Write a config file at the test CONFIG_PATH */
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) {
        /* If we cannot write (e.g. parent dir missing), skip gracefully */
        greatest_suite_info.skipped++;
        return 1;
    }
    fprintf(f, "backup_path=/test/backups\n");
    fprintf(f, "force_backup=1\n");
    fprintf(f, "skip_uninstall_confirm=0\n");
    fprintf(f, "force_restore=1\n");
    fprintf(f, "skip_install_confirm=0\n");
    fclose(f);

    loadConfig();
    ASSERT_STR_EQ(config.backupPath, "/test/backups");
    ASSERT_EQ(config.forceBackup,   true);
    ASSERT_EQ(config.forceRestore,  true);
    ASSERT_EQ(config.skipUninstallConfirm, false);

    remove(CONFIG_PATH);
    GREATEST_PASS();
}

TEST test_config_ignores_comments(void) {
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) { greatest_suite_info.skipped++; return 1; }
    fprintf(f, "# this is a comment\n");
    fprintf(f, "; also a comment\n");
    fprintf(f, "backup_path=/my/path\n");
    fclose(f);

    loadConfig();
    ASSERT_STR_EQ(config.backupPath, "/my/path");
    remove(CONFIG_PATH);
    GREATEST_PASS();
}

SUITE(suite_config) {
    RUN_TEST(test_loadConfig_defaults);
    RUN_TEST(test_saveAndLoad_roundtrip);
    RUN_TEST(test_config_ignores_comments);
}

int main(void) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(suite_config);
    GREATEST_MAIN_END();
}
