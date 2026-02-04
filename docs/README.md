# Technical Documentation

This folder contains technical documentation for developers.

## Architecture

### Core Components

- **Title Management**: Uses 3DS AM (Application Manager) service to enumerate and manage installed titles
- **Rendering**: Hardware-accelerated graphics using citro2d/citro3d
- **Save Backup**: Full save data backup including savedata, extdata, and boss extdata

### Key Technical Decisions

**Why citro2d?**
- Hardware acceleration eliminates flickering
- Better performance than console rendering
- Native support for UTF-8/UTF-16 text rendering

**Why multi-language support?**
- SMDH files contain 12 language variants
- System language preference → English fallback → Japanese fallback → any available
- Prevents empty title names

**Why 500 title limit?**
- HOME menu displays max 300 titles
- App supports 500 to handle edge cases
- Memory footprint: ~500KB for title array

### Data Structures

```c
typedef struct {
    u64 titleID;              // Unique title identifier
    char name[256];           // Display name (may include symbols)
    char fullName[256];       // Full name without truncation
    FS_MediaType mediaType;   // SD or NAND
    bool selected;            // Selection state
    u64 size;                 // Size in bytes
    u16 version;             // Title version
    bool hasBackup;          // Backup exists flag
} TitleInfo;
```

### Rendering Pipeline

1. **Frame Begin**: `C3D_FrameBegin(C3D_FRAME_SYNCDRAW)`
2. **Conditional Rendering**:
   - If SELECT held: Draw UI + overlay in single pass
   - If SELECT not held: Draw normal UI
3. **Frame End**: `C3D_FrameEnd(0)`

**Overlay Rendering** (when SELECT held):
- Renders complete UI first (header, title list, stats)
- Then draws overlay elements on top with higher z-depth (0.6-0.7)
- Uses single `C2D_SceneBegin()` per target (no double calls)
- Text buffer cleared once before overlay rendering
- Bottom screen shows touch controls + darkening overlay

**Critical Fix**: Previous versions called `C2D_SceneBegin()` twice on the same target, causing flickering. Current implementation:
- SELECT held: Custom rendering path with overlay baked in
- SELECT not held: Standard `drawUI()` + `drawTouchControls()`

**Bottom Screen Modes**:
- Normal: Title details (drawTouchControls)
- Dialog: Selected titles list (drawSelectedTitlesList)
- Overlay: Touch controls + darkening layer

**Important**: Render every frame for sleep mode compatibility.

### Input Handling

- **Scroll control**: Delay system prevents accidental fast scrolling
- **Filter system**: `filteredIndices[]` array for efficient filtering
- **Sort modes**: Name (alphabetical), Size (descending), TitleID (numerical)

### Save Backup Strategy

Three types of save data backed up:
1. **User Save Data** (`ARCHIVE_USER_SAVEDATA`)
2. **ExtData** (`ARCHIVE_EXTDATA`) - DLC, extra content
3. **Boss ExtData** (`ARCHIVE_BOSS_EXTDATA`) - SpotPass/StreetPass

Backup path structure:
```
/3ds/fast-uninstall/backups/[TitleID]/
├── backup_info.txt
├── savedata/
├── extdata/
└── boss_extdata/
```

### UTF-16 to UTF-8 Conversion

SMDH files store text as UTF-16. Conversion process:
1. Try system language
2. Fallback to English (index 1)
3. Fallback to Japanese (index 0)
4. Try all 12 languages
5. Validate: not empty, not only "--" or spaces
6. If all fail: use TitleID as name

### Sleep Mode Support

The 3DS requires continuous rendering for proper sleep/wake:
- `aptMainLoop()` returns false during sleep
- Rendering continues but paused
- No frame skipping allowed

## Building

```bash
make clean
make
```

Requires: devkitARM, libctru, citro2d, citro3d

## Memory Management

- **Dynamic text buffer**: 4096 bytes (reused every frame)
- **Title array**: Static allocation (MAX_TITLES * sizeof(TitleInfo))
- **Filtered indices**: Static int array for O(1) lookup

## Performance

- **Title loading**: ~0.1s per title (SMDH read)
- **Rendering**: 60 FPS target
- **Memory usage**: ~2MB total

## UI Components

### Top Screen Layout
```
T:300  Sel:5  Sort:Name    <- Info bar (evenly spaced: 5px, 135px, 265px)
────────────────────────────
[ ] Title Name         ↑  TitleID
 ^      ^              ^      ^
 3px   28px          235px  255px
```

### Bottom Screen Modes

**Normal Mode** (`drawTouchControls()`):
- Shows details of currently selected title
- Full name, TitleID, version, size, type, location
- Backup status and path

**Dialog Mode** (`drawSelectedTitlesList()`):
- Shows list of selected titles during uninstall
- Max 10 visible + "...and X more"
- Clean names with type symbols
- Header: "SELECTED TITLES (X)" in red

### Dialog System

**`drawDialog()`**: Standard top-screen-only dialog
**`drawDialogWithSelectedList()`**: Dialog + selected list combo
- Used for uninstall confirmation dialogs
- Top: Dialog text
- Bottom: Selected titles list

## Known Limitations

- Icon loading disabled (memory management complexity)
- Max file size for backup: 100MB per file
- Title name truncation: 30 chars on top screen, full on bottom

## Debugging

Enable via compile flags if needed:
- Add `-DDEBUG` to CFLAGS for debug output
- Use `consoleDemoInit()` for console debugging

---

For user documentation, see the main [README.md](../README.md) in the root directory.
