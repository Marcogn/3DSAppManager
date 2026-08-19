#pragma once
/* ============================================================
   lang.h — UI string translation (English / Italian)
   ============================================================
   T(id) returns the current-language string for a StringID. Only
   natural-language UI text goes through this table — technical/numeric
   formats (hex title IDs, "v%u", "SD"/"NAND", checkbox glyphs, sort
   symbols "^"/"+") are identical in both languages and stay as literals
   at the call site. Format strings keep their printf specifiers; only
   the surrounding words are translated. */
#include "types.h"

typedef enum {
    /* ---- Common (shared across screens) ---- */
    STR_FOOTER_BACK_EXIT_HELP,      /* "B: Back  START: Exit  SELECT: Help" */
    STR_SELECTED_FMT,               /* "Selected (%d)" */
    STR_AND_N_MORE_FMT,             /* "...and %d more" */
    STR_NO_BACKUP,                  /* "No backup" */
    STR_BACKUP_FMT,                 /* "Backup: %s" */
    STR_UNKNOWN,                    /* "Unknown" */
    STR_TYPE_UPDATE,                /* "Update" */
    STR_TYPE_DLC,                   /* "DLC" */
    STR_TYPE_GAME,                  /* "Game" */
    STR_SORT_NAME,                  /* "Name" */
    STR_SORT_SIZE,                  /* "Size" */
    STR_SORT_ID,                    /* "ID" */
    STR_TITLE_DETAILS,              /* "Title Details" */
    STR_RELEASE_SELECT_RETURN,      /* "Release SELECT to return." */
    STR_A_CONTINUE,                 /* "  A=Continue" */
    STR_A_OK,                       /* "  A=OK" */
    STR_A_YES_B_NO,                 /* "  A=Yes   B=No" */
    STR_LOADING,                    /* "Loading..." */
    STR_LOADING_TITLES,             /* "Loading titles..." */
    STR_DO_NOT_POWER_OFF,           /* "Do not power off the console." */

    /* ---- Main Menu ---- */
    STR_APP_TITLE_FMT,              /* "3DS App Manager  %s" */
    STR_MENU_INSTALL,                /* "Install CIA" */
    STR_MENU_BACKUP,                 /* "Backup Saves" */
    STR_MENU_UNINSTALL,              /* "Uninstall Titles" */
    STR_MENU_SYSINFO,                /* "System Info" */
    STR_MENU_SETTINGS,               /* "Settings" */
    STR_MENU_FOOTER_TOP,             /* "START: Exit  SELECT: Help" */
    STR_MENU_FOOTER_BOTTOM,          /* "A: Select  START: Exit  SEL: Help" */
    STR_MENU_DESCRIPTION,             /* "Description" */
    STR_MENU_DESC_INSTALL_1,
    STR_MENU_DESC_INSTALL_2,
    STR_MENU_DESC_BACKUP_1,
    STR_MENU_DESC_BACKUP_2,
    STR_MENU_DESC_UNINSTALL_1,
    STR_MENU_DESC_UNINSTALL_2,
    STR_MENU_DESC_SYSINFO_1,
    STR_MENU_DESC_SYSINFO_2,
    STR_MENU_DESC_SETTINGS_1,

    /* ---- Uninstall screen (drawUI / drawTouchControls) ---- */
    STR_UNINSTALL_HEADER,            /* "Uninstall" */
    STR_UNINSTALL_INFO_FMT,          /* "T:%-3d Sel:%-3d Sort:" */
    STR_FILTER_ALL,                  /* "All" */
    STR_FILTER_UPD,                  /* "Upd" */
    STR_FILTER_DLC,                  /* "DLC" */
    STR_NO_TITLES,                   /* "No titles to show." */

    /* ---- Install / file browser ---- */
    STR_INSTALL_HEADER,              /* "Install CIA" */
    STR_DIR_PREFIX_FMT,              /* "[DIR] %s/" */
    STR_FILE_INFO,                   /* "File Info" */
    STR_FILE_ID_FMT,                 /* "ID: %016llX" */
    STR_FILE_ID_NA,                  /* "ID: N/A" */
    STR_FILE_SIZE_FMT,               /* "Size: %s" */
    STR_GO_TO_PARENT,                /* "Go to parent folder" */
    STR_FOLDER_FMT,                  /* "Folder: %.30s" */
    STR_EMPTY_FOLDER,                /* "Empty folder" */
    STR_TYPE_UPDATE_BADGE,           /* "Type: Update  [^]" */
    STR_TYPE_DLC_BADGE,              /* "Type: DLC     [+]" */
    STR_TYPE_GAME_LABEL,             /* "Type: Game" */

    /* ---- Backup screen ---- */
    STR_BACKUP_HEADER_FMT,           /* "Save Backups   Sel:%d" */
    STR_SORT_FMT,                    /* "Sort:%s" */

    /* ---- System Info screen ---- */
    STR_SYSINFO_HEADER,              /* "SYSTEM INFORMATION" */
    STR_SYSINFO_ROW_GAMES,           /* "Games:" (padded at call site) */
    STR_SYSINFO_ROW_UPDATES,         /* "Updates:" */
    STR_SYSINFO_ROW_DLC,             /* "DLC:" */
    STR_SYSINFO_SD_FREE_FMT,         /* "  SD Free: %s / %s" */
    STR_SYSINFO_OVERVIEW,            /* "Overview" */
    STR_SYSINFO_HELP_1,              /* "A: Open category list." */
    STR_SYSINFO_HELP_2,              /* "UP/DOWN: Select category." */
    STR_SYSINFO_HELP_3,              /* "B: Back to main menu." */
    STR_SYSINFO_GAMES_HDR,           /* "GAMES" */
    STR_SYSINFO_UPDATES_HDR,         /* "UPDATES" */
    STR_SYSINFO_DLC_HDR,             /* "DLC" */
    STR_SYSINFO_HDR_COUNT_FMT,       /* "%s (%d)" */
    STR_SYSINFO_A_DETAILS,           /* "A: Details & actions" */

    /* ---- Settings screen ---- */
    STR_SETTINGS_HEADER,             /* "SETTINGS" */
    STR_SETTINGS_FORCE_BACKUP,       /* "Force Backup:" */
    STR_SETTINGS_SKIP_UNINSTALL,     /* "Skip Uninstall Confirm:" */
    STR_SETTINGS_FORCE_RESTORE,      /* "Force Restore:" */
    STR_SETTINGS_SKIP_INSTALL,       /* "Skip Install Confirm:" */
    STR_SETTINGS_BACKUP_FOLDER,      /* "Backup Folder:" */
    STR_SETTINGS_LANGUAGE,           /* "Language:" */
    STR_SETTINGS_ON,                 /* "ON" */
    STR_SETTINGS_OFF,                /* "OFF" */
    STR_SETTINGS_SEE_BOTTOM,         /* "<see bottom>" */
    STR_SETTINGS_DESC_HEADER,        /* "Description" */
    STR_SETTINGS_DESC_FORCEBK_1,
    STR_SETTINGS_DESC_FORCEBK_2,
    STR_SETTINGS_DESC_FORCEBK_3,
    STR_SETTINGS_DESC_SKIPUNINST_1,
    STR_SETTINGS_DESC_SKIPUNINST_2,
    STR_SETTINGS_DESC_SKIPUNINST_3,
    STR_SETTINGS_DESC_FORCERESTORE_1,
    STR_SETTINGS_DESC_FORCERESTORE_2,
    STR_SETTINGS_DESC_FORCERESTORE_3,
    STR_SETTINGS_DESC_SKIPINSTALL_1,
    STR_SETTINGS_DESC_SKIPINSTALL_2,
    STR_SETTINGS_DESC_SKIPINSTALL_3,
    STR_SETTINGS_DESC_BACKUPFOLDER_1,
    STR_SETTINGS_DESC_BACKUPFOLDER_2,
    STR_SETTINGS_DESC_BACKUPFOLDER_3,
    STR_SETTINGS_DESC_LANGUAGE_1,
    STR_SETTINGS_DESC_LANGUAGE_2,
    STR_SETTINGS_DESC_LANGUAGE_3,
    STR_SETTINGS_CURRENT_PATH,       /* "Current path:" */
    STR_SETTINGS_SAVED_REALTIME,     /* "Changes saved in real time." */

    /* ---- Title Details (SysInfo detail flow) ---- */
    STR_DETAIL_RELATED,              /* "Related:" */
    STR_DETAIL_RELATED_UPD_FMT,      /* "  [Upd] %.32s" */
    STR_DETAIL_RELATED_DLC_FMT,      /* "  [DLC] %.32s" */
    STR_DETAIL_ACTION_BACKUP,        /* "[A] Backup Save Data" */
    STR_DETAIL_ACTION_RESTORE,       /* "[Y] Restore Save Data" */
    STR_DETAIL_ACTION_DELETE,        /* "[X] Delete Title (+ related)" */
    STR_DETAIL_FOOTER_NAV,           /* "Up/Down: Nav  A: Execute  B: Back" */
    STR_DETAIL_ACTION_HEADER,        /* "Action" */
    STR_DETAIL_DESC_BACKUP_1,
    STR_DETAIL_DESC_BACKUP_2,
    STR_DETAIL_DESC_RESTORE_1,
    STR_DETAIL_DESC_RESTORE_2,
    STR_DETAIL_DESC_DELETE_1,
    STR_DETAIL_DESC_DELETE_2,
    STR_DETAIL_CONFIRM_ACTION,       /* "A: confirm action" */

    /* ---- Help overlay ---- */
    STR_HELP_TITLE_MAINMENU,
    STR_HELP_TITLE_INSTALL,
    STR_HELP_TITLE_BACKUP,
    STR_HELP_TITLE_UNINSTALL,
    STR_HELP_TITLE_SYSINFO,
    STR_HELP_TITLE_SETTINGS,
    STR_HELP_KEY_UPDOWN,             /* "Up/Down" */
    STR_HELP_KEY_LEFTRIGHT,          /* "Left/Right" */
    STR_HELP_KEY_A_LR_LEFTRIGHT,     /* "A/Left/Right/L/R" */
    STR_HELP_KEY_B_START,            /* "B/START" */
    STR_HELP_DESC_NAV_MENU,          /* "Navigate menu" */
    STR_HELP_DESC_ENTER_SCREEN,      /* "Enter screen" */
    STR_HELP_DESC_EXIT_APP,          /* "Exit app" */
    STR_HELP_DESC_ENTER_INSTALL,     /* "Enter folder / Install CIA" */
    STR_HELP_DESC_INSTALL_ALL,       /* "Install all in folder" */
    STR_HELP_DESC_GO_UP,             /* "Go up one level" */
    STR_HELP_DESC_PAGE_JUMP,         /* "Page jump" */
    STR_HELP_DESC_BACK_MENU,         /* "Back to menu" */
    STR_HELP_DESC_SELECT_TITLE,      /* "Select title" */
    STR_HELP_DESC_BACKUP_SELECTED,   /* "Backup selected" */
    STR_HELP_DESC_BACKUP_ALL,        /* "Backup all titles" */
    STR_HELP_DESC_SORT,              /* "Sort" */
    STR_HELP_DESC_TOGGLE_SELECTION,  /* "Toggle selection" */
    STR_HELP_DESC_START_UNINSTALL,   /* "Start uninstall flow" */
    STR_HELP_DESC_CYCLE_SORT,        /* "Cycle sort" */
    STR_HELP_DESC_CYCLE_FILTER,      /* "Cycle filter" */
    STR_HELP_DESC_NAVIGATE,          /* "Navigate" */
    STR_HELP_DESC_OPEN_CATEGORY,     /* "Open category / details" */
    STR_HELP_DESC_SORT_CATEGORY,     /* "Sort (in category)" */
    STR_HELP_DESC_BACK,              /* "Back" */
    STR_HELP_DESC_CHANGE_VALUE,      /* "Change value" */
    STR_HELP_DESC_SAVE_BACK,         /* "Save and back" */
    STR_HELP_HINT_MAINMENU,          /* "A: Confirm selection" */
    STR_HELP_HINT_INSTALL,           /* "Title details shown on bottom screen" */
    STR_HELP_HINT_BACKUP,            /* "Title details + backup status below" */
    STR_HELP_HINT_UNINSTALL,         /* "Selected titles shown on bottom screen" */
    STR_HELP_HINT_SYSINFO,           /* "Title details and actions on bottom screen" */
    STR_HELP_HINT_SETTINGS,          /* "Setting descriptions shown on bottom screen" */

    /* ---- input.c dialogs: install flow ---- */
    STR_DLG_NO_CIA_IN_FOLDER,        /* "  No CIA files in this folder." */
    STR_DLG_INSTALL_N_CIA_FMT,       /* "  Install %d CIA files?" */
    STR_DLG_DEST_SD,                 /* "  Destination: SD card" */
    STR_DLG_INSTALL_QUESTION,        /* "  Install?" */
    STR_DLG_ID_FMT,                  /* "  ID: %016llX" */
    STR_DLG_ID_NA,                   /* "  ID: N/A" */
    STR_DLG_INSTALLATION_COMPLETE,   /* "  Installation complete!" */
    STR_DLG_ERROR_INSTALLING,        /* "  Error during installation." */
    STR_DLG_CHECK_CIA_VALID,         /* "  Check that the CIA file is valid." */
    STR_DLG_SAVE_RESTORED_AUTO_1,    /* "  Save data restored" */
    STR_DLG_SAVE_RESTORED_AUTO_2,    /* "  automatically." */
    STR_DLG_SAVE_RESTORED,           /* "  Save data restored." */
    STR_DLG_FOUND_BACKUP_TITLE,      /* "  Found backup for this title." */
    STR_DLG_RESTORE_QUESTION,        /* "  Restore save data?" */
    STR_DLG_BATCH_INSTALL_DONE,      /* "  Batch install completed." */
    STR_DLG_INSTALLED_FAILED_FMT,    /* "  Installed: %d   Failed: %d" */

    /* ---- input.c dialogs: backup flow ---- */
    STR_DLG_NO_TITLE_SELECTED_SHORT, /* "  No title selected." */
    STR_DLG_SELECT_WITH_A,           /* "  Select titles with A." */
    STR_DLG_PRESS_A_CONTINUE,        /* "  Press A to continue." */
    STR_DLG_BACKUP_N_SELECTED_FMT,   /* "  Backup %d selected titles?" */
    STR_DLG_BACKUP_ALL_N_FMT,        /* "  Backup ALL titles (%d)?" */
    STR_DLG_MAY_TAKE_A_WHILE,        /* "  This may take a while." */
    STR_DLG_BACKUP_COMPLETED_FMT,    /* "  Backup completed: %d ok, %d failed." */

    /* ---- input.c dialogs: uninstall flow ---- */
    STR_DLG_NO_TITLE_SELECTED_LONG,  /* "  No title selected." */
    STR_DLG_SELECT_AT_LEAST_ONE,     /* "  Select at least one title with A." */
    STR_DLG_FOUND_RELATED_FMT,       /* "  Found %d related titles (DLC/Update)." */
    STR_DLG_INCLUDE_SKIP_CANCEL,     /* "  A=Include  B=Skip  START=Cancel" */
    STR_DLG_BACKUP_SAVE_DATA_Q,      /* "  Backup save data?" */
    STR_DLG_YES_NO_CANCEL,           /* "  A=Yes   B=No   START=Cancel" */
    STR_DLG_USE_DEFAULT_CHOOSE,      /* "  A=Use default   Y=Choose other" */
    STR_DLG_SELECT_BACKUP_FOLDER,    /* "  Select backup folder:" */
    STR_DLG_UPDOWN_CONFIRM_CANCEL,   /* "  Up/Down=Change   A=Confirm   B=Cancel" */
    STR_DLG_DELETE_N_TITLES_FMT,     /* "  Delete %d titles?" */
    STR_DLG_BACKUP_TO_FMT,           /* "  Backup to: %.36s" */
    STR_DLG_NO_BACKUP_PERIOD,        /* "  No backup." */
    STR_DLG_CONFIRM_CANCEL,          /* "  A=Confirm   B=Cancel" */
    STR_DLG_N_BACKUPS_FAILED_FMT,    /* "  %d backup(s) failed." */
    STR_DLG_N_TITLES_NOT_DELETED_FMT,/* "  %d title(s) NOT deleted." */
    STR_DLG_SUCCESSFULLY_DELETED,    /* " Successfully deleted:" */
    STR_DLG_ALL_DATA_REMOVED,        /* "All data removed." */
    STR_DLG_PRESS_A_CONTINUE_CAP,    /* "Press A to continue..." */
    STR_DLG_FAILED_TO_DELETE,        /* " Failed to delete:" */
    STR_DLG_TITLE_MAY_REMAIN,        /* "Title may still be present." */

    /* ---- input.c dialogs: SysInfo detail flow ---- */
    STR_DLG_BACKUP_COMPLETED_SIMPLE, /* "  Backup completed." */
    STR_DLG_BACKUP_FAILED_SIMPLE,    /* "  Backup failed." */
    STR_DLG_NO_BACKUP_AVAILABLE,     /* "  No backup available." */
    STR_DLG_RESTORE_ERROR,           /* "  Restore error." */

    STR_COUNT
} StringID;

const char *T(StringID id);
Language detectSystemLanguage(void);
