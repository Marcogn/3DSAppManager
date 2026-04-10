# Technical Documentation

Developer-level documentation for `3ds-fast-uninstall`.

---

## Architecture

The entire application lives in a single C file: `source/main.c` (~2500 lines).  
This is intentional — it simplifies cross-section state sharing (global variables) and avoids link-time complexity in the devkitPro toolchain.

### App States

```c
typedef enum {
    APP_MAIN_MENU,
    APP_INSTALL,
    APP_BACKUP,
    APP_UNINSTALL,
    APP_SYSINFO,
    APP_SETTINGS
} AppState;
```

The main loop dispatches draw calls and input handlers based on `appState`. Blocking flows (uninstall dialog chain, install progress, backup progress) are run as sub-loops **before** `C3D_FrameBegin` to avoid nested frame management.

---

## Core Data Structures

```c
typedef struct {
    u64            titleID;      // Unique title identifier
    char           name[256];    // Display name (truncated for list view)
    char           fullName[256];// Full name (used in detail views)
    FS_MediaType   mediaType;    // MEDIATYPE_SD or MEDIATYPE_NAND
    bool           selected;     // Selection state (uninstall / backup)
    bool           isValid;      // False after successful deletion
    u16            version;      // Title version
    bool           hasBackup;    // True if backup folder exists for this TitleID
    C2D_Image      icon;         // (reserved, not currently loaded)
    bool           iconLoaded;
    u64            size;         // Size in bytes
} TitleInfo;

typedef struct {
    char backupPath[256];
    bool forceBackup;
    bool skipUninstallConfirm;
    bool forceRestore;
    bool skipInstallConfirm;
} Config;

typedef struct {
    char name[256];
    bool isDir;
    bool isCIA;
    u64  size;
    u64  titleID; // Cached at scan time; 0 for dirs or unreadable CIAs
} FileEntry;
```

---

## Key Design Decisions

**Why a single source file?**  
All screens share global state (`titles[]`, `config`, cursors, scroll offsets).  
Keeping everything in one file removes the need for headers and avoids extern sprawl in a project of this scale.

**Why citro2d?**  
Hardware-accelerated rendering eliminates the flickering present in console-based approaches. citro2d provides a clean 2D sprite/text API on top of citro3d's PICA200 pipeline.

**Why 500 title limit?**  
The HOME menu displays at most 300 titles. The app raises the limit to 500 to handle edge cases and let users see the full picture before deciding what to delete. Memory footprint: ~120 KB for `titles[500]`.

**Why cache `titleID` in `FileEntry`?**  
Reading the TitleID from a CIA header requires opening the file. Caching it at `scanDirectory` time (one-time cost) means the render loop never performs file I/O.

---

## Rendering Pipeline

Every frame follows this pattern:

```
C3D_FrameBegin(C3D_FRAME_SYNCDRAW)
    C2D_SceneBegin(top)    → draw top screen
    C2D_SceneBegin(bottom) → draw bottom screen
C3D_FrameEnd(0)
```

`C2D_TextBufClear(dynamicBuf)` is called once at the start of each full draw function. **`C2D_SceneBegin` must be called exactly once per target per frame** — double calls cause flickering.

### SELECT Overlay (Flicker-Free)

When SELECT is held, the main loop takes a dedicated rendering path that bakes the overlay directly into the same frame:

1. Draw the normal UI (list + header) on the top screen.
2. Draw the semi-transparent overlay rectangle and control text on top (higher z-depth: 0.6–0.7).
3. Draw the bottom-screen overlay in the same frame.

This avoids a second `C2D_SceneBegin` on the same target.

### Bottom Screen Modes

| Mode | Trigger | Draw function |
|---|---|---|
| Normal | Default | `drawTouchControls()` |
| Dialog (selected list) | During uninstall dialogs | `drawSelectedTitlesList()` |
| Overlay | SELECT held | Inline overlay rendering |

---

## Title Loading (`loadTitles`)

1. `AM_GetTitleCount` for SD + NAND media types.
2. `AM_GetTitleList` fills a `u64` array of title IDs.
3. For each ID, safety filter: skip ranges `0x00040010`, `0x00040030`, `0x00040138`.
4. `AM_GetTitleInfo` → size, version, media type.
5. `AM_GetTitleInfo` → SMDH icon handle → UTF-16 short description extracted.
6. UTF-16 → UTF-8 conversion with language fallback (see below).
7. Clamp to `MAX_TITLES` (500); call `sortTitles()`.

Progress bar is drawn during loading via a blocking loop.

### Language Fallback Chain

```
1. System language (cfguGetSystemLanguage)
2. English (index 1)
3. Japanese (index 0)
4. All 12 SMDH language slots
5. Fallback: "Title [TitleID]"
```

Names consisting only of spaces or `--` are rejected and the next fallback is tried.

---

## Save Backup (`backupSaveDataToPath`)

Three `FS_Archive` types are attempted for each title:

| Archive type | Folder | Data |
|---|---|---|
| `ARCHIVE_USER_SAVEDATA` | `savedata/` | Main game saves |
| `ARCHIVE_EXTDATA` | `extdata/` | Extended / DLC data |
| `ARCHIVE_BOSS_EXTDATA` | `boss_extdata/` | SpotPass / StreetPass |

Each archive is recursively copied file-by-file using `CHUNK_SIZE` (64 KB) buffers.  
A `backup_info.txt` is written with title name, ID, version and timestamp.

---

## Save Restore (`restoreSaveDataFromPath`)

Reverse of backup: copies files from `backup/[TitleID]/savedata|extdata|boss_extdata/` back into the corresponding `FS_Archive`. The title must already be installed.

---

## Title Deletion (`deleteTitleCompletely` + `deleteTitle`)

```
1. AM_DeleteTitle (removes title executable + content)
2. Delete ARCHIVE_EXTDATA
3. Delete ARCHIVE_BOSS_EXTDATA
4. Verify: AM_GetTitleInfo → if it returns an error, deletion confirmed
5. Mark TitleInfo.isValid = false (removes from filtered list)
```

Related titles (DLC, Updates) are found via `findRelatedTitles()`, which matches the lower 32 bits of the Title ID against other entries in `titles[]`.

---

## CIA Installation (`installCIA`)

```
1. Open CIA file via FS (fsOpenFileDirectly)
2. AM_StartCiaInstall (targets MEDIATYPE_SD)
3. Stream file in CHUNK_SIZE (64 KB) blocks via AM_InstallNativeFirm-compatible write
4. AM_FinishCiaInstall
5. If forceRestore: call restoreSaveDataFromPath for the new title
6. Set titlesNeedRefresh = true (triggers loadTitles on next section entry)
```

---

## Settings Persistence

Config is stored at `sdmc:/3ds/fast-uninstall/config.ini` as a plain `key=value` file.  
`saveConfig()` is called immediately on every settings change (real-time save).  
`loadConfig()` is called once at startup; missing keys fall back to compiled-in defaults.

---

## Sort and Filter

```c
static int filteredIndices[MAX_TITLES]; // indices into titles[]
static int filteredCount;
```

`updateFilteredList()` rebuilds `filteredIndices` by:
1. Iterating `titles[]` and selecting only `isValid == true` entries.
2. Applying `currentFilterMode` (ALL / FILTER_UPDATES / FILTER_DLC).
3. The cursor is clamped to the new count.

`sortTitles()` sorts the full `titles[]` array in-place using `qsort` with one of three comparators (name, size, titleID).

---

## UI Layout Constants

| Element | x | y | scale |
|---|---|---|---|
| Header bar | 0 | 0 | — |
| Header text | 4–8 | 4 | 0.54f |
| Info bar (Uninstall) | 4 | 24 | 0.44f |
| List rows (most screens) | varies | 38 + i×14.5 | 0.38f |
| List rows (Install) | varies | 40 + i×14.5 | 0.38f |
| Type badge `^`/`+` (Uninstall/Backup) | 220 | row y | 0.44f |
| Type badge `^`/`+` (Install) | 316 | row y | 0.44f |
| TitleID right-aligned (Uninstall) | 396 − 16×6.0 = 300 | row y | 0.40f |
| Size right-aligned (Install) | 396 − len×5.7 | row y | 0.38f |
| Size right-aligned (SysInfo) | 396 − len×5.4 | row y | 0.36f |
| Bottom hint bar | 0 | 222 | — |
| Bottom hint text | 4 | 224 | 0.52f |

---

## Sleep Mode

`aptMainLoop()` returns `false` during sleep, which pauses the main loop.  
Rendering must continue every frame when `aptMainLoop()` is `true` to keep the display driver in sync. No aggressive frame-skipping is permitted.

---

## Memory Budget

| Allocation | Size |
|---|---|
| `titles[500]` | ~120 KB |
| `filteredIndices[500]` | ~2 KB |
| `fileEntries[256]` | ~70 KB |
| `dynamicBuf` (text buffer) | 4 KB |
| `sysInfoSubIndices[500]` | ~2 KB |
| **Total approx.** | **~200 KB** |

The 3DS homebrew heap is typically 32–64 MB — this app is well within budget.

---

## Known Limitations

- Icon loading (`C2D_Image icon`) is allocated in `TitleInfo` but not populated at runtime (complexity vs. benefit tradeoff).
- Max file size per backup copy: `MAX_FILE_SIZE` = 100 MB.
- Title name truncation: 28 chars in list views (Uninstall, Backup), 32 chars in SysInfo sublist. Full name shown in all detail/bottom panels.

---

For user-facing documentation, see the main [README.md](../README.md) and [USER_GUIDE.md](../USER_GUIDE.md).
