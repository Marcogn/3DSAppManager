# Plan: 3DS Fast Uninstall v2.0 — Completamento Multi-Flow
# File: plan-3dsFastUninstallV2.prompt.md
## Obiettivo
Completare l'applicazione homebrew Nintendo 3DS trasformandola da semplice
uninstaller in uno strumento multi-funzione con:
- Menu principale di selezione flusso
- Installazione CIA (singola o da cartella) con file browser
- Backup salvataggi con stato visuale e selezione titoli
- Disinstallazione (flusso esistente + DLC/Update correlati)
- Info sistema (panoramica + drill-down + azioni per titolo)
- Pagina impostazioni persistente
---
## Stato del Codice (source/main.c — 831 righe)
### Già implementato (NON modificare righe 1-831)
Il backend è COMPLETO. Le righe 1-831 contengono:
**Defines:**
- MAX_TITLES 500, MAX_VISIBLE_TITLES 13, MAX_FILES 256
- CONFIG_PATH "sdmc:/3ds/fast-uninstall/config.ini"
- DEFAULT_BACKUP_PATH "sdmc:/3ds/fast-uninstall/backups"
- CHUNK_SIZE 0x10000
- BACKUP_PATH_OPTIONS[] con 5 percorsi alternativi
**Enum già definiti:**
- AppState: APP_MAIN_MENU, APP_INSTALL, APP_BACKUP, APP_UNINSTALL, APP_SYSINFO, APP_SETTINGS
- SortMode: SORT_BY_NAME, SORT_BY_SIZE, SORT_BY_TITLEID
- FilterMode: FILTER_ALL, FILTER_UPDATES, FILTER_DLC
- SysInfoMode: SYSINFO_OVERVIEW, SYSINFO_GAMES, SYSINFO_UPDATES, SYSINFO_DLC
**Struct già definiti:**
- TitleInfo: titleID, name[256], fullName[256], mediaType, selected, isValid, version, hasBackup, icon, iconLoaded, size
- Config: backupPath[256], forceBackup, skipUninstallConfirm, forceRestore, skipInstallConfirm, installDest
- FileEntry: name[256], isDir, isCIA, size
**Variabili globali già dichiarate:**
- titles[MAX_TITLES], titleCount, cursor, scrollOffset
- config (Config struct)
- top, bottom (C3D_RenderTarget*)
- dynamicBuf (C2D_TextBuf)
- needsRedraw, currentSortMode, currentFilterMode
- filteredIndices[MAX_TITLES], filteredCount
- appState, menuCursor
- fileEntries[MAX_FILES], fileCount, fileCursor, fileScrollOffset, currentPath[512]
- backupCursor, backupScrollOffset
- sysInfoMode, sysInfoCursor, sysInfoSubCursor, sysInfoSubScrollOffset
- sysInfoSubIndices[MAX_TITLES], sysInfoSubCount
- settingsCursor
**Funzioni backend già implementate:**
- createDirectory, sanitizeName, formatSize
- loadConfig, saveConfig, saveDefaultConfig
- getBackupDirName (formato: "%016llX-%s"), findBackupDir, getBackupLastDate, checkBackupExists
- getSDFreeSpace
- compareTitlesByName/Size/ID, sortTitles, updateFilteredList
- getTitleName (multi-lingua SMDH), getTitleSize, loadTitleIcon, getTitleInfo
- drawLoadingScreen (con barra progresso)
- loadTitles (SD+NAND, filtro sistema, sort)
- findRelatedTitles (trova DLC/Update per un titolo base)
- copyDirectory, backupArchive, backupSaveDataToPath, backupSaveData
- restoreDirectory, restoreSaveData
- getCIATitleID (legge TitleID dall'header CIA)
- scanDirectory (popola fileEntries[] con dir + .cia)
- installCIA (installa a blocchi con progress bar inline)
- deleteTitleCompletely, deleteTitle (con dialog esito + inner aptMainLoop)
**Prototipi già dichiarati (da implementare):**
- drawMainMenu, drawUI, drawSelectedTitlesList, drawTouchControls
- drawDialog, drawDialogWithSelectedList, drawControlsOverlay
- drawFileBrowserScreen, drawBackupScreen, drawSysInfoScreen
- drawSettingsScreen, drawTitleDetails
- handleMainMenuInput, handleUninstallInput
- runInstallFlow, runBackupFlow
- handleSysInfoInput, handleSettingsInput
---
## Cosa Aggiungere (righe 832+)
### SECTION 9 — Funzioni Draw
Palette colori consigliata (definire come macro locali o usare inline):
- CLR_BG       = C2D_Color32(20,20,30,255)      sfondo schermi
- CLR_HEADER   = C2D_Color32(40,60,100,255)     barra header
- CLR_SELECTED = C2D_Color32(60,100,180,255)    voce selezionata
- CLR_WHITE    = C2D_Color32(255,255,255,255)
- CLR_GRAY     = C2D_Color32(160,160,160,255)
- CLR_RED      = C2D_Color32(220,60,60,255)
- CLR_GREEN    = C2D_Color32(60,200,60,255)
- CLR_YELLOW   = C2D_Color32(255,220,60,255)
- CLR_CYAN     = C2D_Color32(100,220,255,255)
- CLR_BACKUP_OK= C2D_Color32(255,200,50,255)    icona backup stella
**REGOLA CRITICA RENDERING** (da CHANGELOG v0.11.1):
Non chiamare mai C2D_SceneBegin() due volte sullo stesso target nello stesso frame.
Le funzioni draw NON chiamano C3D_FrameBegin/End (lo fa il main loop).
Eccezione: flussi bloccanti (runInstallFlow, runBackupFlow, deleteTitle) che hanno
il proprio inner aptMainLoop() possono e devono gestire i propri frame.
#### drawMainMenu()
Top screen:
- Header "3DS Fast Uninstall" sfondo CLR_HEADER y=0 h=22
- 5 voci menu da y=60, spaziatura 30px:
  0: "[A] Installa CIA"
  1: "[B] Backup Salvataggi"
  2: "[U] Disinstalla Titoli"
  3: "[I] Info Sistema"
  4: "[S] Impostazioni"
- Voce con menuCursor: sfondo CLR_SELECTED, ">" a x=15
- Footer "START = Esci"
Bottom screen:
- Descrizione contestuale voce selezionata (2-3 righe)
- Footer "A = Seleziona  START = Esci"
#### drawUI()
Top screen (lista uninstall):
- Header "Disinstalla" + info bar "T:NNN  Sel:NNN  Sort:XXX  [Filter]" y=22
- Lista MAX_VISIBLE_TITLES=13 voci visibili da filteredIndices[]
  Colonne: checkbox x=3, nome x=28 (max ~28 char + "..."), simbolo x=235, TitleID x=255
  Simboli: "^" per Update (hi==0x0004000E), "+" per DLC (hi==0x0004008C)
  Cursore: sfondo CLR_SELECTED
  Selezionati (selected==true): checkbox "[X]" testo CLR_YELLOW
- Footer "A=Sel  X=Esegui  L/R=Sort  Y=Filter  B=Menu  SELECT=Help"
Bottom screen: chiama drawTouchControls()
#### drawTouchControls()
Dettagli titolo corrente (filteredIndices[cursor]):
- "DETTAGLI TITOLO" header
- Nome: fullName
- TitleID: hex 16 cifre
- Versione: v[N]
- Dimensione: formatSize()
- Tipo: Gioco / Update / DLC
- Posizione: SD / NAND
- Backup: SI checkmark (con data) / NO croce
- Footer "[SELECT = Comandi]"
#### drawSelectedTitlesList()
Bottom screen durante dialogs uninstall:
- Header rosso "TITOLI SELEZIONATI (N)"
- Max 10 voci visibili con nome + simbolo tipo
- Se >10: "...e altri N" in fondo
- Sfondo CLR_BG
#### drawDialog(lines, lineCount)
Box centrato su top screen:
- Box scuro con bordo bianco
- lineCount righe di testo centrate verticalmente
- NON tocca bottom screen
#### drawDialogWithSelectedList(lines, lineCount)
- Top: stesso di drawDialog()
- Bottom: chiama drawSelectedTitlesList()
#### drawControlsOverlay()
Overlay SELECT (tenuto premuto):
- Top: box 300x210 centrato, sfondo scuro semitrasparente, bordo bianco, 8 voci controlli spaziatura 13px
- Bottom: box analogo con controlli touch
- CRITICO: deve essere l'UNICA funzione a disegnare su top/bottom nel frame in cui è chiamata
  Non chiamare drawUI() + drawControlsOverlay() nello stesso frame sullo stesso target
#### drawFileBrowserScreen()
Top screen:
- Header "Installa CIA" + path corrente (troncato se >40 char)
- Lista fileEntries[] MAX_VISIBLE_TITLES voci con scroll
  Dir: "[DIR] nome/" colore CLR_CYAN
  CIA: "nome.cia  [X.XX MB]" colore CLR_WHITE
  Cursore: sfondo CLR_SELECTED
- Footer "A=Entra/Sel  Y=Installa tutto  B=Su/Menu  START=Annulla"
Bottom screen:
- Se cursore su .cia: "TitleID: [hex]" (getCIATitleID()), dimensione
- Se cursore su dir: nome cartella
- "Dest: SD / NAND" (config.installDest)
#### drawBackupScreen()
Top screen:
- Header "Backup Salvataggi" + "Sel:N"
- Lista TUTTI i titoli (titleCount, non filtered) MAX_VISIBLE_TITLES voci
  Con backup: "[*] Nome                  DD/MM/YY" asterisco in CLR_BACKUP_OK
  Senza:      "[ ] Nome"
  Selezionati: "[X]" testo CLR_YELLOW
  Cursore: sfondo CLR_SELECTED
- Footer "A=Sel  X=Backup Sel  Y=Backup Tutti  B=Menu"
Bottom screen:
- Dettagli titolo backupCursor
- "Ultimo backup: DD/MM/YYYY HH:MM" o "Nessun backup"
Nota: usare "[*]" invece di "★" per compatibilità charset 3DS
#### drawSysInfoScreen()
Gestisce 4 sotto-modalità via sysInfoMode:
SYSINFO_OVERVIEW (top screen):
- Header "INFORMAZIONI SISTEMA"
- 3 righe navigabili (sysInfoCursor 0-2):
  "  Giochi:  NNN    X.XX GB"
  "  Update:  NNN    X.XX MB"
  "  DLC:     NNN    X.XX MB"
  Calcola totali da titles[] in runtime
- Separatore
- "SD Libero: X.XX GB / X.XX GB"  (getSDFreeSpace())
- Riga selezionata: sfondo CLR_SELECTED
- Footer "A=Dettagli  B=Menu"
SYSINFO_GAMES/UPDATES/DLC (top screen):
- Header "GIOCHI" / "UPDATE" / "DLC" + counter
- Lista sysInfoSubIndices[] MAX_VISIBLE_TITLES voci
  "Nome                    [X.XX MB]"
  sysInfoSubCursor evidenziato CLR_SELECTED
- Footer "A=Dettagli  B=Indietro"
Bottom screen: dettagli titolo sysInfoSubIndices[sysInfoSubCursor] (semplificato)
#### drawSettingsScreen()
Top screen:
- Header "IMPOSTAZIONI"
- 6 voci (settingsCursor 0-5):
  "Forza Backup:          [ON/OFF]"
  "Salta Confirm Uninst:  [ON/OFF]"
  "Forza Restore:         [ON/OFF]"
  "Salta Confirm Install: [ON/OFF]"
  "Dest. Install:         [SD/NAND]"
  "Cartella Backup:       [path troncato 25 char]"
  Cursore: sfondo CLR_SELECTED, valore in CLR_CYAN
- Footer "A/Sx/Dx=Cambia  B=Salva e Torna"
Bottom screen:
- Descrizione della voce selezionata (2-3 righe esplicative)
#### drawTitleDetails()
Usata nel flusso SysInfo per dettaglio titolo selezionato.
Top screen:
- Nome, TitleID, versione, dimensione, tipo, posizione
- Stato backup con data
- Se titolo base: lista DLC/Update correlati (findRelatedTitles())
- 3 opzioni navigabili (variabile locale detailCursor 0-2):
  "[A] Backup Salvataggio"
  "[Y] Ripristina Salvataggio"
  "[X] Cancella Titolo (+ correlati)"
- Footer "B=Indietro"
---
### SECTION 10 — Input/Flow Handlers
#### handleMainMenuInput()
Chiamata ogni frame quando appState==APP_MAIN_MENU.
```
KEY_DOWN/UP: menuCursor (0-4, wrapping)
KEY_A: switch(menuCursor):
  0: appState=APP_INSTALL
  1: if(titleCount==0) loadTitles(); appState=APP_BACKUP
  2: if(titleCount==0) loadTitles(); updateFilteredList(); cursor=0; scrollOffset=0; appState=APP_UNINSTALL
  3: if(titleCount==0) loadTitles(); sysInfoMode=SYSINFO_OVERVIEW; sysInfoCursor=0; appState=APP_SYSINFO
  4: settingsCursor=0; appState=APP_SETTINGS
KEY_START: (gestito da main() con flag running=false)
```
#### handleUninstallInput()
Chiamata ogni frame quando appState==APP_UNINSTALL.
Navigazione:
- KEY_DOWN: cursor++, aggiusta scrollOffset (mantieni cursore in finestra)
- KEY_UP: cursor--, aggiusta scrollOffset
- KEY_LEFT: cursor -= MAX_VISIBLE_TITLES (page up, clamp a 0)
- KEY_RIGHT: cursor += MAX_VISIBLE_TITLES (page down, clamp a filteredCount-1)
- KEY_L: currentSortMode = (currentSortMode-1+3)%3; sortTitles(); updateFilteredList()
- KEY_R: currentSortMode = (currentSortMode+1)%3; sortTitles(); updateFilteredList()
- KEY_Y: currentFilterMode = (currentFilterMode+1)%3; updateFilteredList()
- KEY_A: titles[filteredIndices[cursor]].selected ^= true
- KEY_SELECT (held): inner loop che mostra drawControlsOverlay() finche tenuto
- KEY_B: reset selezioni; appState=APP_MAIN_MENU
Flusso KEY_X (uninstall):
1. Conta selectedCount = titoli con selected==true && isValid==true
2. Se selectedCount==0: showDialog "Nessun titolo selezionato" e return
3. [NUOVO] Ricerca correlati:
   - Per ogni titolo base selezionato (hi==0x00040000):
     findRelatedTitles(tid, idx, relIdx[], &relCount)
     Accumula in pendingRelated[] i titoli NON gia selezionati e isValid==true
   - Se pendingRelated non vuoto:
     drawDialogWithSelectedList: "Trovati N titoli correlati (DLC/Update).\nIncludere? A=Si  B=No  START=Annulla"
     Inner loop attendi A/B/START:
       A: aggiungi pendingRelated a selezioni
       B: continua senza aggiungere
       START: reset selezioni; return (annulla tutto)
4. Backup check:
   - Se config.forceBackup: doBackup=true, chosenPath=config.backupPath
   - Altrimenti: drawDialogWithSelectedList "Backup salvataggi?\nA=Si  B=No  START=Annulla"
     A: doBackup=true; B: doBackup=false; START: return
5. Se doBackup: scegli path
   dialog "Percorso: [config.backupPath]\nA=Usa default  Y=Scegli altro"
   Se Y: cicla BACKUP_PATH_OPTIONS con D-pad, A=conferma
   chosenPath = percorso scelto
6. Se !config.skipUninstallConfirm:
   drawDialogWithSelectedList "Cancella N titoli?\nBackup: [Si/No] in [path]\nA=Conferma  B=Annulla"
   B: reset selezioni; return
7. Esecuzione per ogni titles[i] con selected==true && isValid==true:
   Se doBackup: backupSaveDataToPath(&titles[i], chosenPath)
   deleteTitle(&titles[i])  // ha proprio inner loop con dialog esito
8. titleCount=0; cursor=0; scrollOffset=0
   Reset titles[i].selected per tutti
   appState=APP_MAIN_MENU
#### runInstallFlow()
Flusso bloccante. Gestisce propri C3D_FrameBegin/End.
La funzione NON usa appState (viene chiamata e ritorna).
```
scanDirectory("sdmc:/")
fileCursor=0; fileScrollOffset=0
Inner loop (file browser):
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW)
  drawFileBrowserScreen()
  C3D_FrameEnd(0)
  hidScanInput()
  keys = hidKeysDown()
  KEY_DOWN: fileCursor++; aggiusta fileScrollOffset
  KEY_UP:   fileCursor--; aggiusta fileScrollOffset
  KEY_START: return
  KEY_B:
    Se currentPath != "sdmc:/": sali alla dir padre; scanDirectory(parent)
    Else: return
  KEY_A su directory:
    mini-dialog top screen "A=Entra  Y=Installa tutto .cia  B=Annulla"
    inner loop attendi:
      A: scanDirectory(nuovoPath); break a loop principale
      Y: goto installFolder con currentPath+"/"+fileEntries[fileCursor].name
      B: break da mini-dialog (torna al browser)
  KEY_A su file .cia:
    goto installSingle
  KEY_Y (qualsiasi voce, se la dir corrente ha .cia):
    goto installFolder con currentPath
installSingle:
  ciaFullPath = currentPath + "/" + fileEntries[fileCursor].name
  tid = getCIATitleID(ciaFullPath)
  dest = (config.installDest==0) ? MEDIATYPE_SD : MEDIATYPE_NAND
  Se !config.skipInstallConfirm:
    dialog "Installare [filename]?\nTitleID: [hex]\nDest: [SD/NAND]\nA=Si  B=No"
    Se B: torna al loop browser
  ok = installCIA(ciaFullPath, dest)  // mostra progress bar interna
  Se ok:
    dialog "Installazione completata!\n[filename]\nA=Continua"
    attendi A
    // Controlla backup
    char backupDir[512]
    Se findBackupDir(tid, backupDir, sizeof(backupDir)):
      // Crea TitleInfo temporaneo per il restore
      TitleInfo tmp = {.titleID=tid, .mediaType=dest}
      getTitleName(tid, dest, tmp.name, sizeof(tmp.name))
      Se config.forceRestore:
        restoreSaveData(&tmp)
        dialog "Salvataggio ripristinato automaticamente.\nA=OK"
        attendi A
      Else:
        dialog "Trovato backup per questo titolo.\nRipristinare il salvataggio?\nA=Si  B=No"
        Se A: restoreSaveData(&tmp); dialog "Ripristinato.\nA=OK"; attendi A
  Else:
    dialog "Errore durante l'installazione.\nA=Continua"
    attendi A
  Torna al loop browser
installFolder:
  Costruisci lista CIA nella cartella target:
    Usa scanDirectory() temporaneo o apri dir con opendir() e raccogli .cia
  Per ciascun file .cia: esegui logica installSingle (inline, senza tornare al browser)
  dialog finale "Installati N/M file CIA.\nA=Continua"
  attendi A
  return
```
#### runBackupFlow()
Flusso bloccante. Gestisce propri C3D_FrameBegin/End.
Pre-condizione: titleCount > 0 (il chiamante ha caricato i titoli).
```
Per tutti titles[i]: selected=false
backupCursor=0; backupScrollOffset=0
Inner loop:
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW)
  drawBackupScreen()
  C3D_FrameEnd(0)
  hidScanInput()
  keys = hidKeysDown()
  KEY_DOWN: backupCursor++; clamp a titleCount-1; aggiusta backupScrollOffset
  KEY_UP:   backupCursor--; clamp a 0; aggiusta backupScrollOffset
  KEY_A: titles[backupCursor].selected ^= true
  KEY_B / KEY_START: return
  KEY_X (backup selezionati):
    selectedCount = conta titoli selected==true
    Se selectedCount==0: dialog "Nessun titolo selezionato."; continua loop
    dialog "Backup di N titoli selezionati?\nA=Si  B=No"
    Se A:
      Per ogni titles[i] selected&&isValid:
        drawLoadingScreen(progressivo, selectedCount, titles[i].name)
        backupSaveData(&titles[i])
        titles[i].hasBackup=true
      dialog "Backup completato per N titoli.\nA=Continua"
      attendi A; return
  KEY_Y (backup tutti):
    dialog "Backup di TUTTI i titoli (N)?\nA=Si  B=No"
    Se A:
      Per ogni titles[i] isValid:
        drawLoadingScreen(i, titleCount, titles[i].name)
        backupSaveData(&titles[i])
        titles[i].hasBackup=true
      dialog "Backup completato per N titoli.\nA=Continua"
      attendi A; return
```
#### handleSysInfoInput()
Chiamata ogni frame quando appState==APP_SYSINFO.
```
SYSINFO_OVERVIEW:
  KEY_DOWN/UP: sysInfoCursor (0-2, wrapping)
  KEY_A:
    u32 targetHi = (sysInfoCursor==0)?0x00040000:(sysInfoCursor==1)?0x0004000E:0x0004008C
    sysInfoSubCount=0
    Per ogni titles[i] isValid && (titles[i].titleID>>32)==targetHi:
      sysInfoSubIndices[sysInfoSubCount++]=i
    sysInfoSubCursor=0; sysInfoSubScrollOffset=0
    sysInfoMode = SYSINFO_GAMES / SYSINFO_UPDATES / SYSINFO_DLC
  KEY_B: appState=APP_MAIN_MENU
SYSINFO_GAMES/UPDATES/DLC:
  KEY_DOWN/UP: sysInfoSubCursor (0 a sysInfoSubCount-1)
  KEY_B: sysInfoMode=SYSINFO_OVERVIEW; sysInfoCursor=0
  KEY_A: avvia title detail (inner loop bloccante):
    idx = sysInfoSubIndices[sysInfoSubCursor]
    int detailCursor=0
    inner loop:
      C3D_FrameBegin; drawTitleDetails() per titles[idx]; C3D_FrameEnd
      hidScanInput()
      KEY_UP/DOWN: detailCursor (0-2, wrapping)
      KEY_B: break (torna sottolista)
      KEY_A (detailCursor==0 — Backup):
        backupSaveData(&titles[idx])
        titles[idx].hasBackup=true
        dialog "Backup eseguito.\nA=OK"; attendi A
      KEY_A (detailCursor==1 — Restore):
        Se !titles[idx].hasBackup: dialog "Nessun backup disponibile.\nA=OK"; attendi A; continue
        ok=restoreSaveData(&titles[idx])
        dialog ok?"Ripristinato.\nA=OK":"Errore ripristino.\nA=OK"; attendi A
      KEY_A (detailCursor==2 — Delete):
        [Stessa logica DLC/Update correlati di handleUninstallInput step 3-6]
        Ma solo per titles[idx] come titolo di partenza
        deleteTitle(&titles[idx])
        titleCount=0
        sysInfoMode=SYSINFO_OVERVIEW
        break a inner loop
```
#### handleSettingsInput()
Chiamata ogni frame quando appState==APP_SETTINGS.
```
bool* boolPtrs[4] = {&config.forceBackup, &config.skipUninstallConfirm,
                     &config.forceRestore, &config.skipInstallConfirm}
KEY_DOWN/UP: settingsCursor (0-5, wrapping)
KEY_A / KEY_RIGHT:
  Se settingsCursor 0-3: *boolPtrs[settingsCursor] = !(*boolPtrs[settingsCursor])
  Se settingsCursor==4:  config.installDest = (config.installDest+1)%2
  Se settingsCursor==5:
    Trova indice corrente in BACKUP_PATH_OPTIONS (cerca backupPath nel vettore)
    idx=(idx+1)%NUM_BACKUP_PATHS
    snprintf(config.backupPath, sizeof(config.backupPath), "%s", BACKUP_PATH_OPTIONS[idx])
KEY_LEFT:
  Se settingsCursor==4: config.installDest=(config.installDest+1)%2 (solo 2 opzioni)
  Se settingsCursor==5: stessa logica KEY_RIGHT ma idx decrementa
  Se settingsCursor 0-3: toggle (come KEY_A)
KEY_B / KEY_START:
  saveConfig()
  appState=APP_MAIN_MENU
```
---
### SECTION 11 — main()
```c
int main(void) {
    // Init grafica
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top    = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    dynamicBuf = C2D_TextBufNew(4096);
    // Init servizi 3DS
    amInit();
    fsInit();
    cfguInit();
    // Carica configurazione
    loadConfig();
    // titleCount rimane 0 — caricamento lazy
    bool running = true;
    while (aptMainLoop() && running) {
        hidScanInput();
        u32 keys = hidKeysDown();
        // EXIT globale solo dal menu principale
        if (appState == APP_MAIN_MENU && (keys & KEY_START)) {
            running = false;
            break;
        }
        // Flussi bloccanti: chiudi frame corrente PRIMA di entrare
        if (appState == APP_INSTALL) {
            runInstallFlow();
            appState = APP_MAIN_MENU;
            continue;
        }
        if (appState == APP_BACKUP) {
            runBackupFlow();
            appState = APP_MAIN_MENU;
            continue;
        }
        // Flussi frame-by-frame
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        switch (appState) {
            case APP_MAIN_MENU:
                handleMainMenuInput();
                drawMainMenu();
                break;
            case APP_UNINSTALL:
                handleUninstallInput();
                if (appState == APP_UNINSTALL) drawUI();
                break;
            case APP_SYSINFO:
                handleSysInfoInput();
                if (appState == APP_SYSINFO) drawSysInfoScreen();
                break;
            case APP_SETTINGS:
                handleSettingsInput();
                if (appState == APP_SETTINGS) drawSettingsScreen();
                break;
            default:
                drawMainMenu();
                break;
        }
        C3D_FrameEnd(0);
    }
    // Cleanup
    C2D_TextBufDelete(dynamicBuf);
    C2D_Fini();
    C3D_Fini();
    cfguExit();
    fsExit();
    amExit();
    gfxExit();
    return 0;
}
```
---
## Struttura Backup su SD
```
[config.backupPath]/
  [TitleID16hex]-[TitleNameSanitized]/
    backup_info.txt        (title ID, nome, mediaType, data backup)
    savedata/              (ARCHIVE_USER_SAVEDATA)
    extdata/               (ARCHIVE_EXTDATA, se presente)
    boss_extdata/          (ARCHIVE_BOSS_EXTDATA, se presente)
```
Lookup: findBackupDir(titleID, ...) fa prefix-match sui primi 16 char del dir name.
Funziona anche se il nome del titolo cambia (TitleID come chiave univoca).
---
## Lazy Loading Titoli
- titleCount==0 indica "titoli non caricati"
- handleMainMenuInput() chiama loadTitles() se titleCount==0 prima di entrare in BACKUP/UNINSTALL/SYSINFO
- Dopo ogni operazione che invalida la lista (uninstall, delete da sysinfo): titleCount=0
- updateFilteredList() va chiamata ogni volta che si entra in APP_UNINSTALL e dopo ogni sort/filter change
---
## Filtri Sicurezza (NON modificare mai)
Titoli ESCLUSI durante loadTitles():
- SD:   hi==0x00040010 || hi==0x00040030
- NAND: hi==0x00040010 || hi==0x00040030 || hi==0x00040138
- NAND: incluso SOLO hi==0x00040000 || hi==0x0004000E || hi==0x0004008C
---
## File da Modificare
| File                        | Azione                                                        |
|-----------------------------|---------------------------------------------------------------|
| source/main.c               | Appendere Section 9, 10, 11 dopo riga 831                     |
| romfs/default_config.ini    | Aggiungere chiavi: force_backup=0, skip_uninstall_confirm=0, force_restore=0, skip_install_confirm=0, install_dest=0 |
| README.md                   | Aggiornare con sezioni Install, Backup, SysInfo, Settings     |
| USER_GUIDE.md               | Aggiornare con guide per ogni flusso + controlli              |
| CHANGELOG.md                | Aggiungere voce v2.0 con descrizione feature                  |
---
## Checklist Pre-Build
- [ ] Section 9: tutte le 12 funzioni draw implementate
- [ ] Section 10: tutti i 6 handler/flow implementati
- [ ] Section 11: main() implementato
- [ ] romfs/default_config.ini aggiornato con tutte le chiavi
- [ ] Nessuna duplicazione C2D_SceneBegin() sullo stesso target nello stesso frame
- [ ] Flussi bloccanti (install, backup) gestiscono propri C3D_FrameBegin/End
- [ ] Flussi bloccanti NON chiamano C3D_FrameBegin DOPO che main() ha gia aperto un frame
- [ ] titleCount=0 impostato dopo operazioni che invalidano la lista
- [ ] updateFilteredList() chiamata dopo sort/filter change e all'entrata in APP_UNINSTALL
- [ ] Filtri sicurezza titoli di sistema non toccati (righe 524-526 e 540-542 di main.c)
- [ ] findRelatedTitles() invocata nel flusso uninstall (KEY_X step 3) e nel detail sysinfo (delete)
- [ ] backupCursor usa indice diretto in titles[] (non filteredIndices[])
- [ ] drawBackupScreen() usa "[*]" per backup esistente (non emoji che potrebbe non renderizzare)
---
## Build
```bash
cd /home/marco/Projects/3ds-fast-uninstall
make clean && make
```
Artefatti: 3ds-fast-uninstall.3dsx, .elf, .smdh nella root del progetto.
Prerequisiti: devkitPro con DEVKITARM e DEVKITPRO impostati.
