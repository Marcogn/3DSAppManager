# Test Build - Correzioni Flickering v2

## Data Build
2 Febbraio 2026

## Modifiche in Questa Build

### 🔧 Correzioni Principali

1. **Disabilitato Double Buffering**
   - `gfxSetDoubleBuffering(GFX_TOP, false)` e `gfxSetDoubleBuffering(GFX_BOTTOM, false)`
   - Previene la sovrapposizione dei contenuti tra buffer front e back

2. **Console Globali**
   - `topScreen` e `bottomScreen` ora sono variabili statiche globali
   - Accessibili da tutte le funzioni per gestione corretta dello schermo

3. **Clear Screen Migliorato**
   - Sostituito `consoleClear()` con ANSI escape codes
   - `\x1b[2J` - pulisce l'intero schermo
   - `\x1b[H` - muove il cursore alla posizione home (0,0)

4. **Console Selection Esplicita**
   - Ogni chiamata a `drawUI()` inizia con `consoleSelect(&topScreen)`
   - Garantisce che si stia disegnando sullo schermo corretto

5. **Rimozione Input Doppio**
   - Rimossa chiamata a `hidScanInput()` in `handleInput()`
   - Ora viene chiamato solo una volta nel main loop

6. **Sincronizzazione VBlank**
   - `gspWaitForVBlank()` all'inizio del loop, non alla fine
   - Ordine: VBlank → Input → Draw → Flush → Swap

7. **Ottimizzazione Loop di Input**
   - Rimossi swap ridondanti in tutti i loop di conferma
   - Draw una volta sola, poi solo gestione input

## Cosa Aspettarsi

### ✅ Miglioramenti Attesi
- Nessun flickering dello schermo
- UI pulita senza sovrapposizioni
- Testo nitido e stabile
- Navigazione fluida a 60 FPS
- Input responsivo

### ⚠️ Possibili Effetti Collaterali
- Lo schermo inferiore rimane nero (normale, non è usato dall'app)
- Primo frame potrebbe essere leggermente più lento (normale)

## Come Testare

1. **Test Base**
   - Avvia l'app
   - Verifica che la lista titoli appaia pulita
   - Naviga su/giù con il D-Pad
   - Controlla che non ci sia flickering

2. **Test Menu**
   - Seleziona alcuni titoli con A
   - Premi X per entrare nel menu uninstall
   - Verifica che i menu di conferma siano puliti
   - Naviga attraverso le opzioni

3. **Test Selezione Path**
   - Se richiesto, prova a selezionare un path alternativo con Y
   - Naviga su/giù tra i path
   - Verifica che il menu si aggiorni correttamente

4. **Test Performance**
   - Lascia l'app aperta per qualche minuto
   - Verifica che la performance rimanga costante
   - Controlla che non ci sia degradazione nel tempo

## Problemi Noti

Nessuno attualmente. Se riscontri problemi:
1. Annota esattamente cosa vedi
2. Quando si verifica il problema
3. Cosa stavi facendo quando è apparso
4. Se è riproducibile o casuale

## File Modificati

- `source/main.c`
  - Funzione `main()` - aggiunta disabilitazione double buffering
  - Funzione `drawUI()` - migliorata pulizia schermo
  - Funzione `handleInput()` - rimossa chiamata ridondante a hidScanInput
  - Main loop - riordinato per VBlank all'inizio
  - Tutti i loop di input - ottimizzati

## Compilazione

```bash
make clean && make
```

Output atteso:
- `3ds-fast-uninstall.3dsx` - ~151KB
- `3ds-fast-uninstall.elf` - ~938KB
- Alcuni warning sulla truncation (normali, non critici)

## Installazione

Copia `3ds-fast-uninstall.3dsx` su:
- SD Card: `/3ds/3ds-fast-uninstall/`

Oppure usa Homebrew Launcher direttamente.

## Rollback

Se questa build presenta problemi peggiori della precedente:
```bash
git checkout HEAD~1 source/main.c
make clean && make
```

## Note Sviluppatore

Queste modifiche risolvono il problema del flickering causato da:
1. Double buffering che sovrapponeva contenuti
2. Pulizia schermo inefficace
3. Sincronizzazione VBlank errata
4. Input scannerizzato più volte per frame

La soluzione usa single buffering sincronizzato con VBlank, che è più appropriato per un'applicazione text-based come questa.

## Prossimi Passi

Se questa build funziona correttamente:
- [ ] Verificare su hardware reale
- [ ] Test su Citra emulator
- [ ] Test con diversi titoli installati (pochi, molti)
- [ ] Test su Old3DS e New3DS
- [ ] Release stabile

Se persistono problemi:
- Consulta `docs/RENDERING_TROUBLESHOOTING.md`
- Prova le alternative suggerite
- Considera di aprire un issue con log dettagliati
