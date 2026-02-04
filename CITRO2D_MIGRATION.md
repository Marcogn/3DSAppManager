# Citro2D/Citro3D Migration Guide

## Overview

This document describes the migration from console-based GUI to Citro2D/Citro3D graphics library to fix the refresh/flickering issues.

## Problem Statement

The original implementation used console functions (`printf`, `consoleClear`) with `gfxFlushBuffers()` and `gfxSwapBuffers()` which caused:
- Screen flickering due to frequent full-screen clears
- Poor performance from synchronous text rendering
- Visual artifacts when updating the UI

## Solution

Migrated to Citro2D/Citro3D, which are the official 3DS graphics libraries providing:
- Hardware-accelerated 2D/3D rendering
- Double-buffered display without flickering
- Efficient text rendering with text buffers
- Proper frame synchronization

## Technical Changes

### 1. Dependencies Updated

**Makefile**:
```makefile
# Before
LIBS := -lctru -lm

# After
LIBS := -lcitro2d -lcitro3d -lctru -lm
```

### 2. Headers and Global Variables

**Before**:
```c
#include <3ds.h>
static PrintConsole topScreen, bottomScreen;
```

**After**:
```c
#include <3ds.h>
#include <citro2d.h>
static C3D_RenderTarget* top;
static C3D_RenderTarget* bottom;
static C2D_TextBuf staticBuf;
static C2D_TextBuf dynamicBuf;
```

### 3. Initialization

**Before**:
```c
gfxInitDefault();
consoleInit(GFX_TOP, &topScreen);
consoleInit(GFX_BOTTOM, &bottomScreen);
```

**After**:
```c
gfxInitDefault();
C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
C2D_Prepare();

top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

staticBuf = C2D_TextBufNew(4096);
dynamicBuf = C2D_TextBufNew(4096);
```

### 4. Rendering Pattern

**Before**:
```c
consoleClear();
printf("Text here\n");
gfxFlushBuffers();
gfxSwapBuffers();
```

**After**:
```c
C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
C2D_TargetClear(top, C2D_Color32(0, 0, 0, 255));
C2D_SceneBegin(top);

C2D_Text text;
C2D_TextParse(&text, dynamicBuf, "Text here");
C2D_TextOptimize(&text);
C2D_DrawText(&text, C2D_WithColor, x, y, 0.5f, scale, scale, color);

C3D_FrameEnd(0);
```

### 5. Cleanup

**Before**:
```c
gfxExit();
```

**After**:
```c
C2D_TextBufDelete(dynamicBuf);
C2D_TextBufDelete(staticBuf);
C2D_Fini();
C3D_Fini();
gfxExit();
```

## Key Functions Modified

### `drawUI()`
- Completely rewritten to use C2D drawing
- Uses `C2D_DrawRectSolid` for highlight backgrounds
- Uses `C2D_DrawText` for all text rendering
- No more console escape codes

### `drawTouchControls()`
- Renders to bottom screen target
- Uses proper text parsing and optimization
- Clean layout without console formatting issues

### `drawDialog()`
- New helper function for displaying dialogs
- Accepts array of text lines
- Renders with consistent formatting

### `handleInput()`
- Updated all dialog sections to use citro2d
- Interactive path selection now uses proper graphics
- Removed all `consoleClear()` and `printf()` calls in dialogs

## Building

### Prerequisites
```bash
# Install devkitPro and 3DS development tools
sudo dkp-pacman -S 3ds-dev

# This includes:
# - devkitARM compiler
# - libctru
# - citro2d
# - citro3d
# - 3dstools
```

### Build Command
```bash
make clean
make
```

## Benefits

1. **No Flickering**: Hardware-accelerated rendering eliminates screen flicker
2. **Better Performance**: GPU-based text and graphics rendering
3. **Cleaner Code**: No escape codes or printf formatting tricks
4. **More Flexible**: Easy to add colors, shapes, and graphics
5. **Standard Practice**: Uses the official 3DS graphics library

## Testing Checklist

- [ ] App launches without crashes
- [ ] Title list displays correctly on top screen
- [ ] Controls reminder shows on bottom screen
- [ ] Navigation (up/down/left/right) works smoothly
- [ ] Selection highlighting is visible and clear
- [ ] Dialogs display properly with citro2d
- [ ] No screen flickering during navigation
- [ ] Uninstall process shows progress correctly
- [ ] All text is readable and properly formatted

## Compatibility

- **3DS Models**: Old 3DS, New 3DS, 2DS (all models)
- **Firmware**: Any firmware with Homebrew Launcher access
- **CFW**: Works with Luma3DS and other CFW
- **Libraries**: Requires citro2d/citro3d from devkitPro

## Notes

- The `needsRedraw` flag is maintained for efficiency - only redraws when UI changes
- Text buffers are cleared and reused to minimize memory allocations
- Frame synchronization ensures smooth 60 FPS rendering
- All dialogs now use citro2d for consistency

## Migration Status

✅ **Completed**:
- Core rendering system
- Main UI (top screen)
- Controls display (bottom screen)
- All dialogs
- Build system

⏳ **To Verify**:
- Build on actual devkitPro environment
- Test on real 3DS hardware
- Screenshot comparison before/after
