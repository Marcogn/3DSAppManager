<p align="center">
  <img src="icon.png" alt="3DS Fast Uninstall Icon" width="200"/>
</p>

# 3DS Fast Uninstall

A Nintendo 3DS homebrew application for quickly uninstalling multiple titles with optional save data backup.

**🎯 Perfect for managing your 3DS storage!** Easily remove multiple games and apps at once while safely backing up your save data. Ideal for freeing up space on your SD card or NAND.

## Screenshots

![Screenshot placeholder - Main interface showing title list]
*Coming soon: Screenshots of the main interface*

![Screenshot placeholder - Backup confirmation]
*Coming soon: Screenshots of the backup and deletion process*

## Features

- 📋 **Display all installed titles** - Shows a complete list of all user-installed games and applications with their title IDs
- ✅ **Multi-selection** - Select multiple titles at once for batch uninstallation
- 💾 **Complete save data backup** - Full backup of all save types before uninstalling:
  - User Save Data (main game saves)
  - ExtData (extended data, DLC save data)
  - Boss ExtData (SpotPass/StreetPass data)
- 🗑️ **Complete uninstallation** - Removes all data associated with titles:
  - Title application
  - Save data
  - ExtData
  - Boss ExtData
- ⚙️ **Configurable** - Customize backup location via config file or choose at runtime
- 📁 **Flexible backup paths** - Choose from predefined paths or use config default during uninstallation
- 🎮 **Easy to use** - Simple interface with D-pad and button controls
- 🛡️ **Safe** - Only uninstalls selected titles, system titles are protected
- 🔍 **Title ID display** - Each title shows its 16-digit hexadecimal ID for easy identification

## Installation

1. Download the latest release `.3dsx` file
2. Copy `3ds-fast-uninstall.3dsx` to your SD card's `/3ds/` folder
3. Launch from the Homebrew Launcher

## Usage

### Controls

- **D-Pad Up/Down**: Navigate through the title list
- **A Button**: Toggle selection (mark/unmark a title)
- **X Button**: Uninstall selected titles
- **START**: Exit application

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
- **Services Used**: 
  - AM (Application Manager) - Title management and deletion
  - FS (File System) - Save data access and backup
- **Target**: Nintendo 3DS (Old 3DS, New 3DS, 2DS)
- **Save Types Supported**:
  - ARCHIVE_USER_SAVEDATA (main game saves)
  - ARCHIVE_EXTDATA (extended data)
  - ARCHIVE_BOSS_EXTDATA (SpotPass data)

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

### Q: The app seems slow when loading titles
**A**: This is normal if you have many titles installed. The app needs to read information for each title to display its name.

### Q: Can I use this on a stock 3DS (no CFW)?
**A**: No, you need a 3DS with access to the Homebrew Launcher. This typically requires either:
- Custom Firmware (CFW) like Luma3DS
- Or a homebrew entrypoint (limited access)
Note: Some features may require CFW for full functionality.

## Technical Documentation

For detailed technical information, build verification, and compatibility analysis, see the [docs/](docs/) folder:

- **[Build Verification](docs/BUILD_VERIFICATION.md)** - Detailed build and compatibility report
- **[Compatibility Confirmation](docs/COMPATIBILITY_CONFIRMED.md)** - Complete 3DS compatibility analysis
- **[Final Verification](docs/FINAL_VERIFICATION.md)** - Final checklist and verification
- **[Build Log](docs/build.log)** - Latest successful compilation log

## License

This project is open source. Feel free to use, modify, and distribute.

## Credits

Created by Marcogn

## Disclaimer

Use this software at your own risk. While the application now includes full save data backup functionality for all save types, always ensure you have additional backups of important save data before uninstalling titles. The developers are not responsible for any data loss.