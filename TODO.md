# TODO

## Future Features

### Controls Help Overlay (SELECT button)
**Status**: ✅ Implemented in v2.3.0  
**Summary**: Universal per-screen help overlay triggered by holding SELECT. Semi-transparent box drawn at z=0.7 on the already-active top scene — no `C2D_SceneBegin` duplication, no flicker. Each screen (Main Menu, Install, Backup, Uninstall, SysInfo, Settings) has distinct controls + context hint. All screen footers simplified to `B: Back  START: Exit  SELECT: Help`.

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

