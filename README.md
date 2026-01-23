# 3DS Fast Uninstall

A Nintendo 3DS homebrew application for quickly uninstalling multiple titles with optional save data backup.

## Features

- 📋 **Display all installed titles** - Shows a complete list of all user-installed games and applications with their title IDs
- ✅ **Multi-selection** - Select multiple titles at once for batch uninstallation
- 💾 **Save data backup** - Optional backup marker creation before uninstalling (with user confirmation)
  - ⚠️ **Note**: Full save data backup is not yet fully implemented. For complete save backups, please use [JKSM](https://github.com/J-D-K/JKSM) or [Checkpoint](https://github.com/FlagBrew/Checkpoint) before uninstalling.
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
   - Press **A** to create backup markers (recommended for tracking)
   - Press **B** to skip backup
   - Press **START** to cancel
   - ⚠️ **Important**: Currently only creates backup markers, not full save data. Use JKSM or Checkpoint for complete backups.
5. Press **A** to confirm the final deletion

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
- **Services Used**: AM (Application Manager), FS (File System)
- **Target**: Nintendo 3DS (Old 3DS, New 3DS, 2DS)

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

Use this software at your own risk. Always ensure you have backups of important save data before uninstalling titles. The developers are not responsible for any data loss.