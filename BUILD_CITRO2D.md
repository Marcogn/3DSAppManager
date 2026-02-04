# Build Instructions for Citro2D Migration

## Environment Setup

This project requires the devkitPro toolchain for Nintendo 3DS development.

### Install devkitPro

#### Linux/macOS:
```bash
# Install devkitPro from official package manager
wget https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman.amd64.deb
sudo dpkg -i devkitpro-pacman.amd64.deb

# Install 3DS development tools
sudo dkp-pacman -S 3ds-dev

# Set environment variables (add to ~/.bashrc or ~/.zshrc)
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export PATH=$DEVKITPRO/tools/bin:$PATH
```

#### Windows:
```powershell
# Download and install devkitPro installer from:
# https://github.com/devkitPro/installer/releases

# After installation, ensure these are set:
$env:DEVKITPRO = "C:\devkitPro"
$env:DEVKITARM = "$env:DEVKITPRO\devkitARM"
```

### Verify Installation

```bash
# Check that the compiler is available
arm-none-eabi-gcc --version

# Check that libraries are installed
ls $DEVKITPRO/portlibs/3ds/lib/
# Should show: libcitro2d.a, libcitro3d.a, libctru.a, etc.
```

## Building the Project

```bash
# Navigate to project directory
cd /path/to/3ds-fast-uninstall

# Clean previous builds
make clean

# Build
make

# Output will be: 3ds-fast-uninstall.3dsx and 3ds-fast-uninstall.smdh
```

## Testing

### On Citra Emulator (Recommended for initial testing):

```bash
# Install Citra
sudo apt install citra-qt  # Linux
# or download from https://citra-emu.org/

# Run the app
citra-qt 3ds-fast-uninstall.3dsx
```

### On Real Hardware:

1. Copy `3ds-fast-uninstall.3dsx` to your SD card's `/3ds/` folder
2. Launch from Homebrew Launcher
3. Test all features:
   - UI rendering without flicker
   - Navigation
   - Selection
   - Dialogs
   - Uninstall process (with caution!)

## Expected Build Output

```
arm-none-eabi-gcc version X.X.X ...
...
linking ... 3ds-fast-uninstall.elf
built ... 3ds-fast-uninstall.3dsx
```

## Troubleshooting

### Error: "3ds.h: No such file or directory"
- Solution: Install libctru: `sudo dkp-pacman -S libctru`
- Verify DEVKITARM is set correctly

### Error: "undefined reference to C2D_* or C3D_*"
- Solution: Install citro libraries: `sudo dkp-pacman -S citro3d citro2d`
- Verify Makefile has correct library order: `-lcitro2d -lcitro3d -lctru`

### Error: "Please set DEVKITARM in your environment"
- Solution: Export environment variables (see above)
- Restart terminal after setting

### Linker errors
- Solution: Run `make clean` then `make`
- Ensure all dependencies are installed

## Testing Checklist

After successful build, test the following:

### Visual Tests:
- [ ] No screen flickering during normal use
- [ ] Title list renders clearly
- [ ] Text is readable
- [ ] Selection highlight is visible
- [ ] Dialogs display properly

### Functional Tests:
- [ ] Navigation (D-Pad Up/Down)
- [ ] Fast scroll (D-Pad Left/Right)
- [ ] Title selection (A button)
- [ ] Sort by name (L button)
- [ ] Sort by ID (R button)
- [ ] Uninstall dialog appears
- [ ] Backup path selection works
- [ ] Progress display works
- [ ] App exits cleanly (START button)

### Performance Tests:
- [ ] UI responds immediately to input
- [ ] No lag when scrolling
- [ ] Smooth rendering (60 FPS)
- [ ] No memory leaks during extended use

## Development Notes

### Key Changes from Console to Citro2D:

1. **Frame Management**: Each screen update is wrapped in `C3D_FrameBegin` / `C3D_FrameEnd`
2. **Text Rendering**: Uses text buffers and `C2D_DrawText` instead of `printf`
3. **Colors**: Uses RGBA format via `C2D_Color32(r, g, b, a)`
4. **Coordinates**: Y-axis grows downward, origin at top-left
5. **Text Scale**: 0.5 is roughly equivalent to console text size

### Performance Tips:

- Clear and reuse text buffers instead of creating new ones
- Use `needsRedraw` flag to avoid unnecessary rendering
- Optimize text once before drawing multiple times
- Batch drawing operations within same frame

## Next Steps

After successful build and basic testing:

1. Test on real 3DS hardware
2. Compare with old console-based version
3. Take screenshots for documentation
4. Verify no flickering issues remain
5. Check memory usage is acceptable
6. Test with large title lists (100+ titles)
7. Verify all dialogs work correctly
