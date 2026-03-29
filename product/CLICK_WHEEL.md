# Click Wheel — Design & Implementação

## Contexto

Click wheel ao estilo iPod para o relógio circadiano. Controla brilho (rotação) e modo (botão central).

O iPod 1G era **capacitivo e completamente liso** — sem detents mecânicos. O "click" era o botão central. O prazer vinha do **momentum em software**. Este design segue o mesmo princípio: encoder magnético + ímã, sem detents.

> **Nota histórica:** O design original (v1) usava dois sensores ópticos ITR8307 com disco perfurado de 96 slots para gerar quadratura em GPIO 34/35. Após avaliação mais atenta, o AS5600 foi escolhido por ser mais simples (1 chip I2C vs 2 sensores + disco + 2 resistências), sem partes mecânicas de precisão (disco óptico), e com resolução 12-bit nativa (4096 posições) sem necessidade de quadratura em software. O custo é semelhante (~$1.50 vs ~$0.20 sensores + disco custom).

---

## Hardware

### Encoder magnético — AS5600

O AS5600 (ams-OSRAM) é um encoder rotativo magnético de 12 bits com interface I2C. Mede o ângulo absoluto (0–360°, resolução 0.0879°) de um ímã diametralmente magnetizado posicionado acima do chip.

| Parâmetro | Valor |
|-----------|-------|
| Resolução | 12-bit (4096 posições/volta) |
| Interface | I2C (endereço fixo **0x36**) |
| Alimentação | 3.3V (VDD: 3.0–3.6V) |
| Consumo | ~6.5mA típico |
| Air gap (ímã ↔ chip) | 0.5–3.0mm |
| Ímã recomendado | Diametralmente magnetizado, ø6mm × 2.5mm |
| Package | SOIC-8 (SOP-8) |
| Tempo de resposta | ~150µs (modo rápido I2C 400kHz) |
| Saída alternativa | Analógica (0–VDD) ou PWM no pin OUT |

### Pinout do AS5600 (SOIC-8)

```
         ┌────────┐
   SCL  1│        │8  DIR
   SDA  2│ AS5600 │7  VDD
   PGO  3│        │6  (pad GND)
   GND  4│        │5  OUT
         └────────┘
```

| Pin | Nome | Ligação |
|-----|------|---------|
| 1 | SCL | GPIO 22 (barramento I2C partilhado) |
| 2 | SDA | GPIO 21 (barramento I2C partilhado) |
| 3 | PGO | Não ligado (flutuante) — só para programação OTP |
| 4 | GND | GND |
| 5 | OUT | Não ligado (usa-se I2C para leitura) |
| 6 | (pad) | GND (pad térmico/eléctrico) |
| 7 | VDD | 3.3V (com 100nF decoupling) |
| 8 | DIR | GND (CW incrementa ângulo) ou VDD (CCW) |

### Circuito típico

```
                    3.3V
                     │
                   100nF ── GND       ← Decoupling cap (junto ao pin VDD)
                     │
         ┌───────────┼───────────────┐
         │          VDD(7)           │
         │                           │
SDA(21) ─┤ SDA(2)          DIR(8) ──┤── GND  (CW = ângulo crescente)
SCL(22) ─┤ SCL(1)          OUT(5) ──┤── N/C  (não usado)
         │                           │
         │ PGO(3) ── N/C    GND(4) ──┤── GND
         │              pad(6) ──────┤── GND
         │          AS5600           │
         └───────────────────────────┘
```

**Pull-ups I2C:** Já existentes no barramento (do módulo RTC DS3231 — 4.7kΩ). Não adicionar novos.

### Posicionamento do ímã

```
        ┌──────────┐
        │  ÍMÃN    │  ← Diametralmente magnetizado (N→S horizontal)
        │  ø6×2.5  │     Colado no eixo do knob/wheel
        └──────────┘
           ↕ 0.5–3mm air gap
        ┌──────────┐
        │  AS5600  │  ← Centrado sob o ímã (no PCB, top side)
        │  SOIC-8  │
        └──────────┘
           PCB
```

**Alinhamento:** O centro do ímã deve coincidir com o centro do IC. Tolerância lateral: ±0.25mm. O air gap ideal é ~1mm.

**Ímã recomendado:** Neodímio (NdFeB) diametralmente magnetizado, ø6mm × 2.5mm. AliExpress: ~$0.10/un em packs de 10. Pesquisar: "diametrically magnetized neodymium 6x2.5mm".

### BOM — Componentes JLCPCB

| Componente | LCSC | Basic/Extended | Setup fee | Notas |
|-----------|------|----------------|-----------|-------|
| AS5600-ASOM | C526588 | Extended (1 tipo) | **$3** | SMD assembly JLCPCB |
| 100nF 0402 | — | **Basic** | $0 | Decoupling VDD |

Componentes hand-solder:

| Componente | Fonte | Preço |
|-----------|-------|-------|
| Ímã diametral ø6×2.5mm | AliExpress | ~$0.10/un |

Footprint JLCPCB em falta? Usar:
```bash
python3 -m easyeda2kicad --symbol --footprint --3d --lcsc_id=C526588
```

---

## Pinout

> **Decisão:** O AS5600 comunica via I2C (GPIO 21/22), partilhando o barramento com RTC (0x68) e OLED (0x3C).
> O botão central (click) mantém-se no GPIO 39 (input-only, pull-up externo 10kΩ).
> GPIO 34/35 ficam **livres** no modo click wheel (eram usados pelo encoder óptico anterior).
>
> **Partilha com DevKit Buttons:** Apenas o GPIO 39 é partilhado com BTN_R do DevKit. O I2C é partilhado por natureza (multi-device). A seleção é feita por `#define INPUT_MODE` no firmware (`0` = click wheel, `1` = devkit buttons).

```c
// ESP32 Dev Module
// AS5600 no barramento I2C existente (endereço 0x36)
// I2C SDA = GPIO 21, SCL = GPIO 22 (partilhado com RTC + OLED)
#define AS5600_I2C_ADDR     0x36

// Botão central (click) — input-only, pull-up externo 10kΩ a 3.3V
#define ENCODER_BTN_PIN     39     // SENSOR_VN — click central
```

### I2C — Barramento partilhado (3 dispositivos)

| Dispositivo | Endereço I2C | Função | Conflito? |
|-------------|-------------|--------|-----------|
| DS3231 RTC | 0x68 | Relógio tempo-real | Não |
| SSD1306 OLED | 0x3C | Ecrã DevKit | Não |
| AS5600 | 0x36 | Encoder magnético | Não |

Todos coexistem no mesmo barramento I2C a 400kHz (Fast Mode) sem conflito.

---

## Firmware (TO DO — ainda não implementado)

> O código abaixo é referência de design. Ainda **não foi aplicado** ao `clock.ino` / `board_config.h`.

### TO DO 1: board_config.h — adicionar defines do click wheel

```c
// ============================================================
// CLICK WHEEL (AS5600 Encoder Magnético + Botão Central)
// ============================================================
// Rotação = brilho (leitura de ângulo via I2C)
// Click central = change mode (GPIO 39)
// BOOT (GPIO0) mantém-se livre para flash/debug
#define AS5600_I2C_ADDR       0x36
#define AS5600_RAW_ANGLE_REG  0x0C   // Registos 0x0C:0x0D (12 bits)
#define AS5600_STATUS_REG     0x0B   // Bit 5: magnet detected
#define AS5600_AGC_REG        0x1A   // Automatic gain control

#if !BOARD_MATRIXPORTAL_S3
  // GPIO 39: input-only, pull-up externo 10kΩ a 3.3V
  #define ENCODER_BTN_PIN 39   // SENSOR_VN — click do encoder
#endif

// INPUT_MODE seleciona entre click wheel e devkit buttons (ver DEVKIT_DISPLAY_BUTTONS.md)
#define CLICK_WHEEL_ENABLED    1    // 1 = click wheel activa, 0 = desactivada
#define BRIGHTNESS_STEP        10   // Incremento base por unidade de rotação
#define BRIGHTNESS_MIN         10   // Brilho mínimo
#define ENCODER_BTN_DEBOUNCE_MS 200 // Debounce do click central
#define AS5600_POLL_MS         10   // Intervalo de polling I2C (10ms = 100Hz)
#define AS5600_DEAD_ZONE       5    // Zona morta (em unidades 0-4095) para ignorar ruído
```

### TO DO 2: clock.ino — leitura I2C do AS5600

```c
// ============= CLICK WHEEL (AS5600 Encoder Magnético) =============
#if CLICK_WHEEL_ENABLED

uint16_t lastAS5600Angle    = 0;
bool     as5600Detected     = false;
unsigned long lastAS5600PollMs = 0;

// Botão central (mantém ISR como antes)
volatile bool          encoderBtnPressed = false;
volatile unsigned long encoderBtnLastMs  = 0;

void IRAM_ATTR encoderBtnISR() {
  unsigned long now = millis();
  if ((now - encoderBtnLastMs) >= ENCODER_BTN_DEBOUNCE_MS) {
    encoderBtnPressed = true;
    encoderBtnLastMs  = now;
  }
}

uint16_t readAS5600RawAngle() {
  Wire.beginTransmission(AS5600_I2C_ADDR);
  Wire.write(AS5600_RAW_ANGLE_REG);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)AS5600_I2C_ADDR, (uint8_t)2);
  uint16_t angle = ((uint16_t)(Wire.read() & 0x0F)) << 8;
  angle |= Wire.read();
  return angle;  // 0–4095
}

bool as5600MagnetDetected() {
  Wire.beginTransmission(AS5600_I2C_ADDR);
  Wire.write(AS5600_STATUS_REG);
  Wire.endTransmission(false);
  Wire.requestFrom((uint8_t)AS5600_I2C_ADDR, (uint8_t)1);
  uint8_t status = Wire.read();
  return (status & 0x20) != 0;  // Bit 5 = MD (magnet detected)
}

// Delta com wraparound handling (0→4095 ou 4095→0)
int16_t angleDelta(uint16_t current, uint16_t previous) {
  int16_t delta = (int16_t)current - (int16_t)previous;
  if (delta > 2048) delta -= 4096;   // Wraparound CW
  if (delta < -2048) delta += 4096;  // Wraparound CCW
  return delta;
}

#endif
```

### TO DO 3: clock.ino — setupClickWheel()

```c
void setupClickWheel() {
  // I2C já inicializado pelo RTC (Wire.begin(21, 22))

  // Verificar se AS5600 está presente no barramento
  Wire.beginTransmission(AS5600_I2C_ADDR);
  as5600Detected = (Wire.endTransmission() == 0);

  if (as5600Detected) {
    if (as5600MagnetDetected()) {
      Serial.println("[WHEEL] AS5600 detectado, ímã OK");
    } else {
      Serial.println("[WHEEL] AS5600 detectado, MAS ímã NÃO detectado!");
    }
    lastAS5600Angle = readAS5600RawAngle();
  } else {
    Serial.println("[WHEEL] AS5600 NÃO detectado no I2C (0x36)");
  }

  // Botão central — GPIO 39, input-only, pull-up externo
  pinMode(ENCODER_BTN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BTN_PIN), encoderBtnISR, FALLING);

  Serial.println("[WHEEL] Click wheel configurada (AS5600 I2C + click GPIO39)");
}
```

### TO DO 4: clock.ino — handleClickWheel()

```c
void handleClickWheel() {
  // --- Rotação: leitura do ângulo via I2C (polling) ---
  if (as5600Detected && (millis() - lastAS5600PollMs >= AS5600_POLL_MS)) {
    lastAS5600PollMs = millis();

    uint16_t currentAngle = readAS5600RawAngle();
    int16_t delta = angleDelta(currentAngle, lastAS5600Angle);

    // Ignorar ruído (zona morta)
    if (abs(delta) > AS5600_DEAD_ZONE) {
      lastAS5600Angle = currentAngle;

      // Converter delta angular em steps de brilho
      // 4096 posições / volta → ~11 posições por grau
      // Escalar: cada 40 unidades (~3.5°) = 1 step de brilho
      int steps = delta / 40;
      if (steps != 0) {
        int newBrightness = constrain(
          configBrightness + steps * BRIGHTNESS_STEP,
          BRIGHTNESS_MIN, MAX_BRIGHTNESS_CAP
        );

        if (newBrightness != configBrightness) {
          configBrightness = newBrightness;
          setSafeBrightness(configBrightness);

          // Feedback visual: barra nas 2 linhas inferiores
          int barWidth = map(configBrightness, BRIGHTNESS_MIN, MAX_BRIGHTNESS_CAP, 0, 32);
          uint16_t barColor;
          if (configBrightness < MAX_BRIGHTNESS_CAP / 2) {
            barColor = display->color565(0, 200, 80);
          } else if (configBrightness < MAX_BRIGHTNESS_CAP * 3 / 4) {
            barColor = display->color565(251, 191, 36);
          } else {
            barColor = display->color565(255, 60, 20);
          }
          display->fillRect(0, 14, 32, 2, display->color565(0, 0, 0));
          display->fillRect(0, 14, barWidth, 2, barColor);

          Serial.printf("[WHEEL] Brilho: %d/%d (ângulo: %d)\n",
                        configBrightness, MAX_BRIGHTNESS_CAP, currentAngle);
        }
      }
    }
  }

  // --- Click central: change mode ---
  if (encoderBtnPressed) {
    encoderBtnPressed = false;
    buttonPressed = true;  // Dispara handleButton() no próximo ciclo
  }
}
```

### TO DO 5: clock.ino — forward declarations e integração

Adicionar forward declarations:
```c
#if CLICK_WHEEL_ENABLED
void setupClickWheel();
void handleClickWheel();
#endif
```

Integrar no `setup()` e `loop()`:
```c
// Em setup(), após Wire.begin(21, 22) e attachInterrupt(BUTTON_PIN, ...):
#if CLICK_WHEEL_ENABLED
  setupClickWheel();
#endif

// Em loop(), após handleButton():
#if CLICK_WHEEL_ENABLED
  handleClickWheel();
#endif
```

---

## O que torna a click wheel viciante

A diferença entre "funciona" e "não consigo parar de girar":

### Hardware
- **Sem detents** — ímã + IC, rotação contínua sem clicks mecânicos
- **Alta resolução** — 4096 posições nativas (12-bit), sem engrenagens
- **Resposta rápida** — polling a 100Hz via I2C, ~150µs por leitura
- **Sem desgaste** — sem contacto mecânico entre sensor e peça móvel

### Software (a implementar)

```
Velocidade angular   →  incremento variável
Lenta               →  ±1 (fino)
Média               →  ±15 (médio)
Rápida (flick)      →  ±60 (salto grande)
```

**Momentum/inércia** — após soltar, o valor continua e desacelera. É isto que faz o iPod parecer mágico.

**Snap** — no final do movimento, encaixa no valor mais próximo com micro-efeito visual.

Algoritmo sugerido:
```c
// Medir velocidade angular (delta / tempo)
unsigned long now = millis();
int16_t delta = angleDelta(currentAngle, lastAS5600Angle);
float velocity = abs(delta) / (float)(now - lastWheelMs);
lastWheelMs = now;

// Aceleração: quanto mais rápido, maior o step
int step = BRIGHTNESS_STEP;
if (velocity > 50.0)  step *= 3;   // >50 unidades/ms
if (velocity > 150.0) step *= 6;   // >150 unidades/ms (flick rápido)

// Aplicar com momentum (decaimento exponencial)
momentum = (delta / 40.0) * step + momentum * 0.7;
configBrightness = constrain(configBrightness + (int)momentum, BRIGHTNESS_MIN, MAX_BRIGHTNESS_CAP);
```

---

## Vantagens do AS5600 vs alternativas

| | EC11 mecânico | AS5600 magnético |
|--|---------------|------------------|
| Feel | Click-click-click (tátil) | Fluído, sem resistência |
| Resolução | 20–30 PPR | 4096 posições (12-bit) |
| Momentum possível | Limitado pelos detents | Sim, total |
| Desgaste | 15k–30k ciclos | Sem desgaste (sem contacto) |
| Interface | 2× GPIO (quadratura + ISR) | I2C (partilhado) |
| Componentes | 1 encoder + 2 pull-ups | 1 IC + 1 cap + 1 ímã |
| Custo | ~$0.30 | ~$1.50 (IC) + ~$0.10 (ímã) |
| Veredito | Bom para menus | Melhor para experiência iPod |
