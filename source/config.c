/* ============================================================
   config.c — config load/save
   ============================================================ */
#include "config.h"
#include "globals.h"
#include "utils.h"
#include "lang.h"

const char *BACKUP_PATH_OPTIONS[] = {
    "sdmc:/3ds/3ds-app-manager/backups",
    "sdmc:/backups/3ds-titles",
    "sdmc:/save-backups",
    "sdmc:/3ds-backups",
    "sdmc:/backups"
};
const size_t NUM_BACKUP_PATHS = sizeof(BACKUP_PATH_OPTIONS)/sizeof(BACKUP_PATH_OPTIONS[0]);

void loadConfig(void) {
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", DEFAULT_BACKUP_PATH);
    config.forceBackup          = false;
    config.skipUninstallConfirm = false;
    config.forceRestore         = false;
    config.skipInstallConfirm   = false;
    /* Default for a fresh install and the fallback when an existing
       config.ini predates this setting (no language= line yet) — in
       both cases, follow the console's own system language rather than
       silently defaulting to English. */
    config.language             = detectSystemLanguage();
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) { saveDefaultConfig(); return; }
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line, *val = eq + 1;
        while (*key == ' ' || *key == '\t') key++;
        while (*val == ' ' || *val == '\t') val++;
        if (strcmp(key, "backup_path") == 0)
            snprintf(config.backupPath, sizeof(config.backupPath), "%s", val);
        else if (strcmp(key, "force_backup") == 0)
            config.forceBackup = (strcmp(val, "1") == 0 || strcmp(val, "true") == 0);
        else if (strcmp(key, "skip_uninstall_confirm") == 0)
            config.skipUninstallConfirm = (strcmp(val, "1") == 0 || strcmp(val, "true") == 0);
        else if (strcmp(key, "force_restore") == 0)
            config.forceRestore = (strcmp(val, "1") == 0 || strcmp(val, "true") == 0);
        else if (strcmp(key, "skip_install_confirm") == 0)
            config.skipInstallConfirm = (strcmp(val, "1") == 0 || strcmp(val, "true") == 0);
        else if (strcmp(key, "language") == 0)
            config.language = (strcmp(val, "it") == 0) ? LANG_IT : LANG_EN;
    }
    fclose(f);
}

void saveConfig(void) {
    createDirectory("sdmc:/3ds/3ds-app-manager");
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) return;
    fprintf(f, "# 3DS App Manager Configuration\n");
    fprintf(f, "backup_path=%s\n",           config.backupPath);
    fprintf(f, "force_backup=%d\n",          config.forceBackup ? 1 : 0);
    fprintf(f, "skip_uninstall_confirm=%d\n", config.skipUninstallConfirm ? 1 : 0);
    fprintf(f, "force_restore=%d\n",         config.forceRestore ? 1 : 0);
    fprintf(f, "skip_install_confirm=%d\n",  config.skipInstallConfirm ? 1 : 0);
    fprintf(f, "language=%s\n",              config.language == LANG_IT ? "it" : "en");
    fclose(f);
}

void saveDefaultConfig(void) {
    createDirectory("sdmc:/3ds/3ds-app-manager");
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) return;
    fprintf(f, "# 3DS App Manager Configuration\n");
    fprintf(f, "backup_path=%s\n",           DEFAULT_BACKUP_PATH);
    fprintf(f, "force_backup=0\n");
    fprintf(f, "skip_uninstall_confirm=0\n");
    fprintf(f, "force_restore=0\n");
    fprintf(f, "skip_install_confirm=0\n");
    fprintf(f, "language=%s\n",              config.language == LANG_IT ? "it" : "en");
    fclose(f);
}

