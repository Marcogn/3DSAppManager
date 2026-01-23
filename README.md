# 3DS Fast Uninstall

A Nintendo 3DS homebrew application for quickly uninstalling multiple titles with optional save data backup.

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
- ⚙️ **Configurable** - Customize backup location via config file
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
5. Press **A** to confirm the final deletion

The application will completely remove all data associated with selected titles, including the title itself, save data, ExtData, and Boss ExtData.

### Configuration

The application creates a configuration file at:
```
sdmc:/3ds/fast-uninstall/config.ini
```

You can edit this file to customize settings:

```ini
# Path where save backups will be stored
backup_path=sdmc:/3ds/fast-uninstall/backups
```

Change the `backup_path` to any location on your SD card where you want save backups to be stored.

## Building from Source

### Prerequisites

- [devkitARM](https://devkitpro.org/wiki/Getting_Started)
- libctru

### Build Steps

```bash
make
```

The compiled `.3dsx` file will be created in the project root directory.

### Clean Build

```bash
make clean
make
```

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

## License

This project is open source. Feel free to use, modify, and distribute.

## Credits

Created by Marcogn

## Disclaimer

Use this software at your own risk. While the application now includes full save data backup functionality for all save types, always ensure you have additional backups of important save data before uninstalling titles. The developers are not responsible for any data loss.