<p align="center">
  <img src="icon.png" alt="3DS Fast Uninstall Icon" width="200"/>
</p>

# 3DS Fast Uninstall

[![Latest release](https://img.shields.io/github/v/release/Marcogn/3ds-fast-uninstall?label=release)](https://github.com/Marcogn/3ds-fast-uninstall/releases/latest)
[![Build](https://github.com/Marcogn/3ds-fast-uninstall/actions/workflows/build.yml/badge.svg)](https://github.com/Marcogn/3ds-fast-uninstall/actions/workflows/build.yml)
[![Host-side Unit Tests](https://github.com/Marcogn/3ds-fast-uninstall/actions/workflows/tests.yml/badge.svg)](https://github.com/Marcogn/3ds-fast-uninstall/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A Nintendo 3DS homebrew application for managing installed titles: install
CIA files, back up save data, uninstall titles in batch, and view system
information — all from a single clean multi-screen interface, built for
consoles with a large, cluttered library.

Select as many titles as you want on one screen, get an automatic offer to
back up (and later restore) their save data, and let the app confirm each
deletion actually happened before moving on — instead of picking through
the HOME menu one title at a time.

## Why

The HOME menu only deletes one title at a time and has no built-in way to
back up a save before the title disappears with it. 3ds-fast-uninstall
turns that into a batch operation with the backup step made explicit and
automatic — optional per run, or forced on always from Settings — so
cleaning up storage never has to mean gambling with save data.

## Download

Grab the latest build from the
[**Releases**](https://github.com/Marcogn/3ds-fast-uninstall/releases/latest)
page.

1. Download `3ds-fast-uninstall.3dsx` from the latest release.
2. Copy it to `/3ds/` on your SD card.
3. Launch it from the **Homebrew Launcher**.
4. Wait for the loading screen (a progress bar shows while titles are read).
5. You land on the **Main Menu** — pick a feature and get started.

> **Requirements**: Nintendo 3DS / 2DS / New 3DS running Custom Firmware (e.g. Luma3DS) or a homebrew entrypoint with sufficient AM access.

---

## Features

### Install
Browse the SD card for `.CIA` files and install them one at a time or an
entire folder in a single pass.
- Browse the SD card file system to find `.CIA` files
- Install single files or batch-install an entire folder in one step (**Y**)
- Detects Update (`^`) and DLC (`+`) types at scan time — no extra file reads
- Size shown right-aligned per row; type badge close to size column
- All installs always target the SD card

### Backup Saves
Back up any title's save data on demand, independent of uninstalling it —
so a backup can be refreshed anytime, not just right before deletion.
- List all installed titles with backup status at a glance (`[ ]` none · `[*]` exists · `[X]` selected)
- Select individual titles or back up the entire library at once (**Y**)
- Three save archive types per title: User Save, ExtData, Boss ExtData
- Backup date shown in the bottom-screen detail panel
- Backups stored under `[backup_path]/[TitleID]/`

### Uninstall
The core flow: select as many titles as you want and delete them in one
batch, with an automatic offer to back up save data first.
- Multi-selection with **A**; batch uninstall with **X**
- Optional pre-uninstall backup (prompted automatically, or forced via Settings)
- Selected titles list visible on the bottom screen throughout the confirmation flow
- Automatically finds and offers to delete related DLC/Updates for base games
- Post-delete verification: slot is cleared only after AM confirms deletion

### System Info
A per-category browser for everything installed, with backup/restore/delete
actions available straight from a title's detail page.
- Overview: Games / Updates / DLC counts + total sizes + SD free space
- Navigate into each category (**A**) to browse the full title list
- Per-title detail page with full name, Title ID, version, size, storage location, backup date
- Actions per title: **Backup Save Data**, **Restore Save Data**, **Delete Title**

### Settings
Six toggles/options, all applied and saved as soon as you change them —
no separate save step.

| Setting | Default | Description |
|---|---|---|
| Force Backup | OFF | Auto-backup before every uninstall (no prompt) |
| Skip Uninstall Confirm | OFF | Delete immediately on X without confirmation |
| Force Restore | OFF | Auto-restore backup after every CIA install |
| Skip Install Confirm | OFF | Install immediately on A without confirmation |
| Backup Folder | `sdmc:/3ds/fast-uninstall/backups` | Cycle through 5 preset paths with left/right or L/R |
| Language | System language (EN/IT, else EN) | Cycle between English and Italian for all app UI text |

Changes are saved in real time; press **B** or **START** to return to the menu.

### General
Shared behavior across every screen:
- Sort by **Name**, **Size** (largest first), or **Title ID**
- Filter by **All**, **Updates only**, or **DLC only**
- Supports up to **500 titles** (HOME menu cap is 300; counter turns red above it)
- Multi-language title names: system language → English → Japanese → any available
- Hold **SELECT** for a full controls overlay (flicker-free rendering)
- Sleep mode compatible — close and re-open the console freely
- Progress bar during title loading

---

## How to Use

### Install CIA

1. From the Main Menu, select **Install CIA** and press **A**.
2. Navigate the file browser with the controls listed in the Controls Reference section.
3. Directories appear in cyan with a `[DIR]` prefix. `.CIA` files show size right-aligned and a type badge: `^` = Update, `+` = DLC.

**Installing a single file:**
1. Navigate to the `.CIA` file and press **A**.
2. Confirm the prompt (skipped if "Skip Install Confirm" is ON in Settings).
3. Wait for the progress bar to complete.

**Batch installing a folder:**
1. Enter the folder containing your `.CIA` files.
2. Press **Y**. The app installs all files one by one and shows progress for each.

> All installs target the **SD card**. NAND installs are not supported.

---

### Backup Saves

Each row in the list shows:
- `[ ]` — no backup exists
- `[*]` (green) — backup exists
- `[X]` (yellow) — title selected for batch backup

The bottom screen shows full name, Title ID, size, storage location, and last backup date for the cursor item.

**Single title:** navigate to it -> press **A** to select -> press **X** to back up.  
**Multiple titles:** press **A** on each -> press **X** to back them all up.  
**Entire library:** press **Y** — no selection needed.

For each title, the app saves up to three archive types:
- `savedata/` — main game saves
- `extdata/` — extended data (e.g. DLC content)
- `boss_extdata/` — SpotPass / StreetPass data

A `backup_info.txt` file with title name, version and date is included in each backup folder.

---

### Uninstall Titles

1. Use **up / down** to navigate (or **left / right** to jump a full page).
2. Press **A** to toggle selection (checkbox turns yellow when selected).
3. Use **L / R** to cycle sort modes (Name -> Size -> ID); **Y** to filter (All -> Updates -> DLC).
4. Press **X** to begin the uninstall flow.

**The uninstall flow:**

| Step | Prompt | Actions |
|---|---|---|
| 1 — Backup? | Back up before deleting? | **A** = yes · **B** = skip · **START** = cancel |
| 2 — Backup path | *(shown only if backup chosen)* | **A** = use default · **Y** = pick from preset list |
| 3 — Related titles | DLC/Updates found for base game | **A** = include · **B** = skip · **START** = cancel |
| 4 — Final confirm | Review list on bottom screen | **A** = confirm · **B** = cancel |
| 5 — Deletion | Progress shown per title | automatic |

> If "Force Backup" is ON, step 1 is skipped and backup always runs.  
> If "Skip Uninstall Confirm" is ON, step 4 is skipped.

---

### System Info

**Overview screen:** shows Games / Updates / DLC counts and sizes, plus SD free space.  
Press **up / down** to navigate categories; **A** to enter a category.

**Category list:** lists all titles with name and size. Press **A** on any title to open its **detail page**.

**Title detail page:**
- Full name, Title ID, version, size, storage location, backup date
- Related Updates and DLC (for base games)
- Three actions (navigate with **up / down**, execute with **A**).

| Action | What it does |
|---|---|
| **Backup Save Data** | Saves all archive types to the backup folder |
| **Restore Save Data** | Restores save data from the last backup |
| **Delete Title (+ related)** | Removes the title, its DLC, Updates and all save data |

**Restoring a save:** System Info -> find the title -> **A** -> "Restore Save Data" -> **A**.

---

### Settings

Navigate with **up / down**. Change a value with **A**, **L**, **R**, or the d-pad left/right.  
The bottom screen shows a description of the highlighted setting.

> Settings are saved in real time. Press **B** or **START** to return to the main menu.

---

## Controls Reference

### Main Menu

| Button | Action |
|---|---|
| **Up / Down** | Navigate menu |
| **A** | Enter selected screen |
| **START** | Exit application |
| **SELECT** (hold) | Show context-sensitive help overlay |

### Uninstall Screen

| Button | Action |
|---|---|
| **Up / Down** | Move cursor one row |
| **Left / Right** | Jump one page |
| **A** | Toggle title selection |
| **X** | Start uninstall flow for selected titles |
| **L / R** | Cycle sort mode (Name -> Size -> ID) |
| **Y** | Cycle filter (All -> Updates -> DLC) |
| **SELECT** (hold) | Show context-sensitive help overlay |
| **B** | Back to main menu |
| **START** | Exit application |

### Install Screen (File Browser)

| Button | Action |
|---|---|
| **Up / Down** | Move cursor |
| **Left / Right** | Jump one page |
| **A** | Enter folder / Install CIA file |
| **Y** | Install all CIA files in current folder |
| **B** | Go up one folder level |

### Backup Screen

| Button | Action |
|---|---|
| **Up / Down** | Move cursor one row |
| **Left / Right** | Jump one page |
| **A** | Toggle title selection |
| **X** | Backup selected titles |
| **Y** | Backup all titles |
| **L / R** | Cycle sort mode |
| **SELECT** (hold) | Show context-sensitive help overlay |
| **B** | Back to main menu |

### System Info Screen

| Button | Action |
|---|---|
| **Up / Down** | Navigate categories / title list |
| **A** | Open category list / open title detail |
| **L / R** | Cycle sort (in category list) |
| **SELECT** (hold) | Show context-sensitive help overlay |
| **B** | Back / back to overview |

### Title Detail (from System Info)

| Button | Action |
|---|---|
| **Up / Down** | Select action |
| **A** | Execute selected action |
| **B** | Back to category list |

### Settings Screen

| Button | Action |
|---|---|
| **Up / Down** | Navigate settings |
| **A / L / R / d-pad left-right** | Change value |
| **SELECT** (hold) | Show context-sensitive help overlay |
| **B / START** | Save and return to menu |

---

## Screen Layout Reference

### Top Screen — Uninstall

```
+-----------------------------------------------------+
| Uninstall                                           |  <- header
| T:120  Sel:3  Sort:Name  [All]                      |  <- info bar
| [ ] Super Mario 3D Land             0004000000...   |
| [X] Pokemon Sun              ^      0004000000...   |  <- selected, Update
| [ ] Mario Kart 7 DLC         +      0004008C00...   |  <- DLC
| A=Sel  X=Delete  L/R=Sort  Y=Filt  B=Menu  SEL=Help |
+-----------------------------------------------------+
```

Symbols: `^` = Update (cyan), `+` = DLC (green)  
Checkbox: `[ ]` unselected, `[X]` selected (yellow)

### Top Screen — Install (File Browser)

```
+-----------------------------------------------------+
| Install CIA                                         |  <- header
| sdmc:/roms/cias/                                    |  <- current path
| [DIR] demos/                                        |
| SuperMario.cia                        ^ [45.3 MB]   |
| DLC_Pack.cia                          + [12.1 MB]   |
| A=Enter/Install  Y=All  B=Up  DX/SX=Page            |
+-----------------------------------------------------+
```

### Top Screen — System Info (Overview)

```
+-----------------------------------------------------+
| SYSTEM INFORMATION                                  |
|                                                     |
|   Games:      87    4.23 GB                         |
| > Updates:    43    980 MB                          |  <- cursor row
|   DLC:        31    1.12 GB                         |
|                                                     |
|   SD Free: 12.4 GB / 59.6 GB                        |
+-----------------------------------------------------+
```

---

## Tips & Tricks

**Free up the most space**
1. Go to **Uninstall Titles**.
2. Press **R** twice to sort by Size (largest first).
3. Select the titles you no longer need and press **X**.

**Clean up only DLC and Updates**
1. Press **Y** once for Updates, twice for DLC.
2. Select what you want to remove and press **X**.

**Check what has a backup**  
Open **Backup Saves**. Titles with `[*]` in green already have a backup; `[ ]` means never backed up.

**Batch backup before a big cleanup**  
Go to **Backup Saves** -> press **Y** to back up everything, then proceed to **Uninstall Titles**.

**Restore a save after reinstalling a game**  
Reinstall the `.CIA` from **Install CIA**.
- If "Force Restore" is ON, the backup is restored automatically.
- Otherwise: **System Info** -> find the title -> **A** -> "Restore Save Data".

---

## FAQ & Troubleshooting

**No titles showing up**  
The app only shows user-installed titles. System titles are filtered for safety.

**The title counter is red**  
You have over 300 titles. The HOME menu cap is 300. The app still shows all of them (up to 500) — consider cleaning up.

**How do I restore a backup?**  
System Info -> navigate to the title -> **A** -> "Restore Save Data".

**Where are my backups?**  
`[backup_path]/[TitleID]/`. Default: `sdmc:/3ds/fast-uninstall/backups/`. Change in Settings.

**Is it safe to delete updates or DLC?**  
Yes. Each is an independent title. Deleting an update reverts the game to its base version.

**Can I use this without CFW?**  
A homebrew entrypoint is required. Full title deletion requires AM access (CFW recommended).

**Scroll feels slow**  
Single press = one row. Use **left / right** for page jumps. The deliberate speed prevents accidental skips.

**Cannot find a title**  
Try different sort modes (**L / R**) or use the filter (**Y**) to isolate Updates or DLC.

**Something deleted but still showing in HOME menu**  
Restart the HOME menu or the console. The 3DS system cache can take a moment to update.

| Problem | Solution |
|---|---|
| App will not load | Make sure you are on a recent Homebrew Launcher and have CFW access |
| Loading bar appears stuck | Not stuck — it reads each title individually. 200+ titles can take ~30 seconds |
| Backup failed | Check that the SD card has enough free space and the backup folder is writable |
| Install failed | Verify the `.CIA` file is not corrupted; check free SD space |
| Restore failed | The backup folder may be missing or the title must be installed first |

---

## Safety

- **System titles filtered** — ranges `0x00040010`, `0x00040030`, `0x00040138` are excluded
- **Confirmation dialogs** before every destructive action (unless disabled in Settings)
- **Post-delete verification** via `AM_GetTitleInfo` — title removed from list only after system confirms deletion
- **Backups are independent** — deleting a title does not delete its backup

---

## Backup Structure

```
[backup_path]/
|__ [TitleID]/            e.g. 0004000000033500/
    |__ backup_info.txt   metadata (date, version, name)
    |__ savedata/         main game saves
    |__ extdata/          extended data (DLC content)
    |__ boss_extdata/     SpotPass / StreetPass data
```

**Default `backup_path`:** `sdmc:/3ds/fast-uninstall/backups`  
The path is overridable at build time: `-DDEFAULT_BACKUP_PATH="..."` (used by host tests).

**Alternative paths** (cycle with d-pad left/right in Settings):
1. `sdmc:/3ds/fast-uninstall/backups`
2. `sdmc:/backups/3ds-titles`
3. `sdmc:/save-backups`
4. `sdmc:/3ds-backups`
5. `sdmc:/backups`

---

## Configuration

Config file: `sdmc:/3ds/fast-uninstall/config.ini`

```ini
backup_path=sdmc:/3ds/fast-uninstall/backups
force_backup=0
skip_uninstall_confirm=0
force_restore=0
skip_install_confirm=0
```

---

## Building from Source

### Prerequisites

```bash
# Install devkitPro — see https://devkitpro.org/wiki/Getting_Started
sudo dkp-pacman -S 3ds-dev
```

Environment variables (add to `~/.bashrc` or `~/.zshrc`):

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
```

### Running Tests (host)

Unit tests compile and run on the development host without a 3DS or devkitARM:

```bash
make -C tests
```

Five binaries, no 3DS hardware required: `formatSize`/`sanitizeName`
(`test_utils`), config round-trip (`test_config`), comparators/safety
filter/`smdhSelectName`/filter logic (`test_titles`), the backup/restore
archive-copy logic (`test_backup`, via a small in-memory fake `FS_Archive`
in `tests/shims/fake_fs.h` that actually exercises copy/failure/cleanup
behavior) and CIA header parsing/folder scanning (`test_install`). See
`CLAUDE.md`'s "Test coverage" section for exactly what is and isn't
exercised this way — UI-driven code (`draw.c`, `input.c`'s blocking flows)
and the real AM install/delete calls still need on-device verification.

### Build

```bash
git clone https://github.com/Marcogn/3ds-fast-uninstall.git
cd 3ds-fast-uninstall
make clean && make
```

Output: `3ds-fast-uninstall.3dsx`, `3ds-fast-uninstall.elf`, `3ds-fast-uninstall.smdh`

| Error | Fix |
|---|---|
| `3ds.h: No such file or directory` | `sudo dkp-pacman -S libctru` |
| `Please set DEVKITARM in your environment` | Export `DEVKITARM` as above |
| Linker errors | `make clean && make` |

---

## Technical Reference

**Language:** C (modular architecture)  
**SDK:** libctru (devkitARM)  
**Graphics:** citro3d + citro2d (hardware-accelerated, flicker-free double buffering)  
**Services:** AM (title management), FS (file system / save archives)  
**Target:** Nintendo 3DS, New 3DS, 2DS (all variants)

### Architecture

The application is structured into **9 modules** for maintainability and testability:

```
source/
├── types.h            — Type definitions, enums, structs, constants
├── globals.h/c        — Shared global state (externalized from main.c)
├── utils.h/c          — Utility functions (createDirectory, formatSize, etc.)
├── config.h/c         — Config load/save (INI parsing)
├── titles.h/c         — Title loading (optimized), sort/filter, backup-dir helpers
├── backup_restore.h/c — Save backup, restore, title deletion
├── install.h/c        — CIA scanning and installation
├── draw.h/c           — All rendering functions (~790 lines)
├── input.h/c          — Input handlers and blocking flows (~580 lines)
├── lang.h/c           — UI string translation (English / Italian)
└── main.c             — Entry point + main loop (135 lines)
```

**No Makefile changes required**: the devkitPro template's `$(wildcard $(dir)/*.c)` includes all `.c` files automatically.

#### Module Relationship Diagram

```
main.c
  ├── globals.h/c        (shared state — all modules read/write via extern)
  ├── config.h/c         (INI parse/save — reads/writes globals.config)
  ├── titles.h/c         (loadTitles, sort/filter — reads/writes globals.titles[])
  │     └── [draw.h]     (calls drawLoadingScreen during load — forward-declared)
  ├── draw.h/c           (all rendering — reads globals, calls titles helpers)
  ├── input.h/c          (input + blocking flows — calls draw, titles, backup, install)
  │     ├── backup_restore.h/c  (save backup, restore, delete — calls draw for dialogs)
  │     └── install.h/c         (CIA scan, install — calls draw for progress)
  └── utils.h/c          (pure helpers — no globals, no 3DS services; host-testable)

lang.h/c  ─── T(id) string lookup, called from draw.c/input.c/backup_restore.c
              (reads globals.config.language; no other module dependencies)
types.h   ─── included by all modules (type definitions shared everywhere)
```

#### App State Machine

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

The main loop dispatches draw calls and input handlers based on `appState`. Blocking flows (uninstall dialog chain, install progress, backup progress) run as sub-loops **before** `C3D_FrameBegin` to avoid nested frame management.

### Core Data Structures

```c
typedef struct {
    u64            titleID;
    char           name[256];     // truncated for list view
    char           fullName[256]; // used in detail views
    FS_MediaType   mediaType;     // MEDIATYPE_SD or MEDIATYPE_NAND
    bool           selected;
    bool           isValid;       // false after successful deletion
    u16            version;
    bool           hasBackup;
    C2D_Image      icon;          // reserved, not currently loaded
    bool           iconLoaded;
    u64            size;
} TitleInfo;

typedef struct {
    char backupPath[256];
    bool forceBackup;
    bool skipUninstallConfirm;
    bool forceRestore;
    bool skipInstallConfirm;
} Config;

typedef struct {
    int idx;    // selected category (0=Games, 1=Updates, 2=DLC, 3=System)
    int cursor; // selected row within the category list
} SysInfoDetailState;

typedef struct {
    char name[256];
    bool isDir;
    bool isCIA;
    u64  size;
    u64  titleID; // cached at scan time; 0 for dirs or unreadable CIAs
} FileEntry;
```

### Rendering Pipeline — when frames are managed

The app has two rendering modes that must never overlap:

**Normal frames (main loop):**
```
aptMainLoop()
C3D_FrameBegin(C3D_FRAME_SYNCDRAW)
  C2D_TextBufClear(dynamicBuf)
  C2D_TargetClear(top, CLR_BG)
  C2D_SceneBegin(top)     → draw top screen content at z ≈ 0.5
  C2D_TargetClear(bottom, CLR_BG)
  C2D_SceneBegin(bottom)  → draw bottom screen content at z ≈ 0.5
  [if SELECT held: drawHelpOverlay() draws at z = 0.7 — NO second SceneBegin]
C3D_FrameEnd(0)
```

**Blocking flows** (install progress, backup progress, uninstall dialog chain, SysInfo detail)
run **before** `C3D_FrameBegin` in the main loop and manage their own frames:
```
runInstallFlow() / runBackupFlow() / runUninstallDeleteFlow() / runSysInfoDetailFlow() {
    while (aptMainLoop()) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW)
            drawFileBrowserScreen() / drawBackupScreen() / drawDialog() / drawTitleDetails()
        C3D_FrameEnd(0)
        hidScanInput(); handle input; break when done
    }
}
```

**Rule — never violate**: `C2D_SceneBegin` must be called **exactly once per target per frame**.  
A second call on the same target clears it and causes flickering.  
Overlays (SELECT help) must be drawn into the **already-active** scene at higher z-depth (0.5 → 0.7).  
`drawLoadingScreen` and `drawInstallProgressScreen` each call `C3D_FrameBegin/End` themselves —  
they must never be called from inside an already-open frame.

### Title Loading & Language Fallback

1. `AM_GetTitleCount` for SD + NAND
2. `AM_GetTitleList` fills a `u64` array of title IDs
3. Safety filter: skip ranges `0x00040010`, `0x00040030`, `0x00040138`
4. `AM_GetTitleInfo` -> size, version, media type
5. SMDH icon handle -> UTF-16 short description extracted
6. UTF-16 -> UTF-8 with language fallback:

   ```
   1. System language (cfguGetSystemLanguage)
   2. English (index 1)
   3. Japanese (index 0)
   4. All 12 SMDH language slots
   5. Fallback: "Title [TitleID]"
   ```

7. Clamp to `MAX_TITLES` (500); call `sortTitles()`

Names consisting only of spaces or `--` are rejected and the next fallback is tried.

### Sort & Filter Internals

```c
static int filteredIndices[MAX_TITLES]; // indices into titles[]
static int filteredCount;
```

`updateFilteredList()` rebuilds `filteredIndices` by iterating `titles[]` (only `isValid == true` entries) and applying `currentFilterMode`. `sortTitles()` sorts `titles[]` in-place via `qsort` with one of three comparators (name, size, titleID).

### Memory Budget

| Allocation | Size |
|---|---|
| `titles[500]` | ~120 KB |
| `filteredIndices[500]` | ~2 KB |
| `fileEntries[256]` | ~70 KB |
| `dynamicBuf` (text buffer) | 4 KB |
| `sysInfoSubIndices[500]` | ~2 KB |
| **Total approx.** | **~200 KB** |

The 3DS homebrew heap is typically 32-64 MB — well within budget.

### UI Layout Constants

| Element | x | y | scale |
|---|---|---|---|
| Header bar | 0 | 0 | — |
| Header text (all screens) | 4 | 4–6 | 0.54f |
| List rows (all screens) | varies | 32 + i × 14.5 | 0.38f |
| Type badge ^ / + (all list screens) | 240 | row y | 0.44f |
| TitleID right-aligned (Uninstall/Backup) | 292 | row y | 0.38f |
| Size right-aligned (Install) | 396 − len × 5.7 | row y | 0.38f |
| Size right-aligned (SysInfo) | 396 − len × 5.7 | row y | 0.38f |
| Bottom hint bar | 0 | 222 | — |
| Bottom hint text | 4 | 224 | 0.52f |
| Detail panel — name | 4 | 22 | 0.52f |
| Detail panel — ID (`ID: XXXXXXXXXXXXXXXX`) | 4 | 42 | 0.52f |
| Detail panel — version + size + location | 4 | 60 | 0.52f |
| Detail panel — backup status | 4 | 78–80 | 0.52f |

### Known Limitations

- Icon images (`C2D_Image icon`) are allocated in `TitleInfo` but not loaded at runtime (complexity vs. benefit tradeoff).
- Max file size per backup copy: `MAX_FILE_SIZE` = 100 MB.
- Title name truncation: 32 chars in list views; full name shown in all detail/bottom panels.
- Sleep mode: `aptMainLoop()` returns `false` during sleep, pausing the main loop. No aggressive frame-skipping permitted — rendering must continue every frame when awake.

---

## Documentation

- [`CHANGELOG.md`](CHANGELOG.md) — what shipped in each release
- [`CLAUDE.md`](CLAUDE.md) — contributor/architecture guide: conventions,
  test coverage, backup/deletion invariants to preserve, build notes
- [`TODO.md`](TODO.md) — known technical debt and potential optimizations

## Contributing

This is a personal project built primarily for single-user use, so there's
no public feature roadmap open to proposals. Bug reports and small,
focused fix PRs are welcome — open an issue before working on anything
substantial to avoid wasted effort. Most changes should also come with a
`CHANGELOG.md` entry and, where it's testable on host (see `CLAUDE.md`'s
"Test coverage" section), a test under `tests/`.

## About this project

3ds-fast-uninstall is an independent project, built by Marcogn in close
collaboration with Claude, Anthropic's AI coding assistant — Claude wrote
and reviewed most of the code, under direct human design and review at
every step.

## Disclaimer

Use at your own risk. Always keep additional backups of important save
data before uninstalling titles. The authors are not responsible for any
data loss.

## License

MIT, see [LICENSE](LICENSE).
