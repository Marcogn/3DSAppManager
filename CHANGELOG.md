# Changelog - 3DS Fast Uninstall
## v4.0.0 - Migrazione citro2d/citro3d (2026-02-04)
### 🎨 Migrazione Grafica Completa
- **Rimosso**: Sistema di rendering console (gfxFlushBuffers, gfxSwapBuffers)
- **Aggiunto**: citro3d + citro2d per rendering hardware-accelerato
- **Risultato**: Zero flickering, rendering fluido e ottimizzato
### 🚀 Miglioramenti Tecnici
- **GPU PICA200**: Tutto il rendering ora gestito dalla GPU
- **Double Buffering**: Frame sincronizzati automaticamente
- **Text Buffer Dinamici**: Rendering testo ottimizzato con C2D_TextBuf
- **Redraw Intelligente**: UI si aggiorna solo quando necessario (flag `needsRedraw`)
### 📐 Struttura Rendering
```
Frame Begin (C3D_FRAME_SYNCDRAW)
├── Schermo Superiore (top target)
│   ├── Clear + Scene Begin
│   ├── Draw UI (lista titoli)
│   └── Text con citro2d
└── Schermo Inferiore (bottom target)
    ├── Clear + Scene Begin
    ├── Draw Touch Controls
    └── Text con citro2d
Frame End (sincronizzazione automatica)
```
### 🔧 Dettagli Implementazione
- **C3D_Init/C2D_Init**: Inizializzazione sistema grafico
- **C2D_CreateScreenTarget**: Render targets per entrambi gli schermi
- **C2D_TextParse/Optimize/Draw**: Pipeline di rendering testo
- **C2D_DrawRectSolid**: Elementi UI (highlight, separatori)
- **C3D_FrameBegin/End**: Gestione frame con sincronizzazione
### 📝 Makefile
- **Aggiunto**: `-lcitro2d -lcitro3d` alle librerie linkate
- **Ordine corretto**: citro2d → citro3d → ctru → m
### 📚 Documentazione
- Rimossi file temporanei e di sviluppo
- Rimossa cartella duplicata `_codeql_detected_source_root`
- Aggiornato README con informazioni sul sistema grafico
- Mantenuti solo documenti essenziali per reference tecnico
---
## v3.2.2 - Miglioramenti UI (2026-02-02)
### Funzionalità
- Fast scroll con D-Pad ←→ (page up/down)
- Ordinamento per Nome (L) o Title ID (R)
- Schermo inferiore con reminder comandi
- Percorso backup configurabile
---
## v3.2.1 - Fix Sovrapposizione (2026-02-02)
### Correzioni
- Risolto problema sovrapposizione Title ID
- Aggiunto clear completo schermo (`\x1b[2J`)
- Clear end-of-line per ogni riga (`\x1b[K`)
- Tag [UPD]/[DLC] per distinguere tipi contenuto
---
## v3.2.0 - Save Backup Completo (2026-02-02)
### Funzionalità
- Backup User Save Data
- Backup ExtData
- Backup Boss ExtData (SpotPass)
- Scelta runtime del percorso backup
- File info per ogni backup
---
## v3.0.0 - Release Iniziale
### Funzionalità Base
- Lista titoli installati
- Selezione multipla
- Disinstallazione batch
- Protezione titoli sistema
- Interfaccia dual-screen
