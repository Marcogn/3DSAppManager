i # User Guide

Quick guide to using 3DS Fast Uninstall.

---

## First Time Setup

1. Copy `3ds-fast-uninstall.3dsx` to `/3ds/` on your SD card.
2. Launch it from the Homebrew Launcher.
3. Wait for the loading screen (progress bar is shown).
4. You land on the **Main Menu** — pick a feature and get started.

---

## Main Menu

The app has five sections, each accessible with the **D-Pad** and **A**:

| Option | What it does |
|---|---|
| **Install CIA** | Browse the SD card and install `.CIA` files |
| **Backup Saves** | Back up save data for installed titles |
| **Uninstall Titles** | Remove titles with optional pre-uninstall backup |
| **System Info** | View your library stats and per-title details |
| **Settings** | Configure backup path and automation options |

Press **START** at any time to exit the application.

---

## Install CIA

### Navigating the File Browser

When you enter Install, you see a file browser starting at `sdmc:/`.

- **↑ / ↓** — move cursor one row
- **← / →** — jump one page
- **A** — enter a folder, or install the selected `.CIA` file
- **Y** — install **all** `.CIA` files in the current folder (batch install)
- **B** — go up one directory level

Directories are shown in cyan with a `[DIR]` prefix.  
`.CIA` files show their size right-aligned and a type badge if applicable:
- `^` (cyan) = Update
- `+` (green) = DLC

### Installing a Single File

1. Navigate to the `.CIA` file.
2. Press **A**.
3. Confirm the prompt (or skip if "Skip Install Confirm" is ON in Settings).
4. Wait for the progress bar to complete.

### Batch Installing a Folder

1. Enter the folder containing your `.CIA` files.
2. Press **Y**.
3. The app installs all files one by one and shows progress for each.

> All installs target the **SD card**. NAND installs are not supported.

---

## Backup Saves

### Understanding the List

Each row shows:
- `[ ]` — no backup exists
- `[*]` (green) — backup exists
- `[X]` (yellow) — title selected for batch backup

The bottom screen shows full name, Title ID, size, storage location, and last backup date for the cursor item.

### Backing Up

**Single title:**  
1. Navigate to the title.  
2. Press **A** to select it.  
3. Press **X** to back it up.

**Multiple titles:**  
1. Press **A** on each title to select them (marked `[X]`).  
2. Press **X** to back up all selected.

**Entire library:**  
Press **Y** to back up every title at once — no selection needed.

### What Gets Backed Up

For each title, the app saves up to three archive types:
- **savedata/** — main game saves
- **extdata/** — extended data (e.g. DLC content, extra data)
- **boss_extdata/** — SpotPass / StreetPass data

A `backup_info.txt` file with title name, version and date is included in each backup folder.

---

## Uninstall Titles

### Selecting Titles

1. Use **↑ / ↓** to navigate the list (or **← / →** to jump a full page).
2. Press **A** to toggle selection (checkbox turns yellow when selected).
3. Select as many titles as you want.
4. Press **X** to begin the uninstall flow.

Use **L / R** to cycle sort modes (Name → Size → ID) and **Y** to filter (All → Updates → DLC).

### The Uninstall Flow

**Step 1 — Backup?**  
- **A**: Back up all save data before deleting.
- **B**: Skip backup.
- **START**: Cancel the entire operation.

> If "Force Backup" is ON in Settings, this step is skipped and backup always runs.

**Step 2 — Backup location** *(only if you chose to back up)*  
- **A**: Use the default path from Settings.
- **Y**: Pick a different path from the preset list.

**Step 3 — Final confirmation**  
Review what will be deleted (titles list visible on the bottom screen).  
- **A**: Confirm and delete.
- **B**: Cancel.

> If "Skip Uninstall Confirm" is ON in Settings, the final confirmation is skipped.

**Step 4 — Deletion in progress**  
The app shows each title as it is being removed. The title list is refreshed automatically when done.

### Related Titles (DLC and Updates)

When you select a **base game**, the app automatically finds its Updates and DLC and offers to delete them together. You can confirm or skip for each.

---

## System Info

### Overview Screen

Shows your library at a glance:

```
  Games:      87    4.23 GB
> Updates:    43    980 MB      ← cursor
  DLC:        31    1.12 GB

  SD Free: 12.4 GB / 59.6 GB
```

Navigate the three categories with **↑ / ↓**.  
Press **A** to enter a category and see the full title list.

### Category List

Lists all titles in the selected category with name and size (right-aligned).  
Press **A** on any title to open its **detail page**.

### Title Detail Page

Shows:
- Full name, Title ID, version, size, storage location
- Backup date (if a backup exists)
- Related Updates and DLC (for base games)
- Three action options to select with **↑ / ↓** and execute with **A**:

| Action | What it does |
|---|---|
| **Backup Save Data** | Saves all archive types to the backup folder |
| **Restore Save Data** | Restores save data from the last backup |
| **Delete Title (+ related)** | Deletes the title, its DLC, Updates and all save data |

Press **B** to go back at any level.

---

## Settings

Navigate with **↑ / ↓**. Change a value with **A**, **L**, **R**, **←**, or **→**.

| Setting | Description |
|---|---|
| **Force Backup** | If ON, save data is backed up automatically before every uninstall — no prompt |
| **Skip Uninstall Confirm** | If ON, titles are deleted immediately on X without the confirmation dialog |
| **Force Restore** | If ON, save data is automatically restored after every successful CIA install |
| **Skip Install Confirm** | If ON, CIA files install immediately on A without the confirmation dialog |
| **Backup Folder** | Cycles through 5 preset backup paths |

The bottom screen shows a description of the currently highlighted setting, including the current backup folder path.

> Settings are saved in real time. Press **B** or **START** to return to the main menu.

---

## Tips & Tricks

### Free Up the Most Space

1. Go to **Uninstall Titles**.
2. Press **R** twice to sort by Size (largest first).
3. Select the titles you no longer need.
4. Press **X** and follow the prompts.

### Clean Up DLC and Updates Only

1. Press **Y** once for Updates, twice for DLC.
2. Select what you want to remove.
3. Press **X**.

### Check What Has a Backup

Open **Backup Saves**. Titles with `[*]` in green already have a backup.  
Titles with `[ ]` have never been backed up.

### Batch Backup Before a Big Cleanup

1. Go to **Backup Saves** and press **Y** to back up everything.
2. Then go to **Uninstall Titles** and delete freely.

### Restoring a Save After Reinstalling a Game

1. Reinstall the `.CIA` from **Install CIA**.
   - If "Force Restore" is ON in Settings, the backup is restored automatically.
   - Otherwise: go to **System Info**, find the title, press A → "Restore Save Data".

---

## Common Questions

**No titles showing up**  
The app only shows user-installed titles. System titles are hidden for safety.

**The title counter is red**  
You have over 300 titles. The HOME menu cap is 300. The app still shows all of them (up to 500), but consider cleaning up.

**Scroll feels slow**  
Single press = one row. Use ← / → for page jumps. The deliberate speed prevents accidental skips.

**Can't find a title**  
Try different sort modes (L / R) or use the filter (Y) to isolate Updates or DLC.

**Something deleted but still showing in HOME menu**  
Restart the HOME menu or the console. The 3DS system cache can take a moment to update.

---

## Troubleshooting

| Problem | Solution |
|---|---|
| App won't load | Make sure you're on a recent Homebrew Launcher and have CFW access |
| Loading bar stuck | Not stuck — it reads each title individually. 200+ titles can take ~30 seconds |
| Backup failed | Check that the SD card has enough free space and the backup folder is writable |
| Install failed | Verify the `.CIA` file is not corrupted; check free SD space |
| Restore failed | The backup folder may be missing or the title must be installed first |

---

For full technical documentation and architecture details, see [docs/README.md](docs/README.md).
