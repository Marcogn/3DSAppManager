# Changelog

## v2.4 - The Feature Complete Update (2026-02-04)

This is the big one - everything you asked for and more.

### What's New
- **Sort by size**: Press L/R to cycle through Name/Size/TitleID sorting. Perfect for finding those space hogs.
- **Filter mode**: Press Y to filter by All/Updates/DLC. Makes it easy to clean up specific types of content.
- **Controls overlay**: Press SELECT for a proper overlay with all controls. No more squinting at tiny text or dealing with cut-off messages.
- **500 title support**: Increased from 300 to 500. The HOME menu might cap at 300, but this app shows them all.
- **Unicode symbols back**: Updates get ↑ and DLC gets ⊕ because they're way more recognizable than ^ and +.

### How It Works
The filter system is smart - it builds a filtered list on the fly, so cursor and selection work exactly as you'd expect. When you switch filters with Y, it automatically adjusts. Same with sorting by size - the smaller titles go first, making it easy to scroll down to the whales.

### Why These Changes?
- **Sort by size**: Most requested feature. You want to free up space, you need to see what's taking it.
- **Filter mode**: When you have 200+ titles, finding all the updates or DLC manually is painful.
- **Controls overlay**: The old dialog was getting text cut off at the bottom. The overlay looks better and fits everything.
- **500 titles**: Some people actually hit the 300 limit. Now you won't.

### Technical Stuff
- New `FilterMode` enum with filtering logic
- `filteredIndices[]` array maintains which titles are visible
- L/R now cycle through 3 sort modes instead of toggle
- SELECT opens a full-screen overlay with proper darkening on both screens
- MAX_TITLES bumped to 500 (memory not an issue on 3DS for this)

## v2.3 - The Polish Update (2026-02-04)

Big update focused on making everything smoother and more informative.

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
