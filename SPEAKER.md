# SPEAKER.md — Áudio no Proto Circadian Clock

## Estado actual (clockv7)

### Hardware: BZ1 — Piezo Passivo

```
GPIO18 → R6 (1kΩ) → Q1 Base (MMBT2222A)
                      Q1 Collector → 5V_OUT → BZ1(+) (TMB12A05 / C252948)
                                               BZ1(-) → GND
                      D5 (MMBD4148SE) → back-EMF protection
```

- Componente: TMB12A05 (aliexpress C252948), frequência ressonante 4–5 kHz
- Alimentação: 5V via Q1 (não directamente do GPIO)
- Geração: **LEDC PWM** (square wave) — `BUZZER_PIN = GPIO18`
- Volume: limitado; apenas variação do duty cycle tem algum efeito em piezo

### Firmware: Motor Chiptune LSDJ-inspirado (`sound.h` / `sound.cpp`)

| Componente | Detalhes |
|---|---|
| Oscilador | LEDC channel 0, timer 0, 8-bit |
| Frame sequencer | Timer ISR a **512 Hz** (idêntico ao Game Boy) |
| Polifonia | Arpeggio por time-division (simula acordes) |
| Noise | LFSR 15-bit (Game Boy CH4) clocked ~4 kHz |
| BPM | `ticks_per_step = (512 × 60) / (bpm × 4)` |

**Formato de dados (inspirado em LSDJ):**

```
Song → Chain[] → Pattern[] → Step {note, cmd, val, inst}
```

**Comandos de efeito:**

| Cmd | Hex | Descrição |
|-----|-----|-----------|
| CMD_NONE | 0x00 | Sem efeito |
| CMD_ARP  | 0x01 | Arpeggio: `val = 0xXY` (semi1 \| semi2) |
| CMD_VIB  | 0x02 | Vibrato: `val = 0xSD` (speed \| depth) |
| CMD_DUTY | 0x03 | Duty cycle: val = 0–3 (12/25/50/75%) |
| CMD_ENV  | 0x04 | Volume envelope: `val = 0xAD` |
| CMD_POR  | 0x05 | Portamento (glide) |
| CMD_HOP  | 0x0F | Hop para step destino |

**Melodias predefinidas:**

- `snd_play_startup()` — escala pentatónica ascendente + arpejo final C-E-G
- `snd_play_alarm()` — "Aurora": 3 fases (pulso suave → melodia → alerta máximo)
- `SONG_CIRCADIAN` — tema de demonstração LSDJ-style (melodia + baixo + percussão LFSR)

---

## Upgrade planeado (clockv8) — MAX98357A I2S

### Motivação

O piezo passivo tem limitações severas de qualidade sonora. Para o alarme circadiano gradual funcionar como pretendido (aurora suave), é necessário um speaker real com amplificador I2S.

### Módulo: MAX98357AEWL+T

- Amplificador I2S digital Class D, mono, 3 W @ 4 Ω
- Alimentação: 3.3 V–5 V (directo do 3V3 do ESP32 ou 5V_OUT)
- Interface: I2S digital (sem DA externo necessário)
- LCSC: **C2682619**
- Package: WLP-9 (3×3, 0.4mm pitch) — 1.595mm × 1.415mm
- **Footprint KiCad:** `Maxim_WLP-9_1.595x1.415_Layout3x3_P0.4mm_Ball0.27mm_Pad0.25mm_NSMD`
  - ✅ Usar este default do KiCad — dimensões exactas do datasheet Maxim
  - ❌ Não usar `WLP-9_1.468x1.448mm` (genérico, ligeiramente pequeno)
  - ❌ Não usar o easyeda2kicad `WLP-9_L1.4-W1.3-P0.40-R3-C3-BR` (1.4×1.3mm, pequeno)

**Estado no clockv7:** U9 presente no esquemático e PCB como **DNP** (`exclude_from_bom yes`). Footprint e traces pré-roteadas para facilitar clockv8. Não consta no BOM nem na montagem JLCPCB.

### Pinout completo MAX98357AEWL+T (WLP-9, 3×3 grid, 0.4mm pitch)

```
        Col 1      Col 2    Col 3
Row A:  SD_MODE    VDD      GAIN_SLOT
Row B:  OUT-       GND/EP   OUT+
Row C:  BCLK       DIN      LRCLK
```

| Pin | Nome | Tipo | Ligação clockv7 (DNP) | Ligação clockv8 |
|-----|------|------|----------------------|-----------------|
| A1  | SD_MODE | Shutdown / ch. select | 3V3 (always ON, L ch.) | 3V3 (always ON, L ch.) |
| A2  | VDD | Alimentação | 3V3 | 3V3 |
| A3  | GAIN_SLOT | Ganho analógico | GND (15 dB) | GND (15 dB) |
| B1  | OUT- | Saída BTL − | Pad speaker | Speaker − |
| B2  | GND / EP | GND + thermal pad | GND + vias térmicas | GND + vias térmicas |
| B3  | OUT+ | Saída BTL + | Pad speaker | Speaker + |
| C1  | BCLK | I2S bit clock | IO33 | IO33 |
| C2  | DIN | I2S data in | IO32 | IO32 |
| C3  | LRCLK | I2S word select | IO2 | IO2 |

**Notas por pino:**

- **SD_MODE (A1):** Shutdown e selecção de canal:
  - GND → Shutdown (~1 µA)
  - **3V3 → ON, canal esquerdo** ← usar isto (clockv7 DNP e clockv8 simples)
  - Float → ON, canal esquerdo (pull-up interno fraco)
  - 100kΩ → GND → canal direito
  - 1.5MΩ → VDD → mono mix (L+R)/2
  - GPIO opcional: permite shutdown por software (poupa corrente em standby)
- **VDD (A2):** 2.5V–5.5V. Usar 3V3 (disponível no clockv7). Decoupling: 100nF + 10µF o mais próximo possível.
- **GAIN_SLOT (A3):** Programação de ganho por resistor ou ligação directa:
  - GND directo → **15 dB** ← usar isto
  - Float (pull-up interno) → 12 dB
  - VDD → 9 dB
  - Resistores específicos → 6 / 3 / 0 dB
- **OUT− (B1) / OUT+ (B3):** Saída BTL (bridge-tied load). Ligar directamente ao speaker (4Ω ou 8Ω). **Não ligar ao GND.** Sem capacitor de acoplamento necessário (BTL).
- **GND/EP (B2):** Pad térmico central. Ligar a GND com **mínimo 4 vias térmicas** (0.3mm drill). Crítico para dissipar calor (3W @ 4Ω).
- **BCLK (C1):** I2S bit clock. → IO33 (libertar de VBUS_SENSE → mover para IO36 no clockv8).
- **DIN (C2):** I2S data. → IO32 (livre no clockv7).
- **LRCLK (C3):** Word select (44.1kHz ou 22.05kHz). → IO2 (liberto em clockv8, sem LED físico na PCB custom).

### Pinout I2S no ESP32 custom PCB (clockv8)

O ESP32 clássico suporta I2S em praticamente qualquer GPIO via matrix de roteamento interno. A escolha de pinos para o clockv8 é:

| GPIO | Função I2S | Notas |
|------|-----------|-------|
| **IO32** | I2S DIN (dados) | Livre no clockv7 |
| **IO33** | I2S BCLK (bit clock) | Libertar de VBUS_SENSE → mover para IO36 |
| **IO2**  | I2S LRCLK (word select) | Sem LED físico na PCB custom clockv8 |

### Questão: IO2 e o Error LED

No clockv7, `ERROR_LED_PIN = 2` (LED de diagnóstico). No clockv7, o ESP32 é genérico — **não há NeoPixel** (`HAS_NEOPIXEL = false`).

**Solução para clockv8:** Como o clockv7/v8 é uma **PCB custom**, o IO2 não tem de ter um LED físico. Basta **não rotear IO2 para um LED na PCB** (DNP — Do Not Populate). Os erros de sistema já estão reflectidos na cor do display via `currentStatus` (6 estados com cores distintas: verde/azul/amarelo/roxo/laranja/vermelho).

### VBUS_SENSE — mover IO33 → IO36

Para libertar IO33 para I2S BCLK:

```
IO36 (VP) = ADC1_CH0 = input-only, sem output
→ Perfeito para VBUS_SENSE (só leitura ADC)
```

Ajuste na PCB: redirigir o divisor resistivo (47kΩ + 5.6kΩ) de IO33 para IO36.

### Firmware — alterações para clockv8

```c
// board_config.h — secção ESP32 custom PCB
#define VBUS_SENSE_PIN      36              // movido de IO33 → IO36 (VP, ADC1_CH0)
#define VBUS_SENSE_CHANNEL  ADC1_CHANNEL_0  // IO36 = ADC1_CH0

// I2S MAX98357A
#define HAS_I2S_SPEAKER    true
#define I2S_DIN_PIN        32
#define I2S_BCLK_PIN       33
#define I2S_LRCLK_PIN      2

// Error LED — sem LED físico na PCB clockv8
// (diagnóstico via display currentStatus)
#define ERROR_LED_PIN      -1   // DNP
```

```c
// sound_i2s.h — novo módulo para clockv8
#include <driver/i2s.h>

#define I2S_PORT          I2S_NUM_0
#define I2S_SAMPLE_RATE   22050   // Hz — suficiente para alarme/voz
#define I2S_BITS          16
#define I2S_CHANNELS      1       // Mono (MAX98357A é mono)
```

### Ligação física MAX98357A

```
ESP32          MAX98357A (U9)
------         ---------------
IO32  ───────► DIN   (C2)
IO33  ───────► BCLK  (C1)
IO2   ───────► LRCLK (C3)
3V3   ───────► VDD   (A2)   + 100nF || 10µF ao GND
3V3   ─────── SD_MODE (A1)  → always ON, canal esquerdo
GND   ───────► GND/EP (B2) + vias térmicas (≥4×)
GND   ─────── GAIN_SLOT (A3)   → 15 dB
         OUT+ (B3) ──► speaker+ (4Ω ou 8Ω)
         OUT- (B1) ──► speaker-
```

---

## Comparação piezo vs I2S

| Critério | BZ1 Piezo (clockv7) | MAX98357A I2S (clockv8) |
|---------|---------------------|------------------------|
| Qualidade | Square wave, limitada | PCM 16-bit, real |
| Frequências reproduzíveis | ~1–8 kHz (ressonância 4-5 kHz) | 20 Hz – 20 kHz |
| Volume | Baixo (~70 dB) | 85–90 dB @ 3 W/4 Ω |
| Alarme gradual | Apenas variação duty/envelope | Fade-in real via amplitude PCM |
| Custo PCB | ~0.30 € (Q1 + BZ1) | ~1.50 € (módulo) + speaker |
| Complexidade firmware | LEDC + timer ISR | driver I2S (ESP-IDF) |
| Pinos GPIO | 1 (GPIO18) | 3 (DIN + BCLK + LRCLK) |

---

## Plano de implementação clockv8

1. **PCB:**
   - Mover divisor VBUS de IO33 → IO36
   - Rotear IO32/IO33/IO2 para header I2S (ou pad SMD para módulo GY-MAX98357A)
   - DNP error LED em IO2
   - Pad para speaker 4 Ω (ex: 28 mm × 28 mm, ~1 W)

2. **Firmware:**
   - Novo ficheiro `sound_i2s.cpp` com driver I2S
   - Manter `sound.cpp` (piezo) activo para clockv7 via `#if !HAS_I2S_SPEAKER`
   - Reescrever `snd_play_alarm()` com buffer PCM para fade-in suave
   - Opção: manter motor chiptune para efeitos curtos (boot beep) mesmo com I2S

3. **Compatibilidade:**
   - `board_config.h` controla qual stack de áudio compila
   - clockv7 = piezo LEDC como agora
   - clockv8 = I2S MAX98357A
