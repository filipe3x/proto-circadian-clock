# Bullet Points a seguir no hardware

- **KiCad READ ONLY** — `product/kicad/` é só referência de leitura, NUNCA alterar. Pode estar desatualizado.
- **Componentes JLCPCB** — sugerir sempre **Basic** ou **Promo Extended** para minimizar custos.
- **Ligações pin-a-pin** — consultar esquemático + documentação da rede, sugerir ligação pin a pin com pitfalls.
- **Footprints em falta** — fornecer comando: `python3 -m easyeda2kicad --symbol --footprint --3d --lcsc_id=CXXXXXX`
- **Instruções KiCad passo a passo** — distância entre componentes, espessura dos traces, decoupling capacitors.
- **Reutilizar componentes** — evitar taxa extra de assembly de novo reel na JLCPCB.
- **Modo Tutor** — quando >3 componentes com vários pins, avançar passo a passo com autorização.
- **Firmware ESP32** — dar exemplos de código com bibliotecas ESP32, assumir **ESP32-E 8MB**, ignorar Adafruit Matrix S3.

---

# Proto Circadian Clock

## Project Overview

Uma única PCB com dupla função:

1. **Produto — Proto Circadian Clock:** Relógio LED que simula o ciclo solar natural para regulação do ritmo circadiano. Usa painel P10 32×16 RGB (HUB75) com cores calculadas a partir da elevação solar astronómica (1500K–6500K).

2. **DevKit — Arduino Development Board:** A mesma placa funciona como kit de desenvolvimento ESP32 com ecrã OLED 128×64 (SSD1306, I2C), 4 botões táteis (A/B/L/R), buzzer e painel RGB — tudo acessível via Arduino framework.

### Input Mode — Partilha de GPIOs

Os GPIOs 34, 35 e 39 são partilhados entre dois modos de input, selecionados por flag de compilação:

```cpp
#define INPUT_MODE 0  // INPUT_CLICK_WHEEL  — encoder magnético AS5600 (brilho + modo)
#define INPUT_MODE 1  // INPUT_DEVKIT_BUTTONS — 4 botões táteis (A/B/L/R)
```

| GPIO/Bus | `INPUT_CLICK_WHEEL` (0) | `INPUT_DEVKIT_BUTTONS` (1) |
|----------|-------------------------|----------------------------|
| I2C (21/22) | AS5600 @ 0x36 (rotação) | — |
| 34   | _(livre)_               | BTN_A (confirmar)          |
| 35   | _(livre)_               | BTN_B (cancelar)           |
| 36   | _(livre — btn extra)_   | BTN_L (esquerda)           |
| 39   | ENCODER_BTN (click)     | BTN_R (direita)            |

O AS5600 comunica via I2C (endereço 0x36), partilhando o barramento com RTC (0x68) e OLED (0x3C). O botão central (GPIO 39) usa pull-up 10kΩ + active LOW. Ver `DEVKIT_DISPLAY_BUTTONS.md` e `product/CLICK_WHEEL.md`.

## Tech Stack

- **Platform:** ESP32 Dev Module (Arduino framework)
- **Display (produto):** P10 32×16 RGB LED panel (HUB75 interface, 1/4 scan)
- **Display (devkit):** SSD1306 0.96" 128×64 OLED (I2C, endereço 0x3C)
- **RTC:** DS3231 (optional, I2C, endereço 0x68)
- **Input:** Click wheel magnético (AS5600, I2C) OU 4 botões táteis (seleção por `INPUT_MODE`)
- **Audio:** Buzzer piezo passivo (GPIO 18, via MMBT2222A)
- **Language:** C++ (Arduino)

## Key Files

- `clock.ino` - Main application (single-file Arduino sketch)
- `board_config.h` - GPIO pinout e compilação condicional (Matrix Portal S3 / ESP32 Dev Module)
- `auto_solar.h/.cpp` - Motor de cálculo solar
- `sound.h/.cpp` - Engine de áudio chiptune
- `captive_portal.h/.cpp` - Portal WiFi para configuração
- `DEVKIT_DISPLAY_BUTTONS.md` - Design do ecrã OLED + botões (bottom side)
- `product/CLICK_WHEEL.md` - Design do click wheel magnético (AS5600) iPod-style

## Dependencies

```cpp
// Produto (Proto Circadian Clock)
#include <P10_32x16_QuarterScan.h>  // Custom wrapper: github.com/filipe3x/P10_32x16_QuarterScan
#include <RTClib.h>                  // Adafruit RTC library
#include <WiFi.h>                    // ESP32 built-in
#include <esp_now.h>                 // ESP-NOW mesh protocol
#include <esp_wifi.h>                // WiFi channel control
#include <Preferences.h>             // NVS persistent storage
#include <Dusk2Dawn.h>               // Sunrise/sunset calculations

// DevKit (ecrã OLED)
#include <Adafruit_SSD1306.h>        // OLED driver (I2C)
#include <Adafruit_GFX.h>            // Graphics primitives
```

## Hardware Configuration

### Pinout Completo (ESP32 Dev Module)
```
── HUB75 (Painel P10) ──────────────────
GPIO 4  → LAT          GPIO 16 → CLK
GPIO 5  → C            GPIO 17 → D
GPIO 12 → G2           GPIO 19 → B
GPIO 13 → B2           GPIO 23 → A
GPIO 14 → R2           GPIO 25 → R1
GPIO 15 → OE           GPIO 26 → G1
                        GPIO 27 → B1

── I2C (RTC + OLED + AS5600) ───────────
GPIO 21 → SDA (DS3231 @ 0x68, SSD1306 @ 0x3C, AS5600 @ 0x36)
GPIO 22 → SCL

── Input (ver INPUT_MODE) ──────────────
GPIO 34 → BTN_A              (pull-up 10kΩ externo, exclusivo devkit)
GPIO 35 → BTN_B              (pull-up 10kΩ externo, exclusivo devkit)
GPIO 36 → BTN_L              (pull-up 10kΩ externo, exclusivo devkit)
GPIO 39 → ENCODER_BTN / BTN_R (pull-up 10kΩ externo)

── Outros ──────────────────────────────
GPIO 0  → BOOT Button (INPUT_PULLUP)
GPIO 2  → LED_BUILTIN
GPIO 18 → Buzzer (via Q1 MMBT2222A)
GPIO 33 → VBUS_SENSE (ADC, deteção 20V)
GPIO 1  → DEBUG TX
GPIO 3  → DEBUG RX

── Reserva ─────────────────────────────
GPIO 32 → Livre (I2S DIN / expansão futura)
```

### BOM — Componentes por Configuração

**Base (sempre presente na PCB):**

| Ref | Componente | Qtd | JLCPCB | Tipo | Notas |
|-----|-----------|-----|--------|------|-------|
| U1 | ESP32-WROOM-32E 8MB | 1 | C2934560 | Extended | MCU principal |
| U2 | DS3231SN (RTC) | 1 | C9866 | Extended | I2C 0x68 |
| U3 | CH340C (USB-UART) | 1 | C84681 | Basic | Programação |
| Q1 | MMBT2222A (NPN) | 1 | C2145 | Basic | Driver buzzer |
| BZ1 | Buzzer piezo passivo | 1 | C252948 | Extended | 4-5 kHz |
| R×3 | 10kΩ 0603 | 3 | C25804 | Basic | Pull-up GPIO 34/35/39 |
| Y1 | Crystal 32.768kHz | 1 | — | — | Se RTC externo |

**Modo DevKit (`INPUT_MODE=1`) — bottom side:**

| Ref | Componente | Qtd | JLCPCB | Tipo | Notas |
|-----|-----------|-----|--------|------|-------|
| R_L | 10kΩ 0603 (pull-up BTN_L) | 1 | C25804 | Basic | SMD assembly |
| PAD×4 | Contact pads cobre exposto | 4 | — (Gerbers) | — | Sem componente — pads na PCB |
| OLED1 | SSD1306 0.96" 128×64 I2C | 1 | Módulo hand solder | — | ~$1.00 |
| J_OLED | Header fêmea 1×4 2.54mm | 1 | C124413 | Basic | Hand solder |
| DOME×4 | Rubber dome c/ carbon pill | 4 | AliExpress | — | ~$0.01/un |

**Modo Click Wheel (`INPUT_MODE=0`) — top side:**

| Ref | Componente | Qtd | JLCPCB | Tipo | Preço |
|-----|-----------|-----|--------|------|-------|
| U4 | AS5600-ASOM (encoder magnético I2C) | 1 | C526588 | Extended | ~$1.50 |
| C_AS | 100nF 0402 (decoupling AS5600) | 1 | — | Basic | ~$0.001 |
| MAG | Ímã diametral ø6×2.5mm NdFeB | 1 | AliExpress (hand mount) | — | ~$0.10 |

> **Nota:** O AS5600 usa o barramento I2C existente (GPIO 21/22) — partilhado com RTC e OLED, sem conflito de endereços. A resistência pull-up do GPIO 39 (botão central) está na base. As pull-ups dos GPIO 34/35 ficam disponíveis para o modo DevKit.

## Architecture

### Operating Modes
1. **AUTO_SOLAR** - Automatic color based on solar elevation
2. **THERAPY_RED** - Pure red light therapy mode
3. **OFF** - Display off

### Time Source Hierarchy
1. RTC DS3231 (best - precise, independent)
2. NTP via WiFi (good - requires internet)
3. ESP32 internal clock (acceptable - drifts ~1-2s/day)
4. Compile time (last resort)

### Solar Algorithm
- Calculates true solar elevation using astronomical formulas
- Maps elevation to color temperature (1500K-6500K)
- Converts Kelvin to RGB using Tanner Helland algorithm
- Applies brightness curve based on elevation

### Status Codes
- Green: RTC + WiFi + NTP (perfect)
- Blue: WiFi + NTP (good)
- Yellow: RTC + WiFi, NTP failed
- Purple: RTC only (functional offline)
- Orange: WiFi only, no valid time
- Red: Offline (critical)

## Development Guidelines

### Code Style
- Use Arduino/C++ conventions
- Section headers with `// ============= SECTION =============`
- Forward declarations before implementations
- Portuguese comments are acceptable (project origin)

### Important Patterns
- Non-blocking loop with `millis()` timing
- Button handling via interrupt (IRAM_ATTR)
- WiFi disconnects after sync to save power
- Display updates every 60s in AUTO_SOLAR mode

### Configuration Constants
```cpp
const float LATITUDE = 41.5362;      // Location
const float LONGITUDE = -8.7813;
const int TIMEZONE_OFFSET = 0;       // UTC+0 for Portugal
const int SOLAR_OFFSET_HOURS = 1;    // Shift solar cycle
```

### Testing
- Monitor Serial at 115200 baud for debug output
- Solar info logged every 5 minutes
- Boot sequence shows connectivity status

## Build & Upload

1. Board: ESP32 Dev Module
2. Flash Size: 4MB
3. Upload Speed: 921600
4. Ensure all libraries are installed via Arduino Library Manager

## Mesh Sync (Multi-Device)

Multiple Proto Circadian Clocks can automatically synchronize via ESP-NOW:

- **Device ID:** Unique eFuse MAC (48-bit, factory-programmed)
- **Master Election:** Oldest device (by first boot timestamp) becomes master
- **Sync:** Mode changes propagate instantly (<50ms) to all peers
- **Power Saving:** Hybrid mode - WiFi OFF when alone (~24mA), ON with peers (~95mA)
- **Discovery:** Initial 65s scan on boot, then 3s scans every 60s when alone

See `MeshSync.md` for detailed protocol documentation.

## Known Limitations

- Text rendering on P10 panel handled by separate component
- Graphics operations (fillScreen, fillRect, drawPixel) work correctly
- Quarter-scan mapping handled by P10_32x16_QuarterScan wrapper
- Mesh sync requires all devices on same WiFi channel (default: 1)
