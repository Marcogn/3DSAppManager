# IMPLEMENTATION_GUIDE.md
# Guida all'Implementazione — 3DS Fast Uninstall v2.0
# Documento per Agent di Implementazione

## Contesto

Il file `source/main.c` contiene tutto il backend dell'applicazione homebrew 3DS
(831 righe). Le sezioni da aggiungere in coda sono:
- **Section 9**: Funzioni draw (UI rendering)
- **Section 10**: Handler di input e flussi
- **Section 11**: `main()`

**Non modificare** le righe 1-831 esistenti. Aggiungere tutto in coda.

---

## Stato Attuale del Codice

### Già implementato (righe 1-831)

#### Defines critici
- `MAX_TITLES 500`, `MAX_VISIBLE_TITLES 13`, `MAX_FILES 256`
- `CONFIG_PATH "sdmc:/3ds/fast-uninstall/config.ini"`
- `DEFAULT_BACKUP_PATH "sdmc:/3ds/fast-uninstall/backups"`
- `CHUNK_SIZE 0x10000` (per installazione CIA in blocchi)
- `BACKUP_PATH_OPTIONS[]` — array di 5 percorsi alternativi backup

#### Enum/Struct
```c
typedef enum { APP_MAIN_MENU, APP_INSTALL, APP_BACKUP,
               APP_UNINSTALL, APP_SYSINFO, APP_SETTINGS } AppState;
typedef enum { SORT_BY_NAME, SORT_BY_SIZE, SORT_BY_TITLEID } SortMode;
typedef enum { FILTER_ALL, FILTER_UPDATES, FILTER_DLC } FilterMode;
typedef enum { SYSINFO_OVERVIEW, SYSINFO_GAMES,
               SYSINFO_UPDATES, SYSINFO_DLC } SysInfoMode;

typedef struct { u64 titleID; char name[256]; char fullName[256];
    FS_MediaType mediaType; bool selected; bool isValid;
    u16 version; bool hasBackup; C2D_Image icon; bool iconLoaded;
    u64 size; } TitleInfo;

typedef struct { char backupPath[256]; bool forceBackup;
    bool skipUninstallConfirm; bool forceRestore;
    bool skipInstallConfirm; int installDest; } Config;

typedef struct { char name[256]; bool isDir; bool isCIA; u64 size; } FileEntry;

