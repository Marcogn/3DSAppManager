# AGENTS.md

## Scopo rapido
- Progetto homebrew Nintendo 3DS in C (`libctru` + `citro2d/citro3d`) per disinstallazione batch con backup salvataggi.
- Codice applicativo quasi tutto in `source/main.c` (single-file architecture).

## Mappa essenziale del repository
- `source/main.c`: logica completa (enumerazione titoli, UI top/bottom, input, backup, delete flow).
- `Makefile`: build devkitPro (`DEVKITARM`, `3ds_rules`, output `.3dsx/.elf/.smdh`).
- `romfs/default_config.ini`: default `backup_path`.
- `README.md` + `docs/README.md`: comportamento utente e note tecniche (rendering, limiti, fallback lingua).
- `CHANGELOG.md`: cronologia reale delle decisioni UI/rendering (es. fix flicker SELECT overlay).

## Workflow sviluppatore (reale)
- Prerequisiti: ambiente devkitPro con `DEVKITARM`/`DEVKITPRO` impostati.
- Build standard:
  - `make clean`
  - `make`
- Artefatti generati in root: `3ds-fast-uninstall.3dsx`, `3ds-fast-uninstall.elf`, `3ds-fast-uninstall.smdh`.
- Non ci sono test automatizzati nel repo: validazione principale e' manuale su hardware/CFW.

## Flusso applicativo da preservare
- `main()` inizializza grafica e servizi (`amInit`, `fsInit`), poi `loadConfig()` e `loadTitles()`.
- Enumerazione titoli in `loadTitles()`:
  - legge SD + NAND via AM,
  - filtra title range di sistema,
  - limita a `MAX_TITLES` (500),
  - popola `TitleInfo`, poi `sortTitles()`.
- UI split-screen:
  - top: lista selezionabile (`drawUI()`),
  - bottom: dettagli titolo o lista selezionati nei dialog (`drawTouchControls()`, `drawSelectedTitlesList()`).

## Convenzioni specifiche di questo codice
- Filtri/sort passano da stato globale (`currentSortMode`, `currentFilterMode`, `filteredIndices[]`).
- Simboli Update/DLC sono in colonna dedicata, non nel nome mostrato (vedi pulizia nome in `drawUI()`).
- Overlay controlli con SELECT ha percorso render dedicato nel main loop per evitare flicker (`C2D_SceneBegin` non duplicato sullo stesso target).
- Render continuo durante `aptMainLoop()` e sleep/wake: non introdurre skip frame aggressivi.

## Backup e cancellazione: invarianti critiche
- Backup per titolo in `backupSaveDataToPath()` sotto `[backup_path]/[TitleID]/` con `backup_info.txt` + `savedata/extdata/boss_extdata`.
- Cancellazione completa in `deleteTitleCompletely()` (ExtData/BossExtData + `AM_DeleteTitle`).
- Verifica post-delete in `deleteTitle()` tramite `AM_GetTitleInfo`; solo dopo segna `isValid=false`.
- Non ampliare scope dei titoli eliminabili senza mantenere i filtri sicurezza (`0x00040010`, `0x00040030`, `0x00040138`).

## Quando modifichi il comportamento
- Mantieni allineati i testi UI/dialog e la documentazione utente (`README.md`, `USER_GUIDE.md`).
- Se cambi UX o rendering, aggiorna anche `CHANGELOG.md` con motivazione concreta (regressioni evitate, impatto utente).
- Preferisci modifiche piccole e locali: molte feature condividono stato globale in `main.c`.
