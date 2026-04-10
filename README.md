 es<p align="center">
  <img src="icon.png" alt="3DS Fast Uninstall Icon" width="200"/>
</p>

# 3DS Fast Uninstall

A Nintendo 3DS homebrew application for managing installed titles: install CIA files, back up save data, uninstall titles in batch, and view system information — all from a single clean multi-screen interface.

**🎯 The complete toolkit for managing your 3DS storage.**

## Features

### Install
- Browse the SD card file system to find `.CIA` files
- Install single files or batch-install an entire folder in one step (**Y**)
- Detects Update (`^`) and DLC (`+`) types at scan time — no extra file reads
- Size shown right-aligned per row; type badge close to size column
- All installs always target the SD card

### Backup Saves
- List all installed titles with backup status at a glance
- Select individual titles or back up the entire library at once (**Y**)
- Three save archive types per title: User Save, ExtData, Boss ExtData
- Backup date shown in the bottom-screen detail panel
- Backups stored under `[backup_path]/[TitleID]/`

### Uninstall
- Multi-selection with **A**; batch uninstall with **X**
- Optional pre-uninstall backup (prompted automatically, or forced via Settings)
- Selected titles list visible on the bottom screen throughout the confirmation flow
- Automatically finds and offers to delete related DLC/Updates for base games
- Post-delete verification: slot is cleared only after AM confirms deletion

### System Info
- Overview: Games / Updates / DLC counts + total sizes + SD free space
- Navigate into each category (**A**) to browse the full title list
- Per-title detail page with:
  - Full name, Title ID, version, size, storage location, backup date
  - Related Updates and DLC listed for base games
  - Actions: **Backup Save Data**, **Restore Save Data**, **Delete Title**

### Settings

| Setting | Default | Description |
|---|---|---|
| Force Backup | OFF | Auto-backup before every uninstall (no prompt) |
| Skip Uninstall Confirm | OFF | Delete immediately on X without confirmation |
| Force Restore | OFF | Auto-restore backup after every CIA install |
| Skip Install Confirm | OFF | Install immediately on A without confirmation |
| Backup Folder | `sdmc:/3ds/fast-uninstall/backups` | Cycle through 5 preset paths with ←/→ or L/R |

Changes are saved in real time; press **B** or **START** to return to the menu.

### General
- Sort by **Name**, **Size** (largest first), or **Title ID**
- Filter by **All**, **Updates only**, or **DLC only**
- Supports up to **500 titles** (HOME menu cap is 300; counter turns red above it)
- Multi-language title names: system language → English → Japanese → any available
- Hold **SELECT** for a full controls overlay (flicker-free rendering)
- Sleep mode compatible — close and re-open the console freely
- Progress bar during title loading

---

## Installation

1. Download `3ds-fast-uninstall.3dsx`
2. Copy it to `/3ds/` on your SD card
3. Launch from the Homebrew Launcher

> **Requirements**: Nintendo 3DS / 2DS / New 3DS running Custom Firmware (e.g. Luma3DS) or a homebrew entrypoint with sufficient AM access.

---

## Controls

### Main Menu

| Button | Action |
|---|---|
| **↑ / ↓** | Navigate menu |
| **A** | Enter selected screen |
| **START** | Exit application |

### Uninstall Screen

| Button | Action |
|---|---|
| **↑ / ↓** | Move cursor one row |
| **← / →** | Jump one page |
| **A** | Toggle title selection |
| **X** | Start uninstall flow for selected titles |
| **L / R** | Cycle sort mode (Name → Size → ID) |
| **Y** | Cycle filter (All → Updates → DLC) |
| **SELECT** (hold) | Show controls overlay |
| **B** | Back to main menu |
| **START** | Exit application |

### Install Screen (File Browser)

| Button | Action |
|---|---|
| **↑ / ↓** | Move cursor |
| **← / →** | Jump one page |
| **A** | Enter folder / Install CIA file |
| **Y** | Install all CIA files in the current folder |
| **B** | Go up one folder level |

### Backup Screen

| Button | Action |
|---|---|
| **↑ / ↓** | Move cursor one row |
| **← / →** | Jump one page |
| **A** | Toggle title selection |
| **X** | Backup selected titles |
| **Y** | Backup all titles |
| **B** | Back to main menu |

### System Info Screen

| Button | Action |
|---|---|
| **↑ / ↓** | Navigate categories / title list |
| **A** | Open category list / open title detail |
| **B** | Back / back to overview |

### Title Detail (from System Info)

| Button | Action |
|---|---|
| **↑ / ↓** | Select action |
| **A** | Execute selected action |
| **B** | Back to category list |

**Available actions:**
- **Backup Save Data** — saves all archive types to the backup folder
- **Restore Save Data** — restores from the last backup
- **Delete Title (+ related)** — removes title, DLC, Updates and all associated data

### Settings Screen

| Button | Action |
|---|---|
| **↑ / ↓** | Navigate settings |
| **A / L / R / ← / →** | Change value |
| **B / START** | Save and return to menu |

---

## Screen Layout Reference

### Top Screen — Uninstall

```
┌─────────────────────────────────────────────────────┐
│ Uninstall                                           │  ← header
│ T:120  Sel:3  Sort:Name  [All]                      │  ← info bar
│ [ ] Super Mario 3D Land             0004000000...   │
│ [X] Pokemon Sun              ^      0004000000...   │  ← selected, Update
│ [ ] Mario Kart 7 DLC         +      0004008C00...   │  ← DLC
│ A=Sel  X=Delete  L/R=Sort  Y=Filt  B=Menu  SEL=Help │
└─────────────────────────────────────────────────────┘
```

**Symbols:** `^` = Update (cyan), `+` = DLC (green)  
**Checkbox:** `[ ]` unselected · `[X]` selected (yellow)

### Top Screen — Install (File Browser)

```
┌─────────────────────────────────────────────────────┐
│ Install CIA                                         │  ← header
│ sdmc:/roms/cias/                                    │  ← current path
│ [DIR] demos/                                        │
│ SuperMario.cia                        ^ [45.3 MB]   │
│ DLC_Pack.cia                          + [12.1 MB]   │
│ A=Enter/Install  Y=All  B=Up  DX/SX=Page            │
└─────────────────────────────────────────────────────┘
```

### Top Screen — System Info (Overview)

```
┌─────────────────────────────────────────────────────┐
│ SYSTEM INFORMATION                                  │
│                                                     │
│   Games:      87    4.23 GB                         │
│ > Updates:    43    980 MB                          │  ← cursor row
│   DLC:        31    1.12 GB                         │
│                                                     │
│   SD Free: 12.4 GB / 59.6 GB                        │
└─────────────────────────────────────────────────────┘
```

---

## Backup Structure

```
[backup_path]/
└── [TitleID]/            e.g. 0004000000033500/
    ├── backup_info.txt   metadata (date, version, name)
    ├── savedata/         main game saves
    ├── extdata/          extended data (DLC content)
    └── boss_extdata/     SpotPass / StreetPass data
```

**Default path:** `sdmc:/3ds/fast-uninstall/backups`

**Alternative paths** (cycle with ←/→ in Settings):
1. `sdmc:/3ds/fast-uninstall/backups`
2. `sdmc:/backups/3ds-titles`
3. `sdmc:/save-backups`
4. `sdmc:/3ds-backups`
5. `sdmc:/backups`

---

## Configuration

Config file: `sdmc:/3ds/fast-uninstall/config.ini`

```ini
backup_path=sdmc:/3ds/fast-uninstall/backups
force_backup=0
skip_uninstall_confirm=0
force_restore=0
skip_install_confirm=0
```

---

## Safety

- **System titles filtered** — ranges `0x00040010`, `0x00040030`, `0x00040138` are excluded
- **Confirmation dialogs** before every destructive action (unless disabled in Settings)
- **Post-delete verification** via `AM_GetTitleInfo` — title removed from list only after system confirms deletion
- **Backups are independent** — deleting a title does not delete its backup

---

## Building from Source

### Prerequisites

```bash
# Install devkitPro — see https://devkitpro.org/wiki/Getting_Started
sudo dkp-pacman -S 3ds-dev
```

Environment variables (add to `~/.bashrc` / `~/.zshrc`):

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
```

### Build

```bash
git clone https://github.com/yourusername/3ds-fast-uninstall.git
cd 3ds-fast-uninstall
make clean && make
```

Output: `3ds-fast-uninstall.3dsx`, `3ds-fast-uninstall.elf`, `3ds-fast-uninstall.smdh`

### Troubleshooting

| Error | Fix |
|---|---|
| `3ds.h: No such file or directory` | `sudo dkp-pacman -S libctru` |
| `Please set DEVKITARM in your environment` | Export `DEVKITARM` as above |
| Linker errors | `make clean && make` |

---

## Technical Details

- **Language**: C (single-file: `source/main.c`)
- **SDK**: libctru (devkitARM)
- **Graphics**: citro3d + citro2d (hardware-accelerated, flicker-free double buffering)
- **Services**: AM (title management), FS (file system / save archives)
- **Max titles**: 500 (`MAX_TITLES`)
- **Target**: Nintendo 3DS, New 3DS, 2DS (all variants)

See [docs/README.md](docs/README.md) for architecture and implementation details.

---

## FAQ

**Q: No titles showing up**  
A: The app only shows user-installed titles. System titles are filtered for safety.

**Q: Title counter is red**  
A: You have over 300 titles. The HOME menu cap is 300; the app supports up to 500.

**Q: How do I restore a backup?**  
A: Open System Info → navigate to the title → A → "Restore Save Data".

**Q: Where are my backups?**  
A: `[backup_path]/[TitleID]/`. Default: `sdmc:/3ds/fast-uninstall/backups/`. Change in Settings.

**Q: Is it safe to delete updates or DLC?**  
A: Yes. Each is an independent title. Deleting an update reverts the game to its base version.

**Q: Can I use this without CFW?**  
A: A homebrew entrypoint is required. Full title deletion requires AM access (CFW recommended).

---

## License

Open source. See [LICENSE](LICENSE).

## Credits

Created by Marcogn.

## Disclaimer

Use at your own risk. Always keep additional backups of important save data before uninstalling titles. The authors are not responsible for any data loss.
