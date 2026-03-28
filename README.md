# 🌅 Proto Circadian Clock

An ESP32-powered LED clock that simulates the natural solar light cycle to support healthy circadian rhythms. The display gradually transitions from warm red tones at sunrise/sunset to bright white at midday, mimicking natural daylight patterns.

## Features

- **Automatic Solar Simulation** - Real-time color temperature based on astronomical sun position
- **Red Light Therapy Mode** - Pure red light for evening relaxation
- **Multi-WiFi Support** - Connects to multiple configured networks with automatic fallback
- **Offline Operation** - Works without internet using RTC or internal clock
- **NTP Synchronization** - Keeps time accurate via internet time servers
- **Low Power** - Disconnects WiFi after sync to save energy
- **Startup Sound & Alerts** - Retro-style 8-bit chiptune sounds on boot (can be disabled via captive portal)

## Hardware Requirements

| Component | Specification |
|-----------|---------------|
| Microcontroller | ESP32 Dev Module (4MB Flash) |
| Display | P10 32×16 RGB LED Panel (HUB75, 1/4 scan, SMD3535) |
| RTC (Optional) | DS3231 module |
| Button | Momentary push button |
| Buzzer (Optional) | MLT-5030 piezo buzzer or similar (passive, 3V, 5×5mm) |
| Power | 5V 2A minimum |

## Wiring

### Pinout Absoluto ESP32 — Todos os GPIOs

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32-WROOM-32E  PINOUT                      │
├───────┬──────────────┬──────────┬───────────────────────────────┤
│ GPIO  │ Função       │ Direção  │ Notas                        │
├───────┼──────────────┼──────────┼───────────────────────────────┤
│       │ ── HUB75 (Painel P10 RGB) ──                           │
│  4    │ LAT          │ Output   │ Latch                        │
│  5    │ C            │ Output   │ Row select bit 2             │
│ 12    │ G2           │ Output   │ Green lower                  │
│ 13    │ B2           │ Output   │ Blue lower                   │
│ 14    │ R2           │ Output   │ Red lower                    │
│ 15    │ OE           │ Output   │ Output Enable (active LOW)   │
│ 16    │ CLK          │ Output   │ Pixel clock                  │
│ 17    │ D            │ Output   │ Row select bit 3             │
│ 19    │ B            │ Output   │ Row select bit 1             │
│ 23    │ A            │ Output   │ Row select bit 0             │
│ 25    │ R1           │ Output   │ Red upper                    │
│ 26    │ G1           │ Output   │ Green upper                  │
│ 27    │ B1           │ Output   │ Blue upper                   │
├───────┼──────────────┼──────────┼───────────────────────────────┤
│       │ ── I2C (RTC + OLED) ──                                 │
│ 21    │ SDA          │ Bidi     │ DS3231 (0x68) + SSD1306 (0x3C) │
│ 22    │ SCL          │ Bidi     │ Barramento partilhado        │
├───────┼──────────────┼──────────┼───────────────────────────────┤
│       │ ── Input (partilhado — ver INPUT_MODE) ──              │
│ 34    │ ENC_A/BTN_A  │ Input    │ Pull-up 10kΩ ext. Encoder A ou Botão A │
│ 35    │ ENC_B/BTN_B  │ Input    │ Pull-up 10kΩ ext. Encoder B ou Botão B │
│ 36    │ BTN_L        │ Input    │ Pull-up 10kΩ ext. Exclusivo DevKit     │
│ 39    │ ENC_BTN/BTN_R│ Input    │ Pull-up 10kΩ ext. Enc Click ou Botão R │
├───────┼──────────────┼──────────┼───────────────────────────────┤
│       │ ── Outros ──                                           │
│  0    │ BOOT Button  │ Input    │ INPUT_PULLUP, modo cycle     │
│  2    │ LED_BUILTIN  │ Output   │ LED azul onboard             │
│ 18    │ BUZZER       │ Output   │ PWM → R 1kΩ → Q1 MMBT2222A  │
│ 33    │ VBUS_SENSE   │ Input    │ ADC1_CH5, deteção 20V        │
│  1    │ DEBUG TX     │ Output   │ UART0 TX (USB-UART)          │
│  3    │ DEBUG RX     │ Input    │ UART0 RX (USB-UART)          │
├───────┼──────────────┼──────────┼───────────────────────────────┤
│       │ ── Não disponíveis ──                                  │
│ 6-11  │ Flash SPI    │ —        │ Reservados internamente      │
│ 20    │ Não exposto  │ —        │ Indisponível na maioria dos devkits │
│ 24    │ Não existe   │ —        │ Não existe no ESP32          │
│ 28-31 │ Não expostos │ —        │ Não expostos em devkits      │
├───────┼──────────────┼──────────┼───────────────────────────────┤
│       │ ── Livre ──                                            │
│ 32    │ RESERVA      │ I/O      │ ADC1_CH4, touch — I2S DIN ou expansão │
└───────┴──────────────┴──────────┴───────────────────────────────┘
```

**Total: 28 GPIOs utilizados, 1 livre (GPIO 32), 10 indisponíveis.**

### Input Mode — GPIOs Partilhados

Os GPIOs 34, 35 e 39 servem dois modos de input (mesmo circuito elétrico):

```cpp
#define INPUT_MODE 0  // Click Wheel — encoder óptico (brilho + modo)
#define INPUT_MODE 1  // DevKit Buttons — 4 botões táteis (A/B/L/R)
```

Ver `DEVKIT_DISPLAY_BUTTONS.md` e `product/CLICK_WHEEL.md` para detalhes.

### ESP32 → P10 Panel (HUB75)

```
ESP32          P10 Panel
─────          ─────────
GPIO 25   →    R1
GPIO 26   →    G1
GPIO 27   →    B1
GPIO 14   →    R2
GPIO 12   →    G2
GPIO 13   →    B2
GPIO 23   →    A
GPIO 19   →    B
GPIO 5    →    C
GPIO 17   →    D
GPIO 4    →    LAT
GPIO 15   →    OE
GPIO 16   →    CLK
GND       →    GND
```

### I2C — RTC DS3231 + OLED SSD1306 (barramento partilhado)

```
ESP32          DS3231 (0x68)     SSD1306 (0x3C)
─────          ──────            ──────────────
GPIO 21   →    SDA          →    SDA
GPIO 22   →    SCL          →    SCL
3.3V      →    VCC          →    VCC
GND       →    GND          →    GND
```

> **Nota:** O OLED (SSD1306) é opcional, montado no verso da PCB (modo DevKit).
> Não conflitua com o RTC — endereços I2C diferentes.

### Button

```
GPIO 0    →    Button   →    GND
```

### Buzzer (Optional)

```
ESP32          Buzzer Circuit
─────          ──────────────
GPIO 18   →    R1 (1kΩ)   →   Base (NPN)
                               │
                               ├── Collector → Buzzer+ → 5V
                               │
                               └── Emitter → GND

D1 (1N4148) across Buzzer (cathode to +5V)
```

Recommended transistor: 2N2222 or BC547

**Note:** GPIO 18 is used because GPIO 6-11 are internally connected to the ESP32 flash SPI and unavailable.

## Software Dependencies

### ESP32 Board Package

In Arduino IDE, add this URL to **File → Preferences → Additional Board Manager URLs**:
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Then install **esp32** by Espressif Systems via **Tools → Board → Boards Manager**.

> ⚠️ **Important:** Use **ESP32 Arduino 2.0.x** (tested with 2.0.14), NOT version 3.x.
> The ESP32-HUB75-MatrixPanel-I2S-DMA library has compatibility issues with ESP32 Arduino 3.x
> due to breaking changes in the ESP-IDF 5.x I2S and DMA APIs. Version 2.0.x uses ESP-IDF 4.4
> which provides stable support for HUB75 LED panels.

### Arduino Libraries

Install via **Sketch → Include Library → Manage Libraries**:

| Library | Version | Author | Description |
|---------|---------|--------|-------------|
| **ESP32-HUB75-MatrixPanel-I2S-DMA** | 3.0.x | mrcodetenz (mrfaptastic) | LED matrix driver |
| **RTClib** | 2.1.x | Adafruit | RTC DS3231 support |
| **Dusk2Dawn** | 1.0.x | DM Kishi | Solar calculations |
| **ESPAsyncWebServer** | 1.2.x | me-no-dev | Async web server for captive portal |
| **AsyncTCP** | 1.1.x | me-no-dev | Async TCP for ESP32 |

### Manual Installation

The P10 panel wrapper library (not in Library Manager):

```bash
cd ~/Arduino/libraries/
git clone https://github.com/filipe3x/P10_32x16_QuarterScan.git
```

## Configuration

### Location Settings

Edit these constants in `clock.ino`:

```cpp
// Your location (for solar calculations)
const float LATITUDE = 41.5362;    // Your latitude
const float LONGITUDE = -8.7813;   // Your longitude
const int TIMEZONE_OFFSET = 0;     // Hours from UTC

// Shift the solar cycle (e.g., -1 to wake up earlier)
const int SOLAR_OFFSET_HOURS = 1;
```

### WiFi Credentials

Copy `wifi_credentials.h.example` to `wifi_credentials.h` and add your networks:

```bash
cp wifi_credentials.h.example wifi_credentials.h
```

Then edit `wifi_credentials.h`:

```cpp
WiFiCredentials wifiNetworks[] = {
  {"YourNetwork1", "password1"},
  {"YourNetwork2", "password2"},
};
```

**Note:** `wifi_credentials.h` is ignored by git to keep your passwords private.

## Installation

1. Install [Arduino IDE](https://www.arduino.cc/en/software) with ESP32 board support
2. Install required libraries (see above)
3. Open `clock.ino`
4. Select board: **ESP32 Dev Module**
5. Set upload speed: **921600**
6. Configure your location and WiFi credentials
7. Upload to ESP32

## Usage

### Operating Modes

Press the button (GPIO 0) to cycle through modes:

| Mode | Description |
|------|-------------|
| **AUTO_SOLAR** | Automatic color based on sun position |
| **THERAPY_RED** | Constant red light for therapy/relaxation |
| **OFF** | Display turned off |

### Startup Sound

If buzzer is installed, the clock plays a retro-style 8-bit startup melody on boot. The sound can be:

- **Enabled/Disabled** via the captive portal web interface
- **Customized** with different chiptune melodies (NES/GameBoy style)
- **Future use** as alarm/notification sounds

The buzzer uses PWM to generate classic pulse wave tones with configurable duty cycles (12.5%, 25%, 50%) for authentic retro sound.

### Status Indicators (Boot)

| Color | Status |
|-------|--------|
| 🟢 Green | RTC + WiFi + NTP synced (perfect) |
| 🔵 Blue | WiFi + NTP synced (good) |
| 🟡 Yellow | RTC + WiFi, NTP failed |
| 🟣 Purple | RTC only, offline |
| 🟠 Orange | WiFi only, no valid time |
| 🔴 Red | Fully offline |

### Solar Color Schedule (Example: Winter, Portugal)

| Time | Elevation | Color | Brightness |
|------|-----------|-------|------------|
| 06:00 | -25° | Off | 0% |
| 07:30 | -12° | Deep red | 18% |
| 08:00 | -6° | Orange | 28% |
| 09:00 | 0° | Warm orange | 40% |
| 12:00 | 30° | Bright white | 100% |
| 18:00 | 0° | Warm orange | 40% |
| 21:00 | -12° | Deep red | 18% |
| 22:00 | -18° | Off | 0% |

## Troubleshooting

### Display not working
- Check 5V power supply (minimum 2A)
- Verify all HUB75 connections
- Ensure P10_32x16_QuarterScan library is installed

### RTC not detected
- System works without RTC (uses NTP)
- Check I2C connections (SDA=21, SCL=22)
- Verify 3.3V power (NOT 5V)
- Check CR2032 battery

### WiFi not connecting
- System works offline with RTC
- Check credentials in `wifiNetworks[]`
- Increase `WIFI_TIMEOUT_MS` for slow networks

### Wrong time/colors
- Verify `LATITUDE` and `LONGITUDE`
- Check `TIMEZONE_OFFSET` (0 for Portugal/UK)
- Adjust `SOLAR_OFFSET_HOURS` if needed

## Serial Monitor

Connect at **115200 baud** to see:
- Boot sequence and connectivity status
- Solar information (sunrise/sunset times)
- Elevation and color updates every 5 minutes
- Mode changes

## Roadmap

Potential future enhancements:

- **Dawn Simulator Mode** - Gradual wake-up light 30 min before alarm
- **Brightness Adjustment** - Manual or ambient sensor-based control
- **Web Interface** - Configuration via WiFi AP mode
- **Multiple Profiles** - Different settings per day of week
- **Home Assistant Integration** - MQTT control

## Authors

- **Filipe Marques** ([@filipe3x](https://github.com/filipe3x)) - Creator and maintainer
- **Claude** (Anthropic) - AI pair programming assistant

## License

MIT License - Feel free to use and modify.

## Contributing

Pull requests welcome! Please test on actual hardware before submitting.
