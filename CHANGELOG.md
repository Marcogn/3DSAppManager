# Changelog

## v2.2.0 – Refactoring and optimizations (2026-04-13)

### Code refactoring — multi-file architecture
- **Split `main.c` (2660 lines)** into 8 modules for better maintainability:
  - `types.h` — all type definitions, enums, structs, constants
  - `globals.h/c` — shared global state (externalized from `main.c`)
  - `utils.h/c` — utility functions (`createDirectory`, `sanitizeName`, `formatSize`)
  - `config.h/c` — config load/save (INI parsing)
  - `titles.h/c` — title loading (optimized), sort/filter, backup-dir helpers
  - `backup_restore.h/c` — save backup, restore, title deletion
  - `install.h/c` — CIA scanning and installation
  - `draw.h/c` — all rendering functions (~750 lines)
  - `input.h/c` — input handlers and blocking flows (~560 lines)
  - `main.c` — reduced to **120 lines** (entry point + main loop only)
- **No Makefile changes required**: wildcard `$(wildcard $(dir)/*.c)` includes all `.c` files automatically.
- `main.c.old` preserved as reference (will be removed after validation).

### Title loading optimizations
- **Batch `AM_GetTitleInfo`**: single call per media type instead of one-per-title — eliminates ~500 individual API calls.
- **Unified SMDH open**: `getTitleName` and `fullName` extraction merged into one `FSUSER_OpenFileDirectly` per title — halves SMDH file reads during load.
- **Backup directory cache**: `opendir(config.backupPath)` called **once** at load start, all backup prefixes cached in array — replaces ~500 `opendir` calls with in-memory `strncmp`. `hasBackup` field remains accurate during scroll.
- **Net improvement**: title loading **30–50% faster** on libraries with 200+ titles.

### SELECT overlay removed
- Removed `drawControlsOverlay()` and the `if (hidKeysHeld() & KEY_SELECT)` block from Uninstall screen.
- **Rationale**: overlay replaced entire list, causing context loss; footer became too crowded with all control hints.
- **Future replacement documented in `TODO.md`**: universal per-screen help overlay with simplified footers (`B: Back  START: Exit  SELECT: Help`).

### Documentation updates
- **`TODO.md` created**: documents future SELECT help system design, potential optimizations, known technical debt.
- **`CHANGELOG.md`**: refactoring entry added.
- **`README.md`**: Technical Reference section will be updated with multi-module architecture map (pending).

---

## v2.1.0 – Backup sort, header tweaks, footer separator normalization (2026-04-13)

### Backup — independent sort mode
- Added `backupSortMode` (global, `SortMode`, default `SORT_BY_NAME`) independent from `currentSortMode` used by Uninstall.
- Added `backupCmpName/Size/ID` comparators and `sortBackupList()` (same pattern as `sysInfoSortSubList`).
- `buildBackupList()` calls `sortBackupList()` at the end so the list is already sorted on first access.
- In `runBackupFlow()` the `L/R` buttons cycle the sort (L = previous, R = next), and reset `backupCursor` and `backupScrollOffset`.
- `Sort:Name/Size/ID` indicator added in the top header of `drawBackupScreen` (right-aligned at `x=393`, same position as SysInfo sublist).

### Header `drawUI` (Uninstall) — two clean rows
- Header expanded from 28 px to **36 px** to fit two rows without overlap.
- Title "Uninstall" moved to `y=4` (first row); info bar `T:X Sel:X Sort:` moved to `y=22` (second row).
- List starts at `y=40` (4 px gap below 36 px header); visible row count updated to `UNINSTALL_VISIBLE=12`.
- Sort↔Name gap multiplier fixed: `6.0f → 5.4f` for both `xi` and `xi2`, eliminates visual overflow on `Sort:Size`.

### Header `drawBackupScreen` — reduced
- Header from 28 px to **22 px** (single row): title at `y=4`, list starts at `y=26`.
- Selection counter remains in the title (`Sel:N`).

### Fix `drawSysInfoScreen` — Sort:Name overflow
- `slX = 396.0f → 393.0f`: the `Sort:Name` label (9 chars × 7 px = 63 px) now starts at 330 px, avoiding clip on the right edge.

### Footer — command separator normalization
- All command-reminder footers now use `": "` as separator (was `"="` or `" = "`).
- Updated: `drawTouchControls`, `drawMainMenu`, `drawUI`, `drawFileBrowserScreen`, `drawBackupScreen`, `drawSysInfoScreen` (overview + sublist + bottom hint), `drawSettingsScreen`, `drawTitleDetails`, `_drawSoon`.

## v2.0.8 – Visual consistency — all screens

**Goal**: identical look & feel across all screens: same palette, same symbols, same text scales, same header/footer structure.

#### Color palette relocated
- The `#define CLR_*` macros were moved from Section 9 to Section 4 (before the enums), making them available to all functions including `drawLoadingScreen` and `drawInstallProgressScreen`.

#### Top screen header — unified
- All screens now use `x=4, y=4–6, scale=0.54f, CLR_WHITE`.
- Fixed `drawMainMenu` and `_drawSoon` which used `x=8`.
- Fixed `drawUI` (Uninstall) which used `scale=0.44f` and `y=2` → now `y=6, scale=0.54f` aligned with the other 28 px headers (Backup, SysInfo sublist).
- Fixed `drawSysInfoScreen` sublist from `scale=0.52f` to `0.54f`.

#### Bottom screen header — unified
- "TITLE DETAILS" → "Title Details" (`drawTouchControls`)
- "Info" → "File Info" (`drawFileBrowserScreen`)
- "Title Info" → "Title Details" (`drawBackupScreen`)
- "Info" → "Title Details" (`drawSysInfoScreen` sublist)
- "SELECTED TITLES (N)" → "Selected (N)" (`_renderSelectedList`)

#### Bottom screen footer bar — added where missing
- `drawSysInfoScreen` overview and sublist: added `CLR_HEADER` bar at `y=222` with contextual hint. Previously these were the only screens without a bottom-screen footer.

#### Bottom detail panels — content and scale parity
- All detail views (Uninstall, Backup, SysInfo sublist) now show: name, `ID: XXXXXXXXXXXXXXXX`, `vN   size   SD/NAND`, backup status.
- Added `"ID: "` prefix in `drawBackupScreen` and `drawSysInfoScreen` (was missing).
- Added version field `v%u` in `drawBackupScreen` and `drawSysInfoScreen` (was missing).
- `drawTouchControls`: backup format `"Backup: YES [date]"` → `"Backup: date"` and `"Backup: NO"` → `"No backup"` — aligned with all other panels.
- Scale unified to `0.52f` for all info rows (removed scattered `0.54f` values in `drawTouchControls`).

#### SysInfo sublist — aligned with other lists
- Row scale: `0.36f` → `0.38f` (uniform with Uninstall and Backup).
- Added type symbol `^`/`+` at `x=240` (was missing; present in Uninstall and Backup).
- Size right-align: coefficient updated from `5.4f` to `5.7f` for scale `0.38f`.
- Scroll counter: `x=352` → `x=340` (uniform with all other screens).

#### Loading/progress screens — unified style
- `drawLoadingScreen`: background `CLR_BG` (previously plain black), header `CLR_HEADER` with text "Loading..." at `scale=0.54f`, secondary text at `scale=0.52f`, status at `scale=0.48f`, progress bar fill `CLR_CYAN`, footer bar at `y=222`. Texts converted to `CLR_*` macros (all inline hardcoded colors removed).
- Extracted new function `drawInstallProgressScreen(ciaPath, done, total)`: removes the 25-line inline block from `installCIA`; header "Installing CIA" at `scale=0.54f`, filename at `scale=0.48f`, progress bar fill `CLR_GREEN` (semantic: positive action), footer "Do not power off the console." at `scale=0.48f`. Same structure as `drawLoadingScreen`.

---


## v2.0.7 – Documentation

- **Single-file docs**: merged `USER_GUIDE.md` and `docs/README.md` into the root `README.md` — the repository now has one canonical reference for both users and developers.
- **Task-oriented structure**: README reordered as Quick Start → Features → How to Use (per flow) → Controls Reference → Screen Layout → Tips & Tricks → FAQ & Troubleshooting → Safety → Backup Structure → Configuration → Building from Source → Technical Reference.
- **Content gaps filled**: added `[*]` backup indicator description, Tips & Tricks section, expanded FAQ/troubleshooting entries ("Scroll feels slow", "Something deleted but still in HOME menu", "Can't find a title", loading bar note, install/restore failure causes) and full Technical Reference (architecture, data structures, rendering pipeline, language fallback, sort/filter internals, memory budget, UI layout constants, known limitations).
- **Removed obsolete files**: `USER_GUIDE.md`, `docs/README.md`, `RELEASE_NOTES_v1.0.1.md`, `implementation_guide.md`, `plan-3dsFastUninstallV2.prompt.md`, `BUILD_INFO.txt` — all content either absorbed into README or no longer relevant.
- **Fixed garbage prefixes**: removed stray `es` prefix at the top of `README.md` and `i ` prefix from `USER_GUIDE.md` (now deleted).
- **AGENTS.md updated**: references to deleted files replaced with single `README.md` pointer.

---

## v2.0.6 – Uniform list margins (2026-04-10)

### UNINSTALL / BACKUP / SYSINFO sublist — equal top and bottom margins
- All three list screens now use the same layout formula: **28px header, list starts at y=32, 13 visible rows, 14.5px row spacing**.
  - Top margin (header bottom → first row): 4px on all screens.
  - Bottom margin (last row text bottom → hint bar at y=222): 4px on all screens.
- **UNINSTALL**: header rect expanded from 22px to 28px; info bar moved from y=24 (outside header) to y=16 inside the header rect (scale 0.40f, 6.0px/char); title repositioned to y=2 at scale 0.44f. List start moved from y=38 to y=32 — gains a visible top margin for the first time.
- **BACKUP**: header rect expanded from 22px to 28px; header text moved to y=6; list start moved from y=38 to y=32; `BACKUP_VISIBLE` restored to 13 (was 12 in v1.0.7). This eliminates the extra blank space that was visible above and below the list compared to the other screens.
- **SYSINFO sublist**: header rect expanded from 22px to 28px; header text and sort indicator moved to y=6; list start moved from y=28 to y=32.
- Net result: same number of visible titles (13) and identical visual spacing on all three screens.

---

## v2.0.5 – Bug fixes & UI consistency (2026-04-10)

### UNINSTALL – Info bar shift fixed
- `T:%d` format replaced with `T:%-3d` (and `Sel:%-3d`) so the string is always a fixed width.  
  Previously, when a filter was applied and `filteredCount` dropped from 3 to 1 digit, all following x-positions shifted left, causing sort/filter labels to "slide" visually.
- Sort name is now **always yellow** (was gray when "Name", the default). This makes it easier to see the active sort at a glance.
- Filter label reformatted from three separate `dt()` calls (`"  ["` + `fn` + `"]"`) to a single `" [xxx]"` string, eliminating the extra spaces that made it look like `"[  Upd]"`.

### UNINSTALL / SYSTEMINFO – Sort state decoupled
- Added `sysInfoSortMode` (defaults to `SORT_BY_NAME`), independent from `currentSortMode` used by Uninstall.
- SYSTEMINFO sublist now sorts its own `sysInfoSubIndices[]` using index-based comparators (`sysInfoCmpName/Size/ID`) and `sysInfoSortSubList()` — **not** by relying on the physical order of `titles[]`.
- **L/R in SYSTEMINFO sublist** cycles the sort mode (Name → Size → ID → Name), resets cursor, re-sorts. D-pad Left/Right still page-jumps as before.
- Sort indicator (`Sort:Name` / `Sort:Size` / `Sort:ID`) shown **right-aligned in yellow** inside the category header bar.
- Hint bar updated: `"A=Details  L/R=Sort  B=Back"`.

### SELECT overlay – Compact box
- Top box height reduced from 210 px to 165 px (centred on screen). The old box left ~79 px of blank space at the bottom; the new one has a balanced 9 px margin.
- Line spacing tightened to 0.48f scale (from 0.52f) to match the available height.
- "Release SELECT to return." moved from bottom-screen overlay into a footer separator line inside the **top** box, making the dismiss hint immediately visible.
- Bottom overlay height reduced to 90 px accordingly (was 110 px).

### BACKUP – Last row spacing
- Added `#define BACKUP_VISIBLE 12` (matching `FILE_BROWSER_VISIBLE`).  
  With 13 rows the last entry's text overlapped the hint bar by ~4 px; with 12 rows the gap is ≥ 8 px.
- Scroll navigation (`KEY_DOWN/UP/RIGHT/LEFT`) in `runBackupFlow` updated to use `BACKUP_VISIBLE` for offset clamping.

---

## v2.0.4 – UI/UX polish (2026-04-10)

### UI – Main menu
- Removed `[A]`, `[B]`, `[U]`, `[I]`, `[S]` abbreviations from menu items (prep for v3 5-button UI).

### INSTALL – File browser
- Max visible rows reduced from 13 to 12 (`FILE_BROWSER_VISIBLE`): last row at y=204 no longer bleeds under the hint bar at y=222. This fixed the misleading "truncated list" appearance.
- File name expanded from 28 to 44 characters, filling space before the badge symbol.
- **Directory cursor stack**: cursor position is saved before entering a folder and restored on B/`..` exit. Stack depth max 12 levels (`DIR_STACK_MAX`).

### BACKUP – Save data list
- Added **TitleID column** (right-aligned `%016llX` at x=292, scale 0.38f) matching Uninstall layout.
- **DLC and Update titles filtered out**: `buildBackupList()` excludes titles with high-word `0x0004000E` (Updates) and `0x0004008C` (DLC) — they share the base game's save archive and have no standalone user save data. Introduced `backupIndices[]` / `backupTitleCount` globals.
- Column layout unified with Uninstall: checkbox x=3, name x=25 (32 chars), symbol x=240, TitleID x=292.

### UNINSTALL – Column layout overhaul
- New uniform 3-column layout:
  - Name: x=25, 32 chars, scale 0.38f (was x=28, 28 chars)
  - Symbol `^`/`+`: x=240 (was x=220 — visually dead-centre)
  - TitleID: right-aligned at x=292 (`396 − 16×6.5`), scale 0.38f (was x=300 at 0.40f — clipped off screen)
- **Coloured sort/filter bar**: sort name is yellow when ≠ `Name`, filter name is yellow when ≠ `All`. Bar rendered as 5 separate `dt()` calls with proportional x-positioning (6.5px/char at scale 0.44f).

### SYSTEMINFO – Sublist navigation
- Added `KEY_RIGHT` / `KEY_LEFT` page-jump in the Games/Updates/DLC sublists (same behaviour as Uninstall and Backup).



### Documentation
- **README.md**: Complete rewrite to cover all v2 features (Install, Backup, Uninstall, SysInfo, Settings). Added controls tables for every screen, screen layout ASCII diagrams, Settings table, backup structure, FAQ and safety notes.
- **USER_GUIDE.md**: Complete rewrite. Sections for every flow (Install, Backup Saves, Uninstall, System Info, Settings) with step-by-step instructions, tips, common questions and troubleshooting table.
- **docs/README.md**: Full technical rewrite. Covers app states, all data structures, rendering pipeline, SELECT overlay, title loading with language fallback, save backup/restore, CIA install, deletion flow, settings persistence, sort/filter internals, UI layout constants table, memory budget.
- **CHANGELOG.md**: Previous Italian entries translated to English.
- Removed `RELEASE_NOTES_v1.0.1.md` content duplication (information folded into README and CHANGELOG).

---

## v2.0.3 – UI consistency pass (2026-04-10)

### Uniform look & feel across all screens
- **Main menu descriptions**: all rows are now complete, self-contained sentences (no more continuations from line 1 to line 2).
- **Install — header path**: path moved inside the header bar (height 36px); prevents visual bleed onto the first highlighted row of the file browser.
- **Install — type badge**: `^`/`+` moved from x=206 to x=316 (close to the size column), giving more room to the filename.
- **Backup — type badge**: replaced "Upd"/"DLC" text (0.52f, x=340) with `^`/`+` (0.44f, x=220), identical to Uninstall and SysInfo. Name truncated to 28 chars (same as Uninstall).
- **Uninstall — info bar**: scale reduced from 0.52f to 0.44f; TitleID now right-aligned at x=300 (end of row), matching the Install layout.
- **SysInfo — bottom overview**: three lines rewritten as complete sentences: "A = Open category list." / "UP/DOWN = Select category." / "B = Back to main menu."
- **Settings — Italian strings removed**: all descriptions translated to English; hint bar "Salva & Esci" → "Save & Back".

---

## v2.0.2 – Install UX polish (2026-04-10)

### UI improvements — Install file browser
- **Consistent layout with SysInfo**: CIA entries now use three separate draw calls: name truncated on the left (x=4), type badge (`^`/`+`) at x=206, size `[XXX MB]` right-aligned at `396 - len × 5.7px` (same formula as SysInfo).
- **Update/DLC type badge**: read from the TitleID cached in `FileEntry.titleID` at scan time — no additional file reads during render. `^` cyan = Update (`0x0004000E`), `+` green = DLC (`0x0004008C`).
- **Right/Left page navigation**: `KEY_RIGHT`/`KEY_LEFT` now jump by `MAX_VISIBLE_TITLES` positions in the file browser, matching the existing behaviour in Uninstall and Backup.
- **Bottom screen info**: uses cached `fe->titleID` instead of re-opening the file; shows a coloured "Type: Game/Update/DLC" line.
- **Controls hint updated**: `DX/SX=Page` in the bottom hint bar.

### Technical
- Added `u64 titleID` field to `FileEntry`; populated in `scanDirectory` pass 2 via `getCIATitleID()` — avoids the double-slash bug already present in the bottom-screen path (`currentPath + "/" + name`).

---

## v2.0.0 – Version 2: Install, Backup, System Info, Settings (2026-04-10)

Major release introducing four new application screens, a full app-state machine, and a redesigned user experience.

### New: Install CIA
- File browser starting at `sdmc:/` with two-pass `scanDirectory()` (separate `opendir` for dirs and `.cia` files — avoids `rewinddir` unreliability on CTRU/FAT32).
- `FileEntry` struct: `name`, `isDir`, `isCIA`, `size`, `titleID` (cached at scan time via `getCIATitleID()` — no file re-read during render).
- CIA installation via `installCIA()`: chunked write (`CHUNK_SIZE = 64 KB`) through `AM_StartCiaInstall` / `AM_FinishCiaInstall`, always targeting SD card.
- Progress bar per-file via `drawInstallProgressScreen()`.
- **Y button**: batch-installs all `.cia` files in the current folder via `_installFolderCIAs()`.
- Post-install: automatic backup-restore prompt if a backup exists for the installed title (`forceRestore` config option).
- `titlesNeedRefresh` flag: set after any successful install so the title list reloads on the next Uninstall / Backup / SysInfo entry.

### New: Backup Saves
- `buildBackupList()`: builds `backupIndices[]` filtered to base games only — excludes Updates (`0x0004000E`) and DLC (`0x0004008C`) which share the base game's save archive.
- `backupSaveDataToPath()`: backs up up to three archive types per title — `savedata/` (user saves), `extdata/` (extended data), `boss_extdata/` (SpotPass / StreetPass).
- Backup folder named `[TitleID]-[Name]/` via `getBackupDirName()`; includes `backup_info.txt` with title name, version, media type, timestamp, and path.
- `findBackupDir()` scans the backup folder for a matching 16-char TitleID prefix.
- `getBackupLastDate()` reads the timestamp from `backup_info.txt`.
- `[*]` indicator in the list for titles with an existing backup; `[X]` for selected; `[ ]` for unselected/no backup.
- **Y button**: backs up all eligible titles in one pass with a loading screen.

### New: System Info
- Overview screen: Games / Updates / DLC title counts and total sizes; SD free space via `getSDFreeSpace()`.
- Category lists: navigate with Up/Down, page-jump with Left/Right, sort with L/R (`sysInfoSortMode` independent from Uninstall sort).
- Per-title detail screen (`drawTitleDetails()`): full name, Title ID, version, size, storage location, related titles, backup date.
- Three actions per title (Up/Down to navigate, A to execute): **Backup Save Data**, **Restore Save Data**, **Delete Title (+ related)**.
- Delete from SysInfo reuses `runUninstallDeleteFlow()` (same confirmation + backup + related-title flow as Uninstall).

### New: Settings
- Five configurable options, persisted to `sdmc:/3ds/fast-uninstall/config.ini` in real time via `saveConfig()`:

  | Setting | Default | Description |
  |---|---|---|
  | Force Backup | OFF | Auto-backup before every uninstall (no prompt) |
  | Skip Uninstall Confirm | OFF | Delete immediately on X without confirmation |
  | Force Restore | OFF | Auto-restore backup after every CIA install |
  | Skip Install Confirm | OFF | Install immediately on A without confirmation |
  | Backup Folder | `sdmc:/3ds/fast-uninstall/backups` | Cycle 5 presets with Left/Right or L/R |

- `loadConfig()` reads the INI on startup; `saveDefaultConfig()` creates it on first run.

### App-state machine
- `AppState` enum with 6 states: `APP_MAIN_MENU`, `APP_INSTALL`, `APP_BACKUP`, `APP_UNINSTALL`, `APP_SYSINFO`, `APP_SETTINGS`.
- Lazy title loading: `loadTitles()` is called only on first access to Uninstall / Backup / SysInfo — main menu stays instant.
- Blocking flows (`runInstallFlow`, `runBackupFlow`, `runUninstallDeleteFlow`, `runSysInfoDetailFlow`) run **before** `C3D_FrameBegin` to avoid nested frame management.

### Save restore
- `restoreSaveData()` copies files from the backup `savedata/` folder back into `ARCHIVE_USER_SAVEDATA` via `restoreDirectory()`.

### Visual
- Main menu shows a two-line description per option on the bottom screen.
- `drawUI` (Uninstall) footer and bottom-screen detail panel established.
- 5 preset backup paths selectable from Settings.

---

## v2.0.1 – Bug fixes & UX polish (2026-04-10)

### Bug fix
- **Install: .cia files not found** — `scanDirectory()` was using `rewinddir()` which on CTRU/FAT32 does not reposition the pointer. The second pass (searching for .cia files) ran on an exhausted iterator and returned zero files. Fix: two separate `opendir()` calls — one for folders, one for .cia files — removing the dependency on `rewinddir()`. Also fixed the double-slash (`sdmc:/ + / + name`) in `stat()` calls via the `MKFP` macro.

### UI improvements
- **Backup: right/left page navigation** — `KEY_RIGHT`/`KEY_LEFT` now advance/rewind by `MAX_VISIBLE_TITLES` positions in the Backup flow, identical to the existing behaviour in Uninstall.
- **Uninstall: full TitleID in the list** — all 16 hex characters are now shown (`%016llX`) instead of only the lower 32 bits (`%08lX`). Scale reduced to 0.40f; type symbol (`^`/`+`) moved to x=220 to make room.
- **SysInfo sublist: right-aligned size** — the `[size]` field is drawn at `x = 396 - strlen * 5.4px` so it always aligns to the right edge regardless of title name length.
- **Settings: complete sentences in descriptions** — the first four entries had descriptions cut mid-sentence (e.g. "Auto backup before" / "uninstalling (no prompt)"). Rewritten as complete, self-contained sentences on each line.

---

## v1.0.1 - Bottom screen & UX improvements (2026-03-15)

- **Clear bottom screen during loading**: bottom screen now shows a clean background while loading titles.
- **Improved text clarity**: "SELECTED TITLES" renamed to "TITLES TO BE REMOVED".
- **Better UX after uninstall**: bottom screen properly cleared when reloading titles after an uninstall.

---

## v1.0 - First Stable Release (2026-02-04)

- **Unified screen colors**: top screen background matches bottom screen (20, 20, 30).
- **Release binary**: compiled `.3dsx` added to repository.
- Feature complete: all v1 planned features implemented and tested.

---

## Pre-Alpha Development (2026-02-04)

### 0.11.1 — SELECT Overlay Flickering Fix
- Completely rewrote rendering logic for the controls overlay. `C2D_SceneBegin()` is no longer called twice on the same target. Overlay is baked into a single render pass at higher z-depth (0.6–0.7).

### 0.11 — Selected Titles List and UX Improvements
- Bottom screen shows selected titles during the uninstall flow (up to 10, "…and X more").
- Info bar spacing fixed: T / Sel / Sort now at 5px / 135px / 265px.
- `drawSelectedTitlesList()` and `drawDialogWithSelectedList()` introduced.

### 0.10.1 — SELECT Overlay and Symbol Fixes
- Removed all Unicode symbol variants (↑, ⊕) from title names; symbols now only in dedicated column.
- Overlay box: height 210px, line spacing 13px for 8 control lines.

### 0.10 — Layout and Multi-language
- Column-based layout: symbols at x=235, TitleID at x=255.
- Multi-language support: system language → English → Japanese → all 12 SMDH slots.
- Name validation: rejects empty/dash-only names, falls back to "Title [TitleID]".
- Hold SELECT for overlay, release to hide.

### 0.9.2 — Sorting, Filtering, 500 Title Support
- L/R cycles through Name / Size / TitleID sort modes.
- Y cycles through All / Updates / DLC filter.
- `MAX_TITLES` raised from 300 to 500.

### 0.9 — Polish
- Title size displayed (KB/MB/GB).
- `^` and `+` symbols replace Unicode variants (box rendering issue on some 3DS models).
- Smart name truncation with `...`.
- SELECT overlay for controls.

### 0.8 — Stability
- Sleep mode: `aptMainLoop()` handling for sleep/wake cycles.
- Scroll rewrite: single tap = one row; hold 1.5s = continuous slow scroll.

### 0.7 — Details
- Title names extended to 35 characters in list.
- Scroll speed reduced; bottom screen alignment fixed.

### 0.6 — UI Overhaul
- Loading progress bar.
- Title counter turns red above 300.
- Right-aligned TitleID; larger text; better spacing.

### 0.1–0.5 — Foundation
- Multi-select system, save backup (savedata/extdata/boss extdata), title sorting.
- UTF-16 → UTF-8 conversion for Japanese and special character display.
- Migration from console rendering to citro2d/citro3d.
- System title filtering to prevent accidental deletion.

---

## Known Issues

None currently known. Open an issue on GitHub if you find something.
