# User Guide

Quick guide to using 3DS Fast Uninstall without getting lost.

## First Time Setup

1. Copy `3ds-fast-uninstall.3dsx` to your SD card's `/3ds/` folder
2. Launch it from Homebrew Launcher
3. Wait for it to load (you'll see a progress bar)
4. That's it, you're ready to go

## Basic Usage

### Just Looking Around

When you first open the app, you'll see:
- **Top screen**: List of all your installed titles
- **Bottom screen**: Details about the selected title

Use the D-Pad up/down to move through the list. The bottom screen updates automatically to show you what you're looking at.

### Selecting Titles to Delete

1. Use D-Pad to highlight a title
2. Press **A** to select it (you'll see a checkbox appear)
3. Keep selecting as many as you want
4. Press **X** when you're ready to delete

### The Deletion Process

When you press X, the app walks you through it:

**Step 1: Backup saves?**
- Press **A** if you want to backup your save data (recommended!)
- Press **B** if you're sure you don't need it
- Press **START** to cancel

**Step 2: Where to backup? (if you chose yes)**
- Press **A** to use the default location
- Press **Y** to pick from other locations
- If you picked Y, use D-Pad to choose and press **A** to confirm

**Step 3: Final confirmation**
- Shows you what's about to be deleted
- Press **A** to go ahead
- Press **B** to back out

**Step 4: Watch it work**
The app shows you what it's doing as it:
- Backs up your saves (if you chose to)
- Deletes the title
- Removes all associated data

When it's done, it refreshes the list automatically.

## Understanding the Interface

### Top Screen Layout

```
┌────────────────────────────────────────┐
│ 3DS Fast Uninstall                     │ ← App name
├────────────────────────────────────────┤
│ Titles: 150  Selected: 3  Sort: Name   │ ← Info bar
├────────────────────────────────────────┤
│ [ ] Super Mario 3D Land    00040000... │ ← Not selected
│ [X] Pokemon Sun ^          00040000... │ ← Selected (Update)
│ [ ] Mario Kart 8 DLC +     00040000... │ ← DLC
└────────────────────────────────────────┘
```

**What the symbols mean:**
- `^` = This is an update/patch
- `+` = This is DLC
- `...` = Name is longer, see bottom screen for full name

**Colors:**
- **Green title count** = You're good (under 300)
- **Red title count** = Warning! Over 300 titles (HOME menu can't show more than 300)

### Bottom Screen Layout

```
┌────────────────────────────────────────┐
│  ?   TITLE DETAILS                     │ ← Icon placeholder
├────────────────────────────────────────┤
│ Name: The Legend of Zelda: Ocarina of │ ← Full name
│ Time 3D                                │
│                                        │
│ Title ID: 0004000000033500             │ ← Unique ID
│ Version: v0                            │ ← Game version
│ Size: 823.45 MB                        │ ← Space used
│ Type: Game/Application                 │ ← What it is
│ Location: SD Card                      │ ← Where it's stored
├────────────────────────────────────────┤
│ Backup: NO ✗                           │ ← Backup status
├────────────────────────────────────────┤
│ Press SELECT for controls              │ ← Help reminder
└────────────────────────────────────────┘
```

## Tips and Tricks

### Sorting Your List

- Press **L** to sort by name (A to Z)
- Press **R** to sort by Title ID (groups games/updates/DLC)

Sorting resets your selection, so do it before you start selecting titles.

### Fast Navigation

- **D-Pad Up/Down**: Move one item at a time (precise)
- **D-Pad Left/Right**: Jump by a full page (fast)
- Hold Up/Down for 1.5 seconds: Starts slow continuous scroll

The slow scroll is intentional - it's way better than accidentally flying past what you wanted.

### Finding DLC and Updates

Want to clean up just DLC or updates? Use **R** to sort by Title ID. This groups them together:
- Updates (^) will be near their base games
- DLC (+) will be grouped separately

### Checking Space

The bottom screen shows how much space each title takes. Useful for finding the big ones to delete first.

### Backup Locations

Default backup path: `sdmc:/3ds/fast-uninstall/backups/`

Each backup gets its own folder named after the Title ID. Inside you'll find:
- `backup_info.txt` - Info about what was backed up
- `savedata/` - Your actual save files
- `extdata/` - Extended data (DLC, extra content)
- `boss_extdata/` - SpotPass/StreetPass data

### If You Need Help

Press **SELECT** any time to see all the controls.

## Common Scenarios

### "I want to delete all my DLC"

1. Press **R** to sort by Title ID
2. Scroll through and select all titles with a `+`
3. Press **X** and follow the prompts

### "I'm running out of space"

1. Look at the bottom screen while scrolling - it shows size for each title
2. Select the big ones you don't play
3. Delete and watch your free space go up

### "I have too many titles and the HOME menu is weird"

If your title counter is red (300+):
1. Go through and delete stuff you don't use
2. Get it under 300 for the HOME menu to work properly

### "I deleted something by accident"

If you backed up the saves:
1. Reinstall the title from eShop or cartridge
2. Copy the backup folder contents back manually

If you didn't backup: You'll need to start over, sorry.

## Things to Remember

- **System titles are hidden** - You can't delete them even if you wanted to
- **Backups are separate** - Deleting a title doesn't delete its backup
- **DLC and base games are separate** - Deleting DLC keeps the base game
- **Updates and base games are separate** - Deleting an update keeps the game (just at older version)
- **Sleep mode works** - Close your 3DS, it won't crash

## Troubleshooting

**App won't load/crashes**
- Make sure you're on latest Homebrew Launcher
- Try restarting your 3DS

**No titles showing up**
- You probably have only system titles
- Try installing a demo from eShop to test

**Progress bar stuck**
- It's not stuck, just slow if you have 200+ titles
- Each title needs to be read from the system

**Scroll is too slow**
- It's intentional for precision
- Use Left/Right for faster navigation

**Can't find a title**
- Try both sort modes (L and R)
- Some updates/DLC have weird names

## Need More Help?

Check the README.md or open an issue on GitHub.
