# 3DS Fast Uninstall - Guida Completa v3.2

## 📋 Descrizione

Applicazione homebrew per Nintendo 3DS che permette di disinstallare rapidamente più titoli contemporaneamente, con opzioni di backup dei salvataggi.

## ✨ Caratteristiche

### Versione 3.2 (Attuale)

- ✅ **Selezione multipla** - Seleziona più titoli da disinstallare insieme
- ✅ **Backup salvataggi** - Backup automatico prima dell'uninstall
- ✅ **Ordinamento** - Ordina per Nome (L) o Title ID (R)
- ✅ **UI ottimizzata** - Zero flickering, aggiornamenti fluidi
- ✅ **Schermo inferiore** - Reminder dei comandi sempre visibile
- ✅ **Efficiente** - CPU usage ridotto del 80-95%
- ✅ **Battery friendly** - Consumi ottimizzati

## 🎮 Controlli

### Schermo Superiore (Lista Titoli)
- **D-Pad ↑/↓**: Naviga nella lista
- **A**: Toggle selezione titolo corrente
- **X**: Avvia uninstall titoli selezionati
- **L**: Ordina per Nome (alfabetico)
- **R**: Ordina per Title ID (numerico)
- **START**: Esci dall'applicazione

### Schermo Inferiore (Reminder)
Mostra sempre i comandi disponibili e il percorso di backup.

## 📦 Installazione

1. Copia `3ds-fast-uninstall.3dsx` nella cartella `/3ds/` della tua SD card
2. Avvia dal Homebrew Launcher
3. Seleziona i titoli da disinstallare
4. Conferma l'operazione

## 🔧 Configurazione

Il file di configurazione si trova in:
```
sdmc:/3ds/fast-uninstall/config.ini
```

### Opzioni disponibili:
```ini
# Percorso dove salvare i backup
backup_path=sdmc:/3ds/fast-uninstall/backups
```

### Percorsi alternativi:
- `sdmc:/3ds/fast-uninstall/backups` (default)
- `sdmc:/backups/3ds-titles`
- `sdmc:/save-backups`
- `sdmc:/3ds-backups`
- `sdmc:/backups`

## 🚀 Utilizzo

### 1. Avvio
Apri l'app dal Homebrew Launcher. Verrà caricata la lista dei titoli installati.

### 2. Navigazione
- Usa **D-Pad ↑/↓** per navigare un titolo per volta
- Usa **D-Pad ←→** per scorrimento veloce (26 titoli per volta)
- La lista mostra 26 titoli contemporaneamente

### 3. Selezione
- Premi **A** per selezionare/deselezionare i titoli
- I titoli selezionati mostrano `[X]` invece di `[ ]`

### 4. Ordinamento
- Premi **L** per ordinare alfabeticamente per nome
- Premi **R** per ordinare per Title ID
- L'header mostra la modalità corrente: `Sort: Name` o `Sort: Title ID`

### 5. Uninstall
- Premi **X** per avviare l'uninstall
- Scegli se fare backup dei salvataggi
- Conferma l'operazione
- Attendi il completamento

## ⚙️ Funzionalità Avanzate

### Backup Salvataggi
Prima di disinstallare, l'app può fare backup di:
- **Save data** - Salvataggi del gioco
- **ExtData** - Dati extra (DLC, patch)
- **Boss ExtData** - Dati SpotPass

I backup vengono salvati in:
```
[backup_path]/[TitleID]/
  ├── save/
  ├── extdata/
  └── boss_extdata/
```

### Filtri Titoli
L'app mostra solo:
- ✅ Applicazioni utente (0x00040000)
- ✅ Update/Patch (0x0004000E)
- ✅ DLC (0x0004008C)

Esclusi:
- ❌ Titoli di sistema (0x00040010, 0x00040030)
- ❌ Firmware/Moduli di sistema

## 🐛 Troubleshooting

### L'app non si avvia
- Verifica di avere Homebrew Launcher funzionante
- Controlla che il file `.3dsx` non sia corrotto

### Nomi titoli strani/corrotti
- L'app sanitizza automaticamente i caratteri non-ASCII
- I caratteri UTF-8 (accenti, simboli) vengono rimossi
- Esempio: `Pokémon™` → `Pokemon`

### Performance
Se l'app è lenta:
- Riavvia il 3DS
- Chiudi altre applicazioni homebrew
- Verifica spazio libero su SD card

### Backup fallito
- Controlla spazio libero su SD
- Verifica percorso backup in config.ini
- Alcuni titoli potrebbero non avere salvataggi

## 📊 Specifiche Tecniche

### Requisiti
- **Nintendo 3DS** (Old/New/2DS)
- **CFW**: Luma3DS o simile
- **Homebrew Launcher**: Accesso homebrew attivo
- **SD Card**: Spazio sufficiente per backup

### Prestazioni
- **CPU Usage**: 5% idle, 20% attivo
- **Memory**: ~2MB RAM
- **Battery**: Drain minimo (~12%/ora)
- **Load Time**: 2-5 secondi per 100+ titoli

### Limitazioni
- **Max titoli**: 300 (limite array)
- **Max backup size**: 100MB per file
- **Titoli system**: Non mostrati/gestibili

## 🔄 Changelog

### v3.2 (Attuale)
- Aggiunto ordinamento per Nome/ID con L/R
- Schermo inferiore con reminder comandi
- Migliorato effetto aggiornamento (no flash)
- UI più compatta con più spazio per lista

### v3.1
- Risolto flickering massiccio (dirty flag pattern)
- Risolti caratteri corrotti (ASCII sanitization)
- Ridotto CPU usage del 80-95%
- Migliorata battery life del ~70%

### v3.0
- Selezione multipla titoli
- Backup salvataggi automatico
- Supporto SD e NAND
- Interfaccia utente completa

## 🔗 Link Utili

### Repository
- GitHub: [3ds-fast-uninstall](https://github.com/Marcogn/3ds-fast-uninstall)

### Documentazione Tecnica
- `docs/FLICKERING_FIX.md` - Correzioni rendering
- `docs/CHARACTER_CORRUPTION_FIX.md` - Fix caratteri
- `docs/RENDERING_TROUBLESHOOTING.md` - Debug avanzato

### Build
- Compilato con: devkitARM + libctru
- Build system: Makefile standard devkitPro

## ⚠️ Disclaimer

**ATTENZIONE**: Questa applicazione disinstalla permanentemente i titoli selezionati.
- ✅ Fai sempre backup dei salvataggi importanti
- ✅ Verifica bene cosa stai disinstallando
- ✅ Non disinstallare titoli di sistema
- ⚠️ L'autore non è responsabile per perdita di dati

## 📝 Crediti

Sviluppato da: Marco Gnesutta
Librerie: libctru (devkitPro)
Ispirato da: FBI, Checkpoint

## 📄 Licenza

Vedi file `LICENSE` per dettagli.

---

**Versione**: 3.2  
**Data**: Febbraio 2026  
**Stato**: Stabile  
**Support**: GitHub Issues
