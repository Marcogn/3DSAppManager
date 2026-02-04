# Troubleshooting - Problemi di Rendering

## Sintomi Comuni

### 1. Testo sovrapposto/duplicato sullo schermo
**Possibili cause:**
- Double buffering attivo
- Console clear non funziona correttamente
- Rendering su buffer sbagliato

**Soluzioni applicate:**
- ✅ Disabilitato double buffering con `gfxSetDoubleBuffering(GFX_TOP, false)`
- ✅ Sostituito `consoleClear()` con ANSI escape codes (`\x1b[2J` + `\x1b[H`)
- ✅ Aggiunto `consoleSelect(&topScreen)` in `drawUI()`

### 2. Flickering intenso
**Possibili cause:**
- Swap dei buffer non sincronizzato con VBlank
- Troppi swap per frame
- Ordine errato delle chiamate grafiche

**Soluzioni applicate:**
- ✅ `gspWaitForVBlank()` chiamato all'inizio del loop principale
- ✅ Rimossi swap ridondanti nei loop di input
- ✅ Ordine corretto: VBlank → Input → Draw → Flush → Swap

### 3. Schermo inferiore spento
**Possibili cause:**
- Console inferiore non inizializzata

**Soluzioni applicate:**
- ✅ Inizializzato bottomScreen con `consoleInit(GFX_BOTTOM, &bottomScreen)`

### 4. Input doppio o mancante
**Possibili cause:**
- `hidScanInput()` chiamato più volte per frame
- `hidScanInput()` non chiamato

**Soluzioni applicate:**
- ✅ `hidScanInput()` chiamato una sola volta nel main loop
- ✅ Rimossa chiamata ridondante in `handleInput()`

## Diagnostica Avanzata

Se i problemi persistono, controllare:

1. **Versione libctru**: Assicurarsi di usare una versione aggiornata
   ```bash
   pacman -Q devkitARM libctru
   ```

2. **Impostazioni emulatore**: Se si usa Citra, disabilitare "Hardware Renderer"

3. **Console fisica**: Su hardware reale, assicurarsi che:
   - Il 3DS sia aggiornato all'ultima versione del firmware
   - Luma3DS (se usato) sia aggiornato
   - La SD card non sia corrotta

## Test Diagnostici

### Test 1: Verificare inizializzazione schermo
```c
// In main(), dopo consoleInit
printf("Top screen initialized\n");
gfxFlushBuffers();
gfxSwapBuffers();
gspWaitForVBlank();
sleep(2);
```

### Test 2: Verificare clear screen
```c
// In drawUI()
printf("\x1b[2J\x1b[H");  // Clear + Home
printf("Frame counter: %d\n", frameCounter++);
```

### Test 3: Disabilitare completamente lo swap
```c
// Nel main loop, commentare temporaneamente:
// gfxSwapBuffers();
```

## Parametri di Configurazione Alternativi

Se il problema persiste, provare queste alternative:

### Alternativa 1: Usare double buffering MA con clear su entrambi i buffer
```c
gfxSetDoubleBuffering(GFX_TOP, true);

// In drawUI():
u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
memset(fb, 0, 400*240*3);  // Clear framebuffer manualmente
```

### Alternativa 2: Ridurre frequenza di aggiornamento
```c
static int frameSkip = 0;

// Nel main loop:
if (++frameSkip % 2 == 0) {
    drawUI();
}
```

### Alternativa 3: Forzare VSync più forte
```c
// Dopo gfxSwapBuffers():
gspWaitForVBlank();
gspWaitForVBlank();  // Double wait
```

## Se Nulla Funziona

Ultima risorsa - modalità compatibilità massima:

```c
void drawUI() {
    // Clear TUTTO manualmente
    u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    if (fb) {
        memset(fb, 0, 400*240*3);
    }
    
    // Ricrea la console ogni frame
    consoleClear();
    consoleInit(GFX_TOP, &topScreen);
    consoleSelect(&topScreen);
    
    // ... resto del rendering
}
```

**NOTA**: Questo è molto inefficiente ma può aiutare a isolare il problema.

## Log di Debug

Per debug avanzato, aggiungere logging su SD card:

```c
FILE* debugLog = fopen("sdmc:/3ds/fast-uninstall/debug.log", "a");
if (debugLog) {
    fprintf(debugLog, "Frame %d: Before draw\n", frameNum);
    fflush(debugLog);
}
// ... draw code ...
if (debugLog) {
    fprintf(debugLog, "Frame %d: After draw\n", frameNum);
    fclose(debugLog);
}
```

## Riferimenti

- [devkitPro Graphics Documentation](https://devkitpro.org/wiki/Graphics)
- [3DS Homebrew Development Guide](https://www.3dbrew.org/)
- [libctru Console Functions](https://libctru.devkitpro.org/)
