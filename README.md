<p align="center">
  <img src="icon.png" alt="3DS Fast Uninstall Icon" width="200"/>
</p>

# 3DS Fast Uninstall

A Nintendo 3DS homebrew application for quickly uninstalling multiple titles with optional save data backup.

**🎯 Perfect for managing your 3DS storage!** Easily remove multiple games and apps at once while safely backing up your save data.

## Features

- 📋 **Display all installed titles** - Shows up to 500 titles (HOME menu limit is 300)
- ✅ **Multi-selection** - Select multiple titles for batch uninstallation
- 🔄 **Triple sort modes** - Sort by Name, Size (largest first), or Title ID (cycle with L/R)
- 🎯 **Smart filtering** - Filter by All/Updates/DLC with Y button
- 💾 **Complete save backup** - Backs up all save data before uninstalling
- 🗑️ **Complete removal** - Removes title, saves, ExtData, and Boss ExtData
- ⚙️ **Configurable** - Customize backup location
- 🎮 **Optimized UI** - Smooth updates, no flickering, dual-screen layout
- 🛡️ **Safe** - System titles are protected
- 🎨 **Clear indicators** - Unicode symbols (↑ for Updates, ⊕ for DLC) in separate column
- 📊 **Detailed info panel** - Bottom screen shows title details with size, version, and backup status
- 🌙 **Sleep mode support** - No crashes when closing/opening the 3DS
- ⏱️ **Loading progress** - Visual progress bar when loading titles
- 🚨 **Smart warnings** - Red counter if you have over 300 titles (HOME menu limit)
- 📏 **Table layout** - Clean columnar display: checkbox | title name | type symbol | TitleID
- 🎯 **Precise scrolling** - Slow, controlled navigation without accidental jumps
- 💡 **Hold SELECT** - Hold SELECT button to see controls, release to hide
- 🌐 **Multi-language** - Japanese, English, and all SMDH languages supported

## Installation

1. Download `3ds-fast-uninstall.3dsx`
2. Copy to your SD card's `/3ds/` folder
3. Launch from Homebrew Launcher

## Controls

**Top Screen:**
- **D-Pad ↑/↓**: Navigate list (precise, one item at a time)
- **D-Pad ←→**: Fast scroll (page up/down)
- **A**: Toggle selection
- **X**: Uninstall selected
- **L/R**: Cycle sort mode (Name ↔ Size ↔ Title ID)
  - Name: Alphabetical
  - Size: Largest first
  - TitleID: Numerical
- **Y**: Cycle filter (All → Updates → DLC)
- **SELECT (hold)**: Show controls overlay (release to hide)
- **START**: Exit

**Top Screen Layout:**
```
[ ] Title Name                      ↑  0004000000012345
 ^      ^                           ^         ^
checkbox|                         symbol   TitleID
       name (no type indicator)
```

**Bottom Screen:**
Shows detailed information about the currently selected title:
- Full title name (no truncation)
- Title ID (hexadecimal)
- Version number
- Size (in KB/MB/GB)
- Type (Game/Application, Update (↑), or DLC (⊕))
- Storage location (SD Card / NAND)
- Backup status (YES ✓ if backup exists, NO ✗ if not)
- Backup path (if backup exists)
- Reminder: "Press SELECT for controls"

### Uninstalling Titles

1. Navigate to the titles you want to uninstall using the D-Pad
2. Press **A** to select each title (a checkbox will appear)
3. Press **X** to begin the uninstallation process
4. Choose whether to backup save data:
   - Press **A** to backup all save data types (User Save, ExtData, Boss ExtData)
   - Press **B** to skip backup
   - Press **START** to cancel
5. If you chose to backup, select the backup location:
   - Press **A** to use the default path from config
   - Press **Y** to choose from alternative paths
   - Use **D-Pad** to navigate alternative paths, **A** to confirm
6. Review the final confirmation showing titles, backup option, and backup path
7. Press **A** to confirm the deletion

The application will completely remove all data associated with selected titles, including the title itself, save data, ExtData, and Boss ExtData.

### Configuration

The application creates a configuration file at:
```
sdmc:/3ds/fast-uninstall/config.ini
```

You can edit this file to customize the default backup path:

```ini
# Path where save backups will be stored by default
backup_path=sdmc:/3ds/fast-uninstall/backups
```

**Note**: You can also choose the backup path at runtime when performing an uninstallation. The config file sets the default that is shown first, but you can select from alternative paths during the backup process.

## Building from Source

### Prerequisites

You need to have devkitPro and the 3DS development tools installed on your system.

#### Install devkitPro (All Platforms)

Follow the official guide at: https://devkitpro.org/wiki/Getting_Started

#### Install 3DS Development Tools

After installing devkitPro, install the required libraries:

```bash
# On Linux/macOS:
sudo dkp-pacman -S 3ds-dev

# This installs:
# - devkitARM compiler
# - libctru (3DS system library)
# - citro3d/citro2d (graphics libraries)
# - 3dstools (3dsxtool, smdhtool, etc.)
# - All other required dependencies
```

#### Environment Variables

Make sure these environment variables are set:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
```

Add these to your `~/.bashrc` or `~/.zshrc` to make them permanent.

### Build Steps

```bash
# Clone the repository (if not already done)
git clone https://github.com/yourusername/3ds-fast-uninstall.git
cd 3ds-fast-uninstall

# Build the project
make
```

The compiled `.3dsx` and `.smdh` files will be created in the project root directory.

### Clean Build

```bash
make clean
make
```

### Troubleshooting Build Issues

**Error: "3ds.h: No such file or directory"**
- Make sure libctru is installed: `sudo dkp-pacman -S libctru`
- Verify DEVKITPRO and DEVKITARM environment variables are set

**Error: "Please set DEVKITARM in your environment"**
- Set the environment variables as shown above
- Restart your terminal after setting them

**Linker errors**
- Run `make clean` and try again
- Ensure all dependencies are installed: `sudo dkp-pacman -S 3ds-dev`

## Technical Details

- **Language**: C
- **SDK**: libctru (devkitARM)
- **Graphics**: citro3d + citro2d (hardware-accelerated rendering)
- **Services Used**: 
  - AM (Application Manager) - Title management and deletion
  - FS (File System) - Save data access and backup
- **Target**: Nintendo 3DS (Old 3DS, New 3DS, 2DS)
- **Save Types Supported**:
  - ARCHIVE_USER_SAVEDATA (main game saves)
  - ARCHIVE_EXTDATA (extended data)
  - ARCHIVE_BOSS_EXTDATA (SpotPass data)

### Graphics System

The application uses **citro3d** and **citro2d** for hardware-accelerated graphics rendering:
- **No flickering** - Smooth, synchronized frame updates
- **Efficient rendering** - Only redraws when needed
- **Double buffering** - PICA200 GPU handles all graphics processing
- **Text rendering** - Dynamic text buffers for optimal performance

### How Complete Deletion Works

When you delete a title, the application:
1. Backs up all save data types (if selected)
2. Deletes ExtData archives
3. Deletes Boss ExtData (SpotPass data)
4. Calls AM_DeleteTitle to remove the title and remaining data
5. Verifies deletion was successful

## Safety Features

- System titles are automatically filtered out and cannot be selected
- Only user-installed applications and updates are shown
- Requires explicit confirmation before deletion
- Protected title ranges (0x00040010, 0x00040030, 0x00040138) are excluded

## FAQ / Troubleshooting

### Q: The application doesn't show any titles
**A**: Make sure you have titles installed on your 3DS. The application only shows user-installed titles (games, apps). System titles are filtered out for safety.

### Q: The title counter is red - what does that mean?
**A**: If you have over 300 titles installed, the counter turns red. This is a warning because the 3DS HOME menu can only display up to 300 titles properly. Consider deleting some unused titles.

### Q: Can the app handle more than 300 titles?
**A**: Yes! The app supports up to 500 titles. The 300 limit is only for the HOME menu display, not for this app. You'll see all your titles here.

### Q: How do I find the largest titles to delete?
**A**: Press **R** twice to sort by Size. The largest titles appear at the top of the list.

### Q: How do I see only Updates or only DLC?
**A**: Press **Y** to cycle through filters:
- All (shows everything)
- Updates (shows only updates - marked with ↑)
- DLC (shows only DLC - marked with ⊕)

### Q: Why do I see "..." at the end of some title names?
**A**: Long title names are truncated in the top screen list to fit the layout. You can see the full name in the bottom screen details panel.

### Q: The scroll is too slow/fast
**A**: The scroll has been carefully tuned to be precise. Single presses move one item, holding the button waits 1.5 seconds before starting continuous scroll (to prevent accidents). Use D-Pad Left/Right for faster navigation (page up/down).

### Q: Can I recover a title after deleting it?
**A**: Once deleted, the title cannot be recovered unless you:
- Have a backup of the save data (created by this app)
- Re-download/reinstall the title from the eShop or reinstall from the original source

### Q: Where are the backups stored?
**A**: By default, backups are stored in `sdmc:/3ds/fast-uninstall/backups/[TitleID]/`
Each backup includes:
- `backup_info.txt` - Information about the backup
- `savedata/` - User save data (if present)
- `extdata/` - Extended data (if present)
- `boss_extdata/` - SpotPass data (if present)

### Q: How do I restore a backup?
**A**: Currently, you need to:
1. Reinstall the title
2. Manually copy the backup files back to the appropriate location
Note: A future version may include automatic restore functionality.

### Q: Can I change the backup location?
**A**: Yes, in two ways:
1. Edit the config file at `sdmc:/3ds/fast-uninstall/config.ini`
2. Choose a different path at runtime when performing an uninstallation

### Q: Is it safe to delete DLC or updates?
**A**: Yes, but be aware:
- Deleting DLC will remove the DLC content
- Deleting updates will revert the game to its base version
- Base games and DLC are separate titles
- You'll see (↑) for updates and (⊕) for DLC in the title list

### Q: The app freezes when I close the 3DS lid
**A**: This has been fixed in v2.2+. The app now properly supports sleep mode - you can close and open the 3DS without issues.

### Q: Why does loading take a while?
**A**: The app now shows a progress bar while loading. The time depends on how many titles you have installed - each title needs to be read from the system to get its name, size, and details.

### Q: Can I use this on a stock 3DS (no CFW)?
**A**: No, you need a 3DS with access to the Homebrew Launcher. This typically requires either:
- Custom Firmware (CFW) like Luma3DS
- Or a homebrew entrypoint (limited access)
Note: Some features may require CFW for full functionality.

## Technical Documentation

For technical details about the implementation, see [docs/README.md](docs/README.md).

Topics covered:
- Architecture and core components
- Data structures
- Rendering pipeline
- UTF-16 to UTF-8 conversion
- Save backup strategy
- Sleep mode support
- Memory management
- Performance characteristics

## License

This project is open source. Feel free to use, modify, and distribute.

## Credits

Created by Marcogn

## Disclaimer

Use this software at your own risk. While the application now includes full save data backup functionality for all save types, always ensure you have additional backups of important save data before uninstalling titles. The developers are not responsible for any data loss.