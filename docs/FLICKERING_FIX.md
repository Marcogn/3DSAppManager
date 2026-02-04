# Correzione del Flickering e Problemi di Display

## Problema Riscontrato

L'applicazione mostrava i seguenti problemi:
- **Flickering intenso** dello schermo superiore
- **Contenuti sovrapposti** (UI e comandi visualizzati insieme sullo stesso schermo)
- **Schermo inferiore spento** e non utilizzato
- Lentezza nel caricamento della lista titoli
- **Testo duplicato/multiplo** renderizzato sullo stesso schermo

## Causa del Problema

Il problema era causato da diversi fattori nel codice:

1. **Inizializzazione incompleta degli schermi**: Solo lo schermo superiore veniva inizializzato correttamente
2. **Sincronizzazione errata con VBlank**: Le chiamate a `gfxSwapBuffers()` e `gspWaitForVBlank()` erano nell'ordine sbagliato
3. **Swap ridondanti nei loop di input**: Ogni loop di conferma chiamava `gfxFlushBuffers()` e `gfxSwapBuffers()` anche quando non c'era nulla di nuovo da disegnare
4. **Double buffering problematico**: Il double buffering causava sovrapposizione di contenuti tra i buffer front e back
5. **Doppia chiamata a hidScanInput()**: Veniva chiamato sia nel main loop che in handleInput()
6. **Console clear inefficace**: `consoleClear()` non puliva correttamente lo schermo

## Correzioni Applicate

### 1. Inizializzazione corretta di entrambi gli schermi + Disabilitazione Double Buffering

**Prima:**
```c
gfxInitDefault();
consoleInit(GFX_TOP, NULL);
```

**Dopo:**
```c
gfxInitDefault();

// Disable double buffering to prevent overlapping content
gfxSetDoubleBuffering(GFX_TOP, false);
gfxSetDoubleBuffering(GFX_BOTTOM, false);

// Initialize both screens as static globals
static PrintConsole topScreen, bottomScreen;
consoleInit(GFX_TOP, &topScreen);
consoleInit(GFX_BOTTOM, &bottomScreen);
consoleSelect(&topScreen);
```

### 2. Pulizia corretta dello schermo con ANSI escape codes

**Prima:**
```c
void drawUI() {
    consoleClear();
    printf("\x1b[0;0H");
    // ...
}
```

**Dopo:**
```c
void drawUI() {
    consoleSelect(&topScreen);  // Ensure we're on top screen
    printf("\x1b[2J");           // Clear entire screen
    printf("\x1b[H");            // Move cursor to home (0,0)
    // ...
}
```

### 3. Rimozione della doppia chiamata a hidScanInput()

**Prima:**
```c
void handleInput() {
    hidScanInput();  // Chiamato anche nel main loop!
    u32 kDown = hidKeysDown();
    // ...
}
```

**Dopo:**
```c
void handleInput() {
    // Input già scannerizzato nel main loop
    u32 kDown = hidKeysDown();
    // ...
}
```

### 4. Riordinamento del loop principale

**Prima:**
```c
while (aptMainLoop() && running) {
    hidScanInput();
    drawUI();
    handleInput();
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();  // VBlank DOPO lo swap
}
```

**Dopo:**
```c
while (aptMainLoop() && running) {
    gspWaitForVBlank();  // VBlank PRIMA di tutto
    hidScanInput();
    drawUI();
    handleInput();
    gfxFlushBuffers();
    gfxSwapBuffers();
}
```

### 5. Rimozione degli swap ridondanti nei loop di input

Tutti i loop di conferma (backup, selezione path, conferma finale) sono stati ottimizzati per:
- Disegnare il menu UNA sola volta prima del loop
- Fare swap SOLO quando necessario
- Attendere VBlank all'inizio del loop invece che alla fine

**Esempio - Prima:**
```c
while (aptMainLoop()) {
    hidScanInput();
    if (kDown & KEY_A) break;
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
}
```

**Dopo:**
```c
gfxFlushBuffers();
gfxSwapBuffers();

while (aptMainLoop()) {
    gspWaitForVBlank();
    hidScanInput();
    if (kDown & KEY_A) break;
}
```

### 6. Ottimizzazione del loop di selezione path

Il loop che permette di selezionare un path alternativo per il backup è stato riordinato per:
- Attendere VBlank all'inizio
- Gestire input
- Ridisegnare solo dopo aver elaborato l'input

## Risultato Atteso

Dopo queste correzioni:
- ✅ **Nessun flickering** - sincronizzazione corretta con il refresh dello schermo
- ✅ **Schermo inferiore inizializzato** - può essere utilizzato in future implementazioni
- ✅ **UI pulita** - nessuna sovrapposizione di contenuti
- ✅ **Prestazioni migliorate** - meno operazioni grafiche ridondanti
- ✅ **Frame rate stabile** - sincronizzazione corretta con VBlank (60 FPS)
- ✅ **Nessun testo duplicato** - double buffering disabilitato elimina la sovrapposizione
- ✅ **Input più responsivo** - eliminata la doppia scansione dell'input

## Principali Modifiche Tecniche

1. **Single Buffering**: Disabilitato il double buffering con `gfxSetDoubleBuffering(GFX_TOP, false)` per evitare sovrapposizioni
2. **Console Globali**: Le `PrintConsole` sono ora variabili statiche globali accessibili da tutte le funzioni
3. **Clear Screen Migliorato**: Uso di ANSI escape codes `\x1b[2J` e `\x1b[H` invece di `consoleClear()`
4. **Console Selection**: Ogni `drawUI()` chiama `consoleSelect(&topScreen)` per assicurarsi di disegnare sullo schermo corretto
5. **VBlank First**: Il loop principale attende VBlank all'inizio, non alla fine

## Test Consigliati

1. Avviare l'applicazione sul Nintendo 3DS
2. Verificare che lo schermo superiore mostri la lista titoli senza flickering
3. Verificare che lo schermo inferiore sia inizializzato (potrebbe mostrare uno schermo nero ma non è più "spento")
4. Navigare nella lista con il D-Pad - il movimento dovrebbe essere fluido
5. Entrare nei menu di conferma - nessun flickering dovrebbe apparire
6. Selezionare un path alternativo - il menu dovrebbe aggiornarsi senza flickering

## Note Tecniche

La sincronizzazione con VBlank è fondamentale per:
- Evitare tearing (lacerazione dell'immagine)
- Garantire che lo swap dei buffer avvenga durante l'intervallo di blanking verticale
- Mantenere un frame rate stabile a 60 FPS

L'ordine corretto è sempre:
1. `gspWaitForVBlank()` - attende il periodo di blanking
2. Input e logica di gioco
3. Rendering (printf, draw, ecc.)
4. `gfxFlushBuffers()` - invia i dati al GPU
5. `gfxSwapBuffers()` - scambia i buffer front/back

## File Modificati

- `source/main.c` - Tutte le correzioni sono state applicate a questo file

## Data di Implementazione

2 Febbraio 2026
