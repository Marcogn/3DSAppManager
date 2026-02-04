# Changelog

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
- Search/filter functionality
- Export installed titles list to text file
- Batch operations improvements

Got suggestions? Let me know!
