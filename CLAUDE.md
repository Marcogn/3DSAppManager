# CLAUDE.md

Guide for Claude agents working on this repository. `README.md` is the
source of truth for user-facing behavior, controls, and the full module
architecture — don't duplicate it here; this file only covers what's
relevant to writing and verifying new code.

## What this project is

A Nintendo 3DS homebrew application (C, `libctru` + `citro2d`/`citro3d`)
for batch-uninstalling titles with optional save-data backup, CIA
installation, and system info browsing. Runs under CFW (Luma3DS
recommended) via the Homebrew Launcher.

## Architecture

Modular, 8 modules under `source/` (see README's "Technical Reference" →
"Architecture" for the full breakdown and rendering-pipeline notes):
`types.h` (shared types/constants), `globals.h/c`, `utils.h/c`,
`config.h/c`, `titles.h/c`, `backup_restore.h/c`, `install.h/c`,
`draw.h/c`, `input.h/c`, `main.c`. State (title list, cursors, sort/filter
mode, config) is global, declared in `globals.h` and defined in
`globals.c` — most functions read/write it directly rather than passing
it around.

## Backup and deletion: invariants to preserve

These were the source of a real, user-reported bug (backup running
instantly and silently doing nothing — see `CHANGELOG.md` v2.3.1) and are
easy to regress if touched carelessly:

- `backupSaveDataToPath()` (`backup_restore.c`) must only report success
  (`true`, `hasBackup = true`) when it can actually account for the data:
  either every archive it found (savedata/extdata/boss_extdata) was fully
  copied, or none of them exist yet (a title that was never launched has
  no save — legitimately an empty, successful backup). Opening an archive
  successfully is **not** sufficient on its own — a write can still fail
  partway (SD full) and must be caught by checking the actual read/write
  result, not just the open result.
  - `backup_info.txt` must only be (re)written once success is confirmed
    — writing it earlier lets a failed re-backup attempt overwrite a
    prior valid backup's date with today's, even though nothing new was
    actually copied.
  - On total failure, clean up any stub folder this attempt created —
    but only if `backupDir` did not already exist before this attempt.
    A re-backup attempt on top of an existing valid backup must never be
    deleted on failure (see `removeCreatedTree`/`backupDirPreexisted` in
    `backupSaveDataToPath`).
  - Known limitation, not yet fixed: a failed re-backup attempt is not
    rolled back — files it already overwrote in place before failing
    keep their new (possibly incomplete) content. True atomicity would
    need a staging-folder-and-swap design; flagged in `CHANGELOG.md`
    rather than fixed, since it's a real scope increase.
- `checkBackupExists()`/`findBackupDir()` (`titles.c`) only check for a
  folder whose name starts with the title's ID — they do **not** verify
  it has real content. This is exactly why the cleanup above matters: a
  failed attempt that leaves a folder behind will show up as "has backup"
  everywhere in the UI until the app is restarted and rescans.
- The uninstall flow (`runUninstallDeleteFlow` in `input.c`) must never
  call `deleteTitle()` for a title whose just-requested backup failed —
  skip it and report it, don't delete unbacked-up data.
- Don't widen which titles can be deleted without keeping the safety
  filters (`titlePassesSafetyFilter` in `titles.c`): system title ranges
  `0x00040010`, `0x00040030`, `0x00040138` stay excluded.

## Test coverage — what's real, what isn't

`backup_restore.c`'s archive-copy logic (`copyDirectory`/`backupArchive`/
`backupSaveDataToPath`/`restoreSaveData`/`restoreDirectory`) shipped a real,
user-reported bug (backup running instantly and silently doing nothing —
`CHANGELOG.md` v2.3.1) that green host tests never would have caught,
because at the time the host suite didn't touch that file at all. It now
does: `tests/shims/fake_fs.h` is a small in-memory fake `FS_Archive`
backend (opt-in — unregistered archives/paths behave exactly like the old
always-fail stubs, so existing tests are unaffected) that `tests/test_backup.c`
uses to actually exercise archive traversal, copy failures, the
open-succeeds-but-copy-fails distinction, and the no-clobber-on-reattempt
invariant. `tests/test_install.c` covers `install.c`'s `getCIATitleID`
(binary CIA header parsing) and `scanDirectory` — both pure host I/O, no
fake archive needed.

**Still genuinely untested**, and why:
- `installCIA` (`install.c`): drives real `AM_StartCiaInstall`/
  `AM_FinishCiaInstall` — stubbed to always fail in `tests/shims/3ds.h`,
  never exercised. Verify CIA-install changes by reading the logic
  carefully or on real hardware.
- `deleteTitle`/`deleteTitleCompletely` (`backup_restore.c`): thin
  wrappers around `AM_DeleteTitle`/`AM_GetTitleInfo` plus a blocking
  `drawDialog` + `aptMainLoop` loop — correctness here is really an
  AM-service question, not app logic; not host-testable in a way that
  would add real signal.
- `input.c`'s blocking flow functions (`runInstallFlow`, `runBackupFlow`,
  `runUninstallDeleteFlow`, `runSysInfoDetailFlow`) and all of `draw.c`:
  driven by `hidKeysDown()`/`aptMainLoop()` loops and C2D/C3D rendering —
  UI/integration surface, not practical to assert on with host unit
  tests. `input.c` and `draw.c` DO at least compile cleanly against
  `tests/shims/` now (see below), which catches syntax/type errors even
  without behavioral coverage.

When touching any of the "still untested" code above, say explicitly in
your summary that it wasn't exercised by `make test` — don't imply test
coverage that doesn't exist.

## Known gotchas

- `fopen(path, "wb")` succeeding does **not** mean the write succeeded —
  check `fwrite()`'s return value against the expected byte count,
  especially here where "SD card full" is a realistic failure mode.
- `stat()`'s return value must be checked before reading its output
  struct — using `st.st_mode` after a failed `stat()` reads uninitialized
  stack memory.
- A local buffer sized independently of the app's real limits can create
  an invisible, silent cap. `_installFolderCIAs` (`input.c`) used to hardcode
  `char[64][512]` while the rest of the app scans up to `MAX_FILES` (256)
  — a folder with 65+ CIAs silently installed only the first 64 with no
  warning. Derive limits from the shared constants in `types.h`, not new
  ad hoc numbers.
- The 3DS's default thread stack is small. Don't put large fixed-size
  arrays on the stack (e.g. `char[256][512]` would be 128KB) — heap-
  allocate instead, as `_installFolderCIAs` now does.
- No devkitARM toolchain is available in a plain sandboxed session — you
  cannot build the real `.3dsx` locally. `gcc -fsyntax-only`/`-c` against
  `tests/shims/` (see `tests/shims/3ds.h` for what's stubbed — every
  `source/*.c` file compiles cleanly against it, not just the ones the
  test suite links) catches compile errors in modules the test suite
  doesn't fully build; CI (`.github/workflows/build.yml`,
  `devkitpro/devkitarm` container) is the real build check. Runtime
  behavior on real hardware still can't be verified from here — say so
  rather than claiming it was tested.
- A C block comment containing a literal `*/` substring inside its text
  (e.g. writing out a glob like `FSUSER_*/FSFILE_*`) closes the comment
  early and corrupts everything after it into code — happened once
  writing `tests/shims/fake_fs.h`'s own file header. Avoid `*/` inside
  comment prose entirely, not just as the closer.

## Build/test commands

```bash
make clean && make        # real 3DS build — requires DEVKITARM/DEVKITPRO
make test                 # host-side unit tests: test_utils, test_config,
                           # test_titles, test_backup, test_install
                           # (see "Test coverage" above for what each covers)
```

CI runs both on every push/PR: `.github/workflows/build.yml` (real
devkitARM build) and `.github/workflows/tests.yml` (host test suite).
`.github/workflows/release.yml` is manual-only (`workflow_dispatch`, a
`version` + `title` input, never fires on push/PR) — trigger it from the
Actions tab when the user actually asks to cut a release. It validates the
inputs, renames `CHANGELOG.md`'s `## Unreleased` section to
`## vX.Y.Z – <title> (date)`, bumps `VERSION_STRING` in `source/types.h`
to match, commits and pushes that bump to `main`, builds, and publishes
the `.3dsx`/`.smdh` as a GitHub Release using that section verbatim as the
release notes.

## Changelog policy

Every change gets a `CHANGELOG.md` entry under a `## Unreleased` section
at the top of the file when it's made (create the section if missing),
not deferred until a release is cut. Don't rename it to a `## vX.Y.Z`
heading or bump `VERSION_STRING` yourself — that rename/bump is what the
`Release` workflow above does, on request, as one atomic step.

## Code conventions

- No comments explaining *what* code does — only the non-obvious *why*
  (a hidden constraint, a workaround, an invariant that would surprise a
  reader). The existing codebase is terse and mostly comment-free by
  design; match it.
- Prefer small, local fixes over refactors. State is global and shared
  across many screens/flows — a "small" change to a shared helper
  (`titles.c`, `utils.c`, `globals.c`) can affect every screen.
- Don't introduce new dependencies or expand libctru/citro2d usage beyond
  what's already linked (see `Makefile`'s `LIBS`) without an explicit
  request.
- Keep UI text and dialog strings terse — this app renders to a fixed
  400×240 top / 320×240 bottom screen with a small bitmap font; long
  strings get truncated by the existing `strncpy`+ellipsis patterns
  throughout `draw.c`, so match that pattern rather than assuming
  wrapping.

## What NOT to do until explicitly requested

Don't rename the repository or move deep technical/architecture content
out of `README.md` into a separate `docs/` tree — a further documentation
cleanup pass (README trimmed to a front door, `CLAUDE.md` kept minimal,
everything else moved elsewhere) is planned but separate and
explicitly-scoped, not implied by a request to fix a bug, review code, or
touch a specific section. Don't add internationalization (Italian/English)
either — also planned, also separate.

Do trigger `Release` (`.github/workflows/release.yml`, `workflow_dispatch`)
when the user actually asks to cut a release — it's built and safe to run,
it just never fires on its own (push/PR never triggers it).
