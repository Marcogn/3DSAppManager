# Changelog

## v2.1.0 – Sort backup, header tweaks, footer normalizzazione (2026-04-13)

### Backup — ordinamento indipendente
- Aggiunto `backupSortMode` (globale, `SortMode`, default `SORT_BY_NAME`) separato da `currentSortMode` dell'Uninstall.
- Aggiunte funzioni `backupCmpName/Size/ID` e `sortBackupList()` (stesso schema di `sysInfoSortSubList`).
- `buildBackupList()` chiama `sortBackupList()` al termine, così la lista è già ordinata al primo accesso.
- In `runBackupFlow()` i tasti `L/R` ciclano il sort (L = precedente, R = successivo), resettano `backupCursor` e `backupScrollOffset`.
- Indicatore `Sort:Name/Size/ID` aggiunto nell'header top di `drawBackupScreen` (right-aligned a `x=393`, stessa posizione della SysInfo sublist).

### Header `drawUI` (Uninstall) — due righe pulite
- Header espanso da 28px a **36px** per contenere le due righe senza sovrapposizione.
- Titolo "Uninstall" spostato a `y=4` (prima riga); info-bar `T:X Sel:X Sort:` spostata a `y=22` (seconda riga).
- La lista inizia a `y=40` (4px gap sotto header da 36px); numero di righe visibili aggiornato a `UNINSTALL_VISIBLE=12`.
- Moltiplicatore gap Sort↔Name corretto: `6.0f → 5.4f` per entrambi `xi` e `xi2`, elimina overflow visivo su `Sort:Size`.

### Header `drawBackupScreen` — ridotto
- Header da 28px a **22px** (una riga): titolo a `y=4`, lista parte da `y=26`.
- Il contatore di selezione rimane nel titolo (`Sel:N`).

### Fix `drawSysInfoScreen` — Sort:Name overflow
- `slX = 396.0f → 393.0f`: il label `Sort:Name` (9 caratteri × 7px = 63px) inizia ora a `330px`, evitando clip sul bordo destro.

### Footer — normalizzazione separatore comandi
- Tutti i footer di reminder comandi usano ora `": "` come separatore (era `"="` o `" = "`).
- Aggiornati: `drawTouchControls`, `drawMainMenu`, `drawUI`, `drawFileBrowserScreen`, `drawBackupScreen`, `drawSysInfoScreen` (overview + sublist + hint bottom), `drawSettingsScreen`, `drawTitleDetails`, `_drawSoon`.



### Visual consistency — all screens

**Obiettivo**: look & feel identico su tutte le schermate: stessa palette, stessi simboli, stesse scale tipografiche, stessa struttura header/footer.

#### Palette colori spostata
- I `#define CLR_*` sono stati spostati dalla Section 9 alla Section 4 (prima degli enum), rendendoli disponibili a tutte le funzioni incluse `drawLoadingScreen` e `drawInstallProgressScreen`.

#### Header top screen — uniformato
- Tutte le schermate usano ora `x=4, y=4–6, scale=0.54f, CLR_WHITE`.
- Corretti `drawMainMenu` e `_drawSoon` che usavano `x=8`.
- Corretto `drawUI` (Uninstall) che usava `scale=0.44f` e `y=2` → ora `y=6, scale=0.54f` allineato agli altri header da 28px (Backup, SysInfo sublist).
- Corretta `drawSysInfoScreen` sublist da `scale=0.52f` a `0.54f`.

#### Header bottom screen — uniformato
- "TITLE DETAILS" → "Title Details" (`drawTouchControls`)
- "Info" → "File Info" (`drawFileBrowserScreen`)
- "Title Info" → "Title Details" (`drawBackupScreen`)
- "Info" → "Title Details" (`drawSysInfoScreen` sublist)
- "SELECTED TITLES (N)" → "Selected (N)" (`_renderSelectedList`)

#### Footer bar bottom screen — aggiunto dove mancava
- `drawSysInfoScreen` overview e sublist: aggiunto `CLR_HEADER` bar a `y=222` con hint contestuale. In precedenza erano gli unici schermi senza footer sulla bottom screen.

#### Pannelli dettaglio bottom — parità di contenuto e scala
- Tutte le viste dettaglio (Uninstall, Backup, SysInfo sublist) mostrano ora: nome, `ID: XXXXXXXXXXXXXXXX`, `vN   size   SD/NAND`, stato backup.
- Aggiunto prefisso `"ID: "` in `drawBackupScreen` e `drawSysInfoScreen` (mancante).
- Aggiunto campo versione `v%u` in `drawBackupScreen` e `drawSysInfoScreen` (mancante).
- `drawTouchControls`: formato backup `"Backup: YES [data]"` → `"Backup: data"` e `"Backup: NO"` → `"No backup"` — allineato al formato usato da tutti gli altri pannelli.
- Scale uniformata a `0.52f` per tutte le righe info (rimossi i `0.54f` sparsi in `drawTouchControls`).

#### Lista SysInfo sublist — allineata alle altre liste
- Scale righe: `0.36f` → `0.38f` (uniforme con Uninstall e Backup).
- Aggiunto simbolo tipo `^`/`+` a `x=240` (mancante; presente in Uninstall e Backup).
- Size right-align: coefficiente aggiornato da `5.4f` a `5.7f` per scala `0.38f`.
- Scroll counter: `x=352` → `x=340` (uniforme con tutti gli altri schermi).

#### Schermate di caricamento/progress — stile unificato
- `drawLoadingScreen`: sfondo `CLR_BG` (prima pure nero), header `CLR_HEADER` con testo "Loading..." a `scale=0.54f`, testo secondario a `scale=0.52f`, status a `scale=0.48f`, barra progress fill `CLR_CYAN`, footer bar a `y=222`. Testi convertiti a macro `CLR_*` (rimossi tutti i colori inline hardcoded).
- Estratta nuova funzione `drawInstallProgressScreen(ciaPath, done, total)`: rimuove il blocco inline di 25 righe da `installCIA`; header "Installing CIA" a `scale=0.54f`, filename a `scale=0.48f`, barra progress fill `CLR_GREEN` (semantico: azione positiva), footer "Do not power off the console." a `scale=0.48f`. Stessa struttura di `drawLoadingScreen`.

---



### Documentation
- **Single-file docs**: merged `USER_GUIDE.md` and `docs/README.md` into the root `README.md` — the repository now has one canonical reference for both users and developers.
- **Task-oriented structure**: README reordered as Quick Start → Features → How to Use (per flow) → Controls Reference → Screen Layout → Tips & Tricks → FAQ & Troubleshooting → Safety → Backup Structure → Configuration → Building from Source → Technical Reference.
- **Content gaps filled**: added `[*]` backup indicator description, Tips & Tricks section, expanded FAQ/troubleshooting entries ("Scroll feels slow", "Something deleted but still in HOME menu", "Can't find a title", loading bar note, install/restore failure causes) and full Technical Reference (architecture, data structures, rendering pipeline, language fallback, sort/filter internals, memory budget, UI layout constants, known limitations).
- **Removed obsolete files**: `USER_GUIDE.md`, `docs/README.md`, `RELEASE_NOTES_v1.0.1.md`, `implementation_guide.md`, `plan-3dsFastUninstallV2.prompt.md`, `BUILD_INFO.txt` — all content either absorbed into README or no longer relevant.
- **Fixed garbage prefixes**: removed stray `es` prefix at the top of `README.md` and `i ` prefix from `USER_GUIDE.md` (now deleted).
- **AGENTS.md updated**: references to deleted files replaced with single `README.md` pointer.

---

## v1.0.8 – Uniform list margins (2026-04-10)

### UNINSTALL / BACKUP / SYSINFO sublist — equal top and bottom margins
- All three list screens now use the same layout formula: **28px header, list starts at y=32, 13 visible rows, 14.5px row spacing**.
  - Top margin (header bottom → first row): 4px on all screens.
  - Bottom margin (last row text bottom → hint bar at y=222): 4px on all screens.
- **UNINSTALL**: header rect expanded from 22px to 28px; info bar moved from y=24 (outside header) to y=16 inside the header rect (scale 0.40f, 6.0px/char); title repositioned to y=2 at scale 0.44f. List start moved from y=38 to y=32 — gains a visible top margin for the first time.
- **BACKUP**: header rect expanded from 22px to 28px; header text moved to y=6; list start moved from y=38 to y=32; `BACKUP_VISIBLE` restored to 13 (was 12 in v1.0.7). This eliminates the extra blank space that was visible above and below the list compared to the other screens.
- **SYSINFO sublist**: header rect expanded from 22px to 28px; header text and sort indicator moved to y=6; list start moved from y=28 to y=32.
- Net result: same number of visible titles (13) and identical visual spacing on all three screens.

---

## v1.0.7 – Bug fixes & UI consistency (2026-04-10)

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

## v1.0.6 – UI/UX polish (2026-04-10)

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

## v1.0.4 - UI consistency pass (2026-04-10)

### Uniform look & feel across all screens
- **Main menu descriptions**: all rows are now complete, self-contained sentences (no more continuations from line 1 to line 2).
- **Install — header path**: path moved inside the header bar (height 36px); prevents visual bleed onto the first highlighted row of the file browser.
- **Install — type badge**: `^`/`+` moved from x=206 to x=316 (close to the size column), giving more room to the filename.
- **Backup — type badge**: replaced "Upd"/"DLC" text (0.52f, x=340) with `^`/`+` (0.44f, x=220), identical to Uninstall and SysInfo. Name truncated to 28 chars (same as Uninstall).
- **Uninstall — info bar**: scale reduced from 0.52f to 0.44f; TitleID now right-aligned at x=300 (end of row), matching the Install layout.
- **SysInfo — bottom overview**: three lines rewritten as complete sentences: "A = Open category list." / "UP/DOWN = Select category." / "B = Back to main menu."
- **Settings — Italian strings removed**: all descriptions translated to English; hint bar "Salva & Esci" → "Save & Back".

---

## v1.0.3 - Install UX polish (2026-04-10)

### UI improvements — Install file browser
- **Consistent layout with SysInfo**: CIA entries now use three separate draw calls: name truncated on the left (x=4), type badge (`^`/`+`) at x=206, size `[XXX MB]` right-aligned at `396 - len × 5.7px` (same formula as SysInfo).
- **Update/DLC type badge**: read from the TitleID cached in `FileEntry.titleID` at scan time — no additional file reads during render. `^` cyan = Update (`0x0004000E`), `+` green = DLC (`0x0004008C`).
- **Right/Left page navigation**: `KEY_RIGHT`/`KEY_LEFT` now jump by `MAX_VISIBLE_TITLES` positions in the file browser, matching the existing behaviour in Uninstall and Backup.
- **Bottom screen info**: uses cached `fe->titleID` instead of re-opening the file; shows a coloured "Type: Game/Update/DLC" line.
- **Controls hint updated**: `DX/SX=Page` in the bottom hint bar.

### Technical
- Added `u64 titleID` field to `FileEntry`; populated in `scanDirectory` pass 2 via `getCIATitleID()` — avoids the double-slash bug already present in the bottom-screen path (`currentPath + "/" + name`).

---

## v1.0.2 - Bug fixes & UX polish (2026-04-10)

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
