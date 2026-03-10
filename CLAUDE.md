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

ESP32-based LED clock that simulates the natural solar light cycle for circadian rhythm regulation. Uses a P10 32×16 RGB LED panel to display colors ranging from warm red (sunrise/sunset) to cool white (midday), calculated from astronomical solar elevation.

## Tech Stack

- **Platform:** ESP32 Dev Module (Arduino framework)
- **Display:** P10 32×16 RGB LED panel (HUB75 interface, 1/4 scan)
- **RTC:** DS3231 (optional, I2C)
- **Language:** C++ (Arduino)

## Key Files

- `clock.ino` - Main application (single-file Arduino sketch)

## Dependencies

```cpp
#include <P10_32x16_QuarterScan.h>  // Custom wrapper: github.com/filipe3x/P10_32x16_QuarterScan
#include <RTClib.h>                  // Adafruit RTC library
#include <WiFi.h>                    // ESP32 built-in
#include <esp_now.h>                 // ESP-NOW mesh protocol
#include <esp_wifi.h>                // WiFi channel control
#include <Preferences.h>             // NVS persistent storage
#include <Dusk2Dawn.h>               // Sunrise/sunset calculations
```

## Hardware Configuration

### Pinout (ESP32 → P10 Panel)
```
GPIO 0  → Button (INPUT_PULLUP)
GPIO 4  → LAT
GPIO 5  → C
GPIO 12 → G2
GPIO 13 → B2
GPIO 14 → R2
GPIO 15 → OE
GPIO 16 → CLK
GPIO 17 → D
GPIO 19 → B
GPIO 21 → SDA (RTC)
GPIO 22 → SCL (RTC)
GPIO 23 → A
GPIO 25 → R1
GPIO 26 → G1
GPIO 27 → B1
```

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
