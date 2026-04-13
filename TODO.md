# TODO

## Future Features

### Controls Help Overlay (SELECT button)
**Status**: Removed in v2.1.0 refactoring  
**Rationale**: The SELECT overlay in Uninstall screen replaced the entire list draw, causing context loss. Footer became too crowded with all button hints.

**Proposed solution**:
- **Universal help system**: pressing SELECT on any screen displays a context-aware help overlay
- **Per-screen help content**: each screen (Main Menu, Install, Backup, Uninstall, SysInfo, Settings) has its own controls reminder and tips
- **Simplified footer**: main footer shows only `B: Back  START: Exit  SELECT: Help` on most screens
- **Overlay design**: semi-transparent box (like current dialogs) with:
  - Screen title at top
  - List of available controls with descriptions
  - Context hints (e.g., "Selected titles shown on bottom screen")
  - Footer: "Release SELECT to return"

**Implementation notes**:
- Add `showingHelpOverlay` global flag
- In main loop, if `hidKeysHeld() & KEY_SELECT`, set flag and call `drawHelpOverlay(appState)` instead of normal draw
- Each screen gets a help content array in `draw.c`
- No frame management in `drawHelpOverlay` — called from within existing `C3D_FrameBegin/End`

**Benefits**:
- Cleaner UI (less cluttered footers)
- Better discoverability for new users
- Consistent help access across all screens
- No context loss (overlay instead of replacement)

---

## Code Quality

### Potential optimizations
- [ ] Evaluate caching SMDH data to avoid re-reading on every `getTitleInfo` call (currently optimized in `loadTitles` only)
- [ ] Consider async title loading with progress updates every N titles instead of every 10

### Known technical debt
- `sysInfoDetailIdx` and `sysInfoDetailCursor` moved to globals — could be encapsulated in a struct
- Error handling for file I/O is minimal (most functions silently fail)

---

## Documentation
- [ ] Add architecture diagram to README showing module relationships
- [ ] Document the rendering pipeline (when C3D frames are managed vs. when they aren't)

