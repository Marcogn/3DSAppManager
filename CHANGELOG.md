# Changelog

## v1.0.3 - Install UX polish (2026-04-10)

### Miglioramenti UI — Install file browser
- **Layout uniforme con SysInfo**: le voci CIA ora usano tre draw call separate:
  nome troncato a sinistra (x=4), badge tipo (`^`/`+`) a x=206, dimensione
  `[XXX MB]` right-aligned a `396 - len × 5.7px` (stessa formula di SysInfo).
- **Badge tipo update/DLC**: letto dal TitleID cachato in `FileEntry.titleID` al
  momento della scan — nessuna lettura file aggiuntiva durante il render.
  `^` ciano = Update (`0x0004000E`), `+` verde = DLC (`0x0004008C`).
- **Navigazione DX/SX per pagina**: `KEY_RIGHT`/`KEY_LEFT` ora saltano di
  `MAX_VISIBLE_TITLES` posizioni nel file browser, come già in Uninstall e Backup.
- **Bottom screen info**: usa `fe->titleID` cachato invece di ri-aprire il file;
  mostra riga "Type: Game/Update/DLC" colorata.
- **Hint controlli aggiornato**: `DX/SX=Page` nella barra inferiore.

### Tecnico
- Aggiunto campo `u64 titleID` a `FileEntry`; popolato in `scanDirectory` pass 2
  tramite `getCIATitleID()` — evita la doppia-slash bug già presente nel path
  della bottom screen (`currentPath + "/" + name`).

---

## v1.0.2 - Bug fixes & UX polish (2026-04-10)

### Bug fix
- **Install: file .cia non trovati** — `scanDirectory()` usava `rewinddir()` che su
  CTRU/FAT32 non riposiziona il puntatore. La seconda passata (ricerca .cia) si
  eseguiva su un iteratore esaurito e restituiva zero file. Fix: due `opendir()`
  distinte, una per cartelle e una per i .cia, eliminano la dipendenza da
  `rewinddir()`. Risolto anche il double-slash (`sdmc:/ + / + name`) nelle chiamate
  `stat()` tramite macro `MKFP`.

### Miglioramenti UI
- **Backup: navigazione DX/SX per pagina** — `KEY_RIGHT`/`KEY_LEFT` ora avanzano/
  indietreggiano di `MAX_VISIBLE_TITLES` posizioni nel flusso Backup, identico al
  comportamento già presente in Uninstall.
- **Uninstall: TitleID completo nella lista** — mostrati tutti i 16 caratteri hex
  (`%016llX`) invece dei soli 32 bit bassi (`%08lX`). Scala ridotta a 0.40f per
  contenere il testo; simbolo tipo (^/+) spostato a x=220 per fare spazio.
- **SysInfo sublist: dimensione allineata a destra** — il campo `[size]` è ora
  disegnato con `x = 396 - strlen * 5.4px` così si allinea sempre al bordo destro
  indipendentemente dalla lunghezza del nome titolo.
- **Settings: descrizioni senza frasi spezzate** — le prime quattro voci avevano
  la descrizione tagliata a metà frase (es. "Auto backup before" / "uninstalling
  (no prompt)"). Riscritte come frasi complete e autonome su ogni riga.



Improved user interface clarity and screen management.

### Changes
- **Clear bottom screen during loading**: Bottom screen now shows a clean blue background while loading titles, instead of displaying the old selected titles list
- **Improved text clarity**: Changed "SELECTED TITLES" to "TITLES TO BE REMOVED" for better understanding
- **Better UX after uninstall**: When reloading titles after uninstalling, the bottom screen is properly cleared

## v1.0 - First Stable Release (2026-02-04)

Unified screen colors and production release.

### Changes
- **Unified screen colors**: Changed top screen background to match bottom screen (20, 20, 30)
- **Release binary**: Added compiled .3dsx to repository for easy distribution
- **Feature complete**: All planned features implemented and tested

This is the first production-ready stable release!

---

## PreAlpha Development Versions

The following versions were development iterations leading up to v1.0.

### PreAlpha 0.11.1 - SELECT Overlay Flickering Fix (2026-02-04)

Fixed the flickering issue when holding the SELECT button.

### Fix
- **SELECT overlay no longer flickers**: Completely rewrote the rendering logic for the controls overlay. Instead of calling `C2D_SceneBegin()` twice on the same target (which caused flickering), the overlay now:
  - Renders the complete UI in one pass when SELECT is held
  - Draws the overlay elements directly on top using higher z-depth values
  - Uses separate rendering path for overlay mode vs normal mode
  - Properly manages text buffer clearing to prevent text overlap

### Technical
- Eliminated double `C2D_SceneBegin()` calls
- Increased z-depth for overlay elements (0.6-0.7) to ensure proper layering
- Added state variable to track SELECT button state
- Fixed bottom screen overlay to render after touch controls

### PreAlpha 0.11 - Selected Titles List and UX Improvements (2026-02-04)

Added visual feedback during uninstall operations.

### Changes
- **Bottom screen shows selected titles during uninstall**: Press X and you'll see what you're about to delete on the bottom screen. Shows up to 10 titles, with "...and X more" if needed.
- **Info bar spacing fixed**: T:XXX / Sel:XXX / Sort:XXX now properly spaced at 5px / 135px / 265px.
- **Consistent feedback**: Selected titles list stays visible through all confirmation dialogs (backup, path selection, final confirm).

### Implementation
- `drawSelectedTitlesList()` - renders selected titles on bottom screen
- `drawDialogWithSelectedList()` - combines dialog with selected list
- Modified uninstall flow to use new dialog function

### PreAlpha 0.10.1 - SELECT Overlay and Symbol Fixes (2026-02-04)

Fixed flickering and duplicate symbols.

### Fixes
- **SELECT overlay flickering**: Was calling `C2D_SceneBegin(top)` twice per frame. Now uses simpler approach: always draw UI, then add overlay layer on top if SELECT held.
- **DLC double symbols**: Removed all symbol variants (↑, ⊕) from title names. Symbols now only appear in dedicated column.
- **Overlay sizing**: Box height 210px, spacing 13px for 8 control lines.

### Technical
- Simplified rendering: UI draws once, overlay is additive layer
- No text buffer conflicts
- Separate SceneBegin for overlay (after UI completes)

### PreAlpha 0.10 - Layout and Multi-language (2026-02-04)

Table layout and better name handling.

### Changes
- **Column-based layout**: Symbols moved to dedicated column (X=235), TitleID at X=255
- **Multi-language support**: Tries system language → English → Japanese → all 12 languages. No more "--" names.
- **Name validation**: Rejects empty/dash-only names, falls back to "Title [TitleID]"
- **Hold SELECT for controls**: Overlay shows while button held, hides on release

### Layout
```
[X] Title Name                  ⊕  TitleID
 3     28                      235   255
```

### Technical
- Symbol stripping with strstr() on all Unicode variants
- UTF-8 multi-byte character handling
- Sleep mode compatible rendering

### PreAlpha 0.9.2 - Japanese Support and Icon Refinement (2026-02-04)

Sorting, filtering, and 500 title support.

### Changes
- **Sort by size**: L/R cycles through Name/Size/TitleID. Size shows largest first.
- **Filter mode**: Y cycles through All/Updates/DLC
- **500 title support**: Bumped from 300 (HOME menu still caps at 300)
- **SELECT overlay**: Full-screen controls overlay

### Technical
- FilterMode enum with filteredIndices[] array
- 3 sort modes with comparator functions
- MAX_TITLES = 500

### PreAlpha 0.9 - Polish (2026-02-04)

### What's New
- **Title size display**: Now you can see how much space each title takes (in KB/MB/GB)
- **Better symbols**: Changed to simple ^ for updates and + for DLC (the fancy Unicode symbols looked like boxes on some 3DS models)
- **Smart truncation**: Long title names now show "..." so you know there's more
- **Cleaner controls**: Hit SELECT to see all the controls instead of having them clutter the screen
- **Better icon placeholder**: The "?" is actually centered now (yeah, it was bugging me too)

### Under the Hood
- Fixed some edge cases where title info wasn't loading properly
- Improved the getTitleInfo function to grab size data from the system

### PreAlpha 0.8 - The Stability Update (2026-02-04)

### Major Fixes
- **Sleep mode works now**: You can finally close your 3DS without the app crashing when you open it back up
- **Scroll control that actually works**: Completely rewrote how scrolling works. Now it's way more precise:
  - Single tap = move one item
  - Hold for 1.5 seconds = start slow continuous scroll
  - No more flying past the title you wanted!

### Technical Details
- Added proper aptMainLoop() handling for sleep/wake cycles
- Implemented frame-based scroll delay system with canScroll flag
- Continuous rendering for system event compatibility

### PreAlpha 0.7 - The Details Update (2026-02-04)

### Improvements
- **Longer names**: Title names now show up to 35 characters instead of 27
- **Slower scroll**: Made the scroll even slower because it was still too fast
- **Better alignment**: Fixed the bottom screen so everything lines up properly
- **Controls on multiple lines**: Split the control hints so they're not cut off

### Why These Changes?
The original scroll was way too sensitive - you'd tap up or down once and it would shoot across half the list. Now it's actually usable for precise navigation.

### PreAlpha 0.6 - The UI Overhaul (2026-02-04)

Complete redesign of the user interface. This was a big one.

### New Features
- **Progress bar**: Loading screen now shows actual progress instead of just sitting there
- **Smart counter**: Title count turns red when you hit 300+ (that's the HOME menu limit)
- **Better layout**: 
  - Title names take up more space
  - Title IDs are aligned to the right
  - Everything's more readable
- **Bigger text**: Increased sizes across the board
- **Better spacing**: More room between items

### The Why
The old UI was functional but cramped. This update makes everything easier to read and understand at a glance. Plus that loading screen sitting there doing nothing was annoying - now you know it's actually working.

---

## PreAlpha 0.1-0.5 - The Foundation

Early development versions focused on core functionality:

### Core Features Implemented
- **Multi-select system**: Getting checkbox selection working properly
- **Save backup system**: Implementing proper backups for all types:
  - Regular savedata
  - Extdata (extended data)
  - BOSS extdata (SpotPass data)
- **Title sorting**: Alphabetical and other sort modes
- **Character encoding**: UTF-16 to UTF-8 conversion for proper Japanese/special character display
- **Graphics migration**: Moved from console rendering to hardware-accelerated citro2d/citro3d to eliminate flickering
- **Safety features**: Filtering system titles to prevent accidental deletion of critical system components

### Technical Challenges Overcome
- **Character rendering**: Japanese and special characters were displaying as garbage. Implemented proper UTF-16 to UTF-8 conversion and sanitization.
- **Screen flickering**: Console rendering caused severe flickering. Migrated entire UI to citro2d hardware acceleration.
- **Save backup complexity**: 3DS has multiple save archive types, each requiring different handling through the FS service.
- **Title filtering**: Distinguishing user-installed titles from system titles without breaking functionality required extensive testing.

---

## Known Issues

None currently! If you find something, open an issue on GitHub.

## What's Next?

Some ideas for future versions (no promises on timeline):
- Automatic save restore functionality
- Title icon loading (currently just shows "?")
- Search/filter by name
- Export installed titles list to text file
- Batch operations improvements

Got suggestions? Let me know!
