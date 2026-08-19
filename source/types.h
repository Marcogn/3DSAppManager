#pragma once
/* ============================================================
   types.h — shared type definitions, enums, structs, constants
   ============================================================ */
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <time.h>

/* ---- SMDH structure ---- */
typedef struct {
    u16 shortDescription[0x40];
    u16 longDescription[0x80];
    u16 publisher[0x40];
} SMDH_title;
typedef struct {
    u32 magic;
    u16 version;
    u16 reserved;
    SMDH_title titles[16];
    u8  ratings[16];
    u32 region;
    u32 matchMakerId;
    u64 matchMakerBitId;
    u32 flags;
    u16 eulaVersion;
    u16 reserved2;
    u32 optimalBannerFrame;
    u32 streetpassId;
    u64 reserved3;
    u8  smallIcon[0x480];
    u8  largeIcon[0x1200];
} SMDH;

/* ---- Constants ---- */
#define MAX_TITLES          500
#ifndef CONFIG_PATH
#define CONFIG_PATH         "sdmc:/3ds/fast-uninstall/config.ini"
#endif
#ifndef DEFAULT_BACKUP_PATH
#define DEFAULT_BACKUP_PATH "sdmc:/3ds/fast-uninstall/backups"
#endif
#define SMDH_LANG_COUNT     12   /* number of language slots in an SMDH title entry */
#define LANGUAGE_ENGLISH    1
#define MAX_FILE_SIZE       (100 * 1024 * 1024)
#define MAX_VISIBLE_TITLES  13
#define FILE_BROWSER_VISIBLE 12
#define BACKUP_VISIBLE      13
#define UNINSTALL_VISIBLE   12
#define DIR_STACK_MAX       12
#define MAX_FILES           256
#define CHUNK_SIZE          0x10000
#define ALIGN64(x)          (((u32)(x) + 63) & ~63U)
#define VERSION_STRING      "v2.3.1"

/* ---- Color palette ---- */
#define CLR_BG        C2D_Color32(20,20,30,255)
#define CLR_HEADER    C2D_Color32(40,60,100,255)
#define CLR_SELECTED  C2D_Color32(60,100,180,255)
#define CLR_WHITE     C2D_Color32(255,255,255,255)
#define CLR_GRAY      C2D_Color32(160,160,160,255)
#define CLR_RED       C2D_Color32(220,60,60,255)
#define CLR_GREEN     C2D_Color32(60,200,60,255)
#define CLR_YELLOW    C2D_Color32(255,220,60,255)
#define CLR_CYAN      C2D_Color32(100,220,255,255)
#define CLR_BACKUP_OK C2D_Color32(255,200,50,255)

/* ---- Enums ---- */
typedef enum {
    APP_MAIN_MENU,
    APP_INSTALL,
    APP_BACKUP,
    APP_UNINSTALL,
    APP_SYSINFO,
    APP_SETTINGS
} AppState;

typedef enum {
    SORT_BY_NAME,
    SORT_BY_SIZE,
    SORT_BY_TITLEID
} SortMode;

typedef enum {
    FILTER_ALL,
    FILTER_UPDATES,
    FILTER_DLC
} FilterMode;

typedef enum {
    SYSINFO_OVERVIEW,
    SYSINFO_GAMES,
    SYSINFO_UPDATES,
    SYSINFO_DLC
} SysInfoMode;

/* ---- Language (UI string translation — see lang.h/lang.c) ---- */
typedef enum {
    LANG_EN,
    LANG_IT,
    LANG_COUNT
} Language;

/* ---- Structs ---- */
typedef struct {
    u64          titleID;
    char         name[256];
    char         fullName[256];
    FS_MediaType mediaType;
    bool         selected;
    bool         isValid;
    u16          version;
    bool         hasBackup;
    C2D_Image    icon;
    bool         iconLoaded;
    u64          size;
} TitleInfo;

typedef struct {
    char backupPath[256];
    bool forceBackup;
    bool skipUninstallConfirm;
    bool forceRestore;
    bool skipInstallConfirm;
    Language language;
} Config;

typedef struct {
    int idx;
    int cursor;
} SysInfoDetailState;

typedef struct {
    char name[256];
    bool isDir;
    bool isCIA;
    u64  size;
    u64  titleID;
} FileEntry;

/* ---- Backup path presets (defined in config.c) ---- */
extern const char *BACKUP_PATH_OPTIONS[];
extern const size_t NUM_BACKUP_PATHS;

