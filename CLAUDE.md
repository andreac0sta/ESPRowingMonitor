# Progetto Remoergometro BLE — CLAUDE.md

## Obiettivo
Sostituire la centralina (PM) del remoergometro Concept2 Model C con uno smartphone per atleta.
Ogni atleta connette il sensore al proprio smartphone via BLE e visualizza i dati di allenamento.

## Architettura del sistema

```
Sensore bobina (Model C)
        ↓
Circuito condizionamento (LM393)
        ↓
ESP32 DevKit V1 — firmware ESPRowingMonitor
        ↓ BLE
Smartphone atleta — WebGUI (browser Chrome Android)
```

## Hardware

### Sensore Concept2 Model C
- Bobina passiva 2 fili, ~105Ω
- Genera segnale AC ~0.5V a regime di allenamento
- 1 impulso per giro del volano (~20Hz durante la remata)
- Cadenza atleta: ~0.5Hz (modulazione del duty cycle)

### Circuito condizionamento segnale
- C1: 100nF ceramico (accoppiamento AC, blocca la DC)
- U1: LM393 comparatore (soglia ~0.15-0.20V)
- RV1: trimmer 10kΩ (regolazione soglia)
- R1: 10kΩ (divisore tensione)
- R3: 10kΩ pull-up sull'uscita LM393
- Protezione: diodo zener 3.3V sull'ingresso se bobina genera picchi elevati

### ESP32
- Board: DOIT ESP32 DEVKIT V1
- GPIO sensore: **GPIO13** (definito in `src/profiles/devkit-v1.board-profile.h`)
- Alimentazione: 3×AA alcaline → VIN (4.5V, regolatore AMS1117 onboard)
- Autonomia stimata: 1-2 anni con deep sleep tra sessioni

## Firmware: ESPRowingMonitor

### Repository
```
https://github.com/Abasz/ESPRowingMonitor
```

### Tool
- PlatformIO (VS Code extension)
- Board: `esp32doit-devkit-v1`

### Environment da usare
```
concept2modelC-devkit-v1
```
Definito in `platformio.ini`:
```ini
[env:concept2modelC-devkit-v1]
extends = devkit-v1-board
build_flags =
        ${env.build_flags}
        ${devkit-v1-board.build_flags}
        '-D ROWER_PROFILE="profiles/concept2modelC.rower-profile.h"'
```

### Profilo Concept2 Model C
File: `src/profiles/concept2modelC.rower-profile.h` — **già creato e completo**.

Parametri chiave:
```cpp
#define IMPULSES_PER_REVOLUTION 1       // bobina passiva, 1 impulso/giro
#define FLYWHEEL_INERTIA 0.1001         // kg⋅m² — da calibrare con ergometro reale
#define SPROCKET_RADIUS 1.4             // cm    — da verificare con ergometro reale
#define CONCEPT_2_MAGIC_NUMBER 2.8
#define ROTATION_DEBOUNCE_TIME_MIN 10
#define ROWING_STOPPED_THRESHOLD_PERIOD 7'000
#define GOODNESS_OF_FIT_THRESHOLD 0.90
#define LOWER_DRAG_FACTOR_THRESHOLD 75   // → 75e-6 N·m·s² dopo /1e6 in firmware
#define UPPER_DRAG_FACTOR_THRESHOLD 250  // → 250e-6 N·m·s²
#define DRAG_COEFFICIENTS_ARRAY_LENGTH 1
#define MINIMUM_POWERED_TORQUE 0
#define MINIMUM_DRAG_TORQUE 0.07
#define STROKE_DETECTION_TYPE STROKE_DETECTION_TORQUE
#define MINIMUM_RECOVERY_TIME 800
#define MINIMUM_DRIVE_TIME 400
#define IMPULSE_DATA_ARRAY_LENGTH 6
#define DRIVE_HANDLE_FORCES_MAX_CAPACITY 30
```
⚠️ I valori di FLYWHEEL_INERTIA e SPROCKET_RADIUS vanno verificati/calibrati
con l'ergometro fisico reale.

### Note sui threshold drag factor
Il firmware divide `LOWER/UPPER_DRAG_FACTOR_THRESHOLD` per 1e6 prima del confronto
(`settings.model.h` righe 23-24). Quindi i valori 75 e 250 nel profilo corrispondono
a k ∈ [75×10⁻⁶, 250×10⁻⁶] N·m·s², che è il range tipico Concept2 (drag factor 75–250).

### Upload
```bash
~/.platformio/penv/bin/pio run -e concept2modelC-devkit-v1 --target upload --upload-port /dev/ttyUSB0
```

### Monitor seriale
```bash
~/.platformio/penv/bin/pio device monitor --port /dev/ttyUSB0 --baud 1500000
```

## WebGUI

### URL
```
https://abasz.github.io/ESPRowingMonitor-WebGUI/
```
- PWA installabile, funziona offline dopo prima visita
- Si connette via **Web Bluetooth** direttamente all'ESP32
- ✅ Android Chrome — funziona
- ❌ iPhone Safari — Web Bluetooth non supportato (serve app nativa)

### Repository WebGUI
```
https://github.com/Abasz/ESPRowingMonitor-WebGUI
```

## Simulatore (per test senza ergometro fisico)

### Hardware
```
ESP32 simulatore GPIO18  ──── ESP32 monitor GPIO13
ESP32 simulatore GND     ──── ESP32 monitor GND
```

### Progetto PlatformIO
- Cartella: `~/git/simulatoreremoergometro/`
- Board: `esp32doit-devkit-v1`
- Porta: `/dev/ttyUSB1`

### Upload simulatore
```bash
~/.platformio/penv/bin/pio run -e esp32doit-devkit-v1 --target upload --upload-port /dev/ttyUSB1
```

### Logica simulatore (fisica reale)
Il simulatore usa la fisica del volano, non una sinusoide:
- **Drive** (33% del ciclo, ~1s): accelerazione lineare da ~13Hz a ~22Hz
- **Recovery** (67% del ciclo, ~2s): decelerazione fisica `1/ω' = 1/ω + (k/J)·dt`

Questo produce delta-time quasi linearmente crescenti durante il recovery →
R² alto nella regressione OLS del firmware → drag factor valido → potenza calcolata.

Parametri fisici nel simulatore (devono essere coerenti con il profilo):
```cpp
J      = 0.1001f   // inerzia volano [kg·m²]
k_drag = 150e-6f   // coefficiente drag [N·m·s²]
```

### Formato seriale simulatore
Output su `/dev/ttyUSB1` @ 115200 baud, una riga per impulso:
```
timestamp_sec,period_us,freq_hz,phase
19.696,45234,22.11,D
19.741,44891,22.28,R
```

| Colonna | Esempio | Significato |
|---|---|---|
| `timestamp_sec` | `19.696` | Secondi dall'avvio ESP32 |
| `period_us` | `45234` | Tempo tra impulsi consecutivi (µs) |
| `freq_hz` | `22.11` | 1.000.000 / period_us (Hz) |
| `phase` | `D` / `R` | Drive o Recovery |

## remo-analyzer (analisi segnale simulatore)

### Cartella
```
~/git/remo-analyzer/
```

### Utilizzo
```bash
cd ~/git/remo-analyzer

# Acquisisce 30s di dati dal simulatore
python3 acquire.py --duration 30

# Visualizza il CSV più recente
python3 plot.py

# Salva come PNG
python3 plot.py --save
```

### Formato CSV prodotto
`data/YYYYMMDD_HHMMSS.csv` con colonne: `timestamp_sec,period_us,freq_hz,phase`

I CSV con 3 colonne (formato legacy senza `phase`) sono ancora compatibili.

## Stato attuale

### ✅ Completato
- Toolchain PlatformIO funzionante su Ubuntu 24.04
- Profilo `concept2modelC.rower-profile.h` creato con tutti i parametri
- Environment `concept2modelC-devkit-v1` aggiunto a `platformio.ini`
- Simulatore aggiornato con fisica reale del volano (drive/recovery espliciti)
- remo-analyzer aggiornato per formato 4 colonne con fase D/R
- Collegamento GPIO18→GPIO13 + GND verificato

### ⏳ Da completare (setup simulatore)
- Flashare firmware `concept2modelC-devkit-v1` su ESP32 #1 (USB0)
- Flashare simulatore aggiornato su ESP32 #2 (USB1)
- Verificare watt non-zero nella WebGUI con il nuovo simulatore

### ⏳ Da fare (richiede accesso all'ergometro fisico)
1. Misurare tensione AC bobina Model C con multimetro durante rotazione volano
2. Costruire circuito LM393 su breadboard
3. Verificare uscita digitale pulita con oscilloscopio/multimetro
4. Collegare all'ESP32 e testare con ergometro reale
5. Calibrare parametri profilo (FLYWHEEL_INERTIA, SPROCKET_RADIUS)
6. Valutare se fare PCB dedicato o shield per ESP32

### ⏳ Sviluppi futuri
- App nativa iOS (Web Bluetooth non supportato su Safari)
- Multi-atleta: ogni atleta con il proprio ESP32+smartphone
- Dashboard coach: visione di tutti gli atleti in tempo reale
- Storico sessioni per atleta

## Note tecniche importanti

### Perché non il jack audio
- Filtri AGC del telefono distruggono il duty cycle variabile
- Non tutti i telefoni moderni hanno il jack
- Segnale AC a 0.5V troppo basso e distorto dopo i filtri

### Perché non sensore bici BLE
- Overflow: sensori bici progettati per 2-3Hz, ergometro gira a 20Hz

### Logica del calcolo potenza
ESPRowingMonitor misura il **tempo tra impulsi consecutivi** via interrupt GPIO.
Non analizza la forma d'onda — solo i deltaT tra fronti.

Pipeline:
1. **Recovery**: regressione OLS sui delta-time → slope lineare → drag factor k
2. **Drive**: integrazione della velocità angolare → potenza = k × (ω_medio)³

Il simulatore deve generare una decelerazione **quasi-lineare** durante il recovery
(condizione necessaria per R² alto e drag factor valido). Una sinusoide non soddisfa
questa condizione → potenza = 0. La fisica reale `1/ω' = 1/ω + (k/J)·dt` sì.

Documentazione fisica: `https://github.com/laberning/openrowingmonitor/blob/main/docs/physics_openrowingmonitor.md`

## Riferimenti
- ESPRowingMonitor: https://github.com/Abasz/ESPRowingMonitor
- OpenRowingMonitor: https://github.com/JaapvanEkris/openrowingmonitor
- Physics doc: https://github.com/laberning/openrowingmonitor/blob/main/docs/physics_openrowingmonitor.md
- Sensore Model C/D: https://dibi-online.com/store/accessori-concept2/101-sensore-modello-d-e.html
- Forum Concept2: https://www.c2forum.com
