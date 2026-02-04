# Changelog

## v2.6 - Selected Titles List and UX Improvements (2026-02-04)

Added visual feedback during uninstall operations.

### Changes
- **Bottom screen shows selected titles during uninstall**: Press X and you'll see what you're about to delete on the bottom screen. Shows up to 10 titles, with "...and X more" if needed.
- **Info bar spacing fixed**: T:XXX / Sel:XXX / Sort:XXX now properly spaced at 5px / 135px / 265px.
- **Consistent feedback**: Selected titles list stays visible through all confirmation dialogs (backup, path selection, final confirm).

### Implementation
- `drawSelectedTitlesList()` - renders selected titles on bottom screen
- `drawDialogWithSelectedList()` - combines dialog with selected list
- Modified uninstall flow to use new dialog function

## v2.5.1 - SELECT Overlay and Symbol Fixes (2026-02-04)

Fixed flickering and duplicate symbols.

### Fixes
- **SELECT overlay flickering**: Was calling `C2D_SceneBegin(top)` twice per frame. Now uses simpler approach: always draw UI, then add overlay layer on top if SELECT held.
- **DLC double symbols**: Removed all symbol variants (↑, ⊕) from title names. Symbols now only appear in dedicated column.
- **Overlay sizing**: Box height 210px, spacing 13px for 8 control lines.

### Technical
- Simplified rendering: UI draws once, overlay is additive layer
- No text buffer conflicts
- Separate SceneBegin for overlay (after UI completes)

## v2.5 - Layout and Multi-language (2026-02-04)

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

## v2.4 - Feature Complete (2026-02-04)

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

## v2.3 - Polish (2026-02-04)

### What's New
- **Title size display**: Now you can see how much space each title takes (in KB/MB/GB)
- **Better symbols**: Changed to simple ^ for updates and + for DLC (the fancy Unicode symbols looked like boxes on some 3DS models)
- **Smart truncation**: Long title names now show "..." so you know there's more
- **Cleaner controls**: Hit SELECT to see all the controls instead of having them clutter the screen
- **Better icon placeholder**: The "?" is actually centered now (yeah, it was bugging me too)

### Under the Hood
- Fixed some edge cases where title info wasn't loading properly
- Improved the getTitleInfo function to grab size data from the system

## v2.2 - The Stability Update (2026-02-04)

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

## v2.1 - The Details Update (2026-02-04)

### Improvements
- **Longer names**: Title names now show up to 35 characters instead of 27
- **Slower scroll**: Made the scroll even slower because it was still too fast
- **Better alignment**: Fixed the bottom screen so everything lines up properly
- **Controls on multiple lines**: Split the control hints so they're not cut off

### Why These Changes?
The original scroll was way too sensitive - you'd tap up or down once and it would shoot across half the list. Now it's actually usable for precise navigation.

## v2.0 - The UI Overhaul (2026-02-04)

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

## Earlier Versions

### v1.x - The Foundation

Initial releases focused on:
- Getting multi-select working
- Implementing proper save backups (all types: savedata, extdata, boss extdata)
- Sorting functionality
- Fixing character encoding issues (UTF-16 was a pain)
- Eliminating screen flickering with citro2d/citro3d
- Making sure system titles can't be deleted by accident

### What Was Hard
- **Character rendering**: Japanese/special characters were showing up as garbage. Had to implement proper UTF-16 to UTF-8 conversion and sanitization.
- **Flickering**: The screen was flickering like crazy. Ended up migrating from console rendering to hardware-accelerated citro2d.
- **Save backup**: 3DS has multiple save types (user saves, extdata, boss extdata) and they all need different handling.
- **Title detection**: Filtering out system titles without breaking anything took some trial and error.

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
