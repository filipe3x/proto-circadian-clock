# Click Wheel — Design & Implementação

## Contexto

Click wheel ao estilo iPod para o relógio circadiano. Controla brilho (rotação) e modo (botão central).

O iPod 1G era **capacitivo e completamente liso** — sem detents mecânicos. O "click" era o botão central. O prazer vinha do **momentum em software**. Este design segue o mesmo princípio: rotação fluída sem detents.

> **Nota histórica:** A versão anterior deste design usava **2× ITR8307** (sensores ópticos reflectivos IR) com um disco óptico de 96 slots (3D print/laser cut). Após avaliação mais atenta dos custos, complexidade mecânica e facilidade de montagem, a solução foi substituída pelo **AS5600** — encoder magnético absoluto via I2C. O AS5600 elimina o disco custom (~€5–15), reduz a BOM, simplifica o alinhamento mecânico (apenas um íman de 6mm no eixo) e oferece 4096 posições/revolução vs 384 PPR do óptico.

---

## Hardware

### AS5600 — Encoder magnético absoluto 12-bit

O AS5600 é um encoder rotativo magnético contactless com saída I2C, analógica ou PWM. Mede o ângulo absoluto de um íman diametricamente magnetizado colocado acima do IC.

**Características principais:**

| Parâmetro | Valor |
|-----------|-------|
| Resolução | 12-bit (4096 posições/revolução) |
| Interface | I2C (endereço fixo **0x36**) |
| Alimentação | 3.3V ou 5V |
| Consumo | ~6.5 mA típico |
| Package | SOIC-8 |
| Air gap (íman → IC) | 0.5–3 mm |
| Temperatura | -40°C a +125°C |

### Pinout AS5600 (SOIC-8)

```
        ┌────────────┐
  SDA  1│            │8  VDD (3.3V)
  SCL  2│  AS5600    │7  N.C.
  PGO  3│            │6  DIR
  GND  4│            │5  OUT (analog/PWM)
        └────────────┘
```

| Pin | Nome | Função |
|-----|------|--------|
| 1 | SDA | I2C Data (open-drain, precisa pull-up) |
| 2 | SCL | I2C Clock (open-drain, precisa pull-up) |
| 3 | PGO | Programming output (não usado — deixar flutuante) |
| 4 | GND | Ground |
| 5 | OUT | Saída analógica (0–3.3V) ou PWM — backup via ADC |
| 6 | DIR | Direcção: GND = CW incrementa, VDD = CCW incrementa |
| 7 | N.C. | Não ligado |
| 8 | VDD | Alimentação 3.3V |

### Esquemático — Typical Application

```
ESP32 GPIO 21 (SDA) ──┬── DS3231/PCF8563 SDA
                       ├── SSD1306 SDA
                       └── AS5600 pin 1 (SDA)

ESP32 GPIO 22 (SCL) ──┬── DS3231/PCF8563 SCL
                       ├── SSD1306 SCL
                       └── AS5600 pin 2 (SCL)

3.3V ──┬── AS5600 pin 8 (VDD)
       └── C_DEC 100nF ── GND

GND ─── AS5600 pin 4 (GND)
GND ─── AS5600 pin 6 (DIR)     ← CW = ângulo incrementa

AS5600 pin 5 (OUT) ── GPIO 34  ← Backup analógico (opcional, 0–3.3V ratiométrico)
AS5600 pin 3 (PGO) ── flutuante
AS5600 pin 7 (N.C.) ── flutuante
```

**Pull-ups I2C:** Já existem no barramento (4.7kΩ do módulo RTC). Não adicionar mais.

**DIR pin:** Ligado a GND → rotação CW incrementa o ângulo. Para inverter, ligar a VDD.

### Íman

| Parâmetro | Valor |
|-----------|-------|
| Tipo | **Diametricamente magnetizado** (N–S através do diâmetro) |
| Material | NdFeB (neodímio) N35 ou superior |
| Tamanho | 6mm diâmetro × 2–3mm altura (standard) |
| Air gap | 0.5–3mm entre topo do íman e topo do IC |
| Montagem | Colado no eixo do knob, centrado sobre o IC |
| Tolerância | ±0.5mm lateral (o AS5600 é tolerante) |
| Fonte | AliExpress "6x2.5mm diametric magnet" (~€0.05–0.15/un) |

> **IMPORTANTE:** O íman deve ser **diametricamente magnetizado** (campo N–S perpendicular ao eixo de rotação). Ímanes axiais (N–S paralelo ao eixo) **não funcionam**.

```
Vista lateral:                Vista topo (campo magnético):

  ┌──────────┐ Knob
  │  ┌────┐  │                    N ←──── S
  │  │íman│  │ ← 6mm Ø             (diametric)
  │  └────┘  │
  ═══════════  ← air gap 1-2mm
  ┌──────────┐
  │  AS5600  │ ← SOIC-8 no PCB
  └──────────┘
```

### BOM — Componentes Click Wheel (AS5600)

| Ref | Componente | LCSC | Tipo | Qtd | Custo/un | Notas |
|-----|-----------|------|------|-----|----------|-------|
| U_ENC | AS5600-ASOM | C183784 | Extended | 1 | ~€1.20 | SOIC-8, I2C 0x36 |
| C_ENC | 100nF 0402 | C307331 | Basic | 1 | ~€0.002 | Decoupling VDD |
| MAG1 | Íman 6×2.5mm diametric | — | Hand place | 1 | ~€0.10 | AliExpress, colado no knob |

**Custo total: ~€1.35** (vs ~€5–15 da solução óptica anterior com disco custom)

**GPIOs consumidos: 0** — reutiliza o barramento I2C existente (GPIO 21/22). O GPIO 34 pode opcionalmente receber a saída analógica do AS5600 como backup.

### Botão Central (Click)

O botão central do encoder continua no **GPIO 39** (SENSOR_VN) com pull-up externo 10kΩ — partilhado com BTN_R do modo DevKit. Pode ser um tactile switch, rubber dome, ou contacto mecânico no knob.

```
3.3V ─── 10kΩ ──┬── GPIO 39
                 │
              [SWITCH]    ← Botão central (click do knob)
                 │
                GND
```

### Compatibilidade I2C — Endereços no Bus

| Dispositivo | Endereço | Conflito? |
|-------------|----------|-----------|
| DS3231 / PCF8563 (RTC) | 0x68 / 0x51 | Não |
| SSD1306 (OLED) | 0x3C | Não |
| AS5600 (encoder) | **0x36** | Não |

Todos os endereços são únicos. O barramento I2C suporta os 3 dispositivos sem problema a 400kHz.

---

## Firmware (TO DO — ainda não implementado)

> O código abaixo é referência de design. Ainda **não foi aplicado** ao `clock.ino` / `board_config.h`.

### Registos I2C do AS5600

| Registo | Endereço | Tipo | Descrição |
|---------|----------|------|-----------|
| RAW ANGLE | 0x0C (H) + 0x0D (L) | Read | Ângulo raw 12-bit (0–4095) |
| ANGLE | 0x0E (H) + 0x0F (L) | Read | Ângulo com offset aplicado |
| STATUS | 0x0B | Read | Bit 5: magnet detected, Bit 4: too weak, Bit 3: too strong |
| AGC | 0x1A | Read | Automatic Gain Control (0–255) — indica qualidade do sinal |
| MAGNITUDE | 0x1B (H) + 0x1C (L) | Read | Magnitude do campo magnético |

### TO DO 1: board_config.h — adicionar defines do click wheel AS5600

```c
// ============================================================
// CLICK WHEEL (AS5600 Magnetic Encoder + Botão Central)
// ============================================================
// Rotação = brilho (leitura via I2C), click central = change mode
// BOOT (GPIO0) mantém-se livre para flash/debug
// AS5600 no barramento I2C existente (0x36, sem conflito)
#define AS5600_I2C_ADDR     0x36
#define AS5600_RAW_ANGLE_H  0x0C
#define AS5600_RAW_ANGLE_L  0x0D
#define AS5600_STATUS_REG   0x0B
#define AS5600_AGC_REG      0x1A

// Botão central do encoder — GPIO input-only, pull-up externo 10kΩ
#if BOARD_MATRIXPORTAL_S3
  #define ENCODER_BTN_PIN 11
#else
  #define ENCODER_BTN_PIN 39   // SENSOR_VN — click do encoder
#endif

// INPUT_MODE seleciona entre click wheel e devkit buttons (ver DEVKIT_DISPLAY_BUTTONS.md)
// Estes defines são activados automaticamente quando INPUT_MODE == INPUT_CLICK_WHEEL (0)
#define CLICK_WHEEL_ENABLED    1    // 1 = click wheel activa, 0 = só botão BOOT
#define ENCODER_INVERT_DIR     0    // 1 = inverter CW/CCW (ou trocar DIR pin)
#define BRIGHTNESS_STEP        10   // Incremento por tick do encoder
#define BRIGHTNESS_MIN         10   // Brilho mínimo
#define ENCODER_BTN_DEBOUNCE_MS 200 // Debounce do click central
#define WHEEL_POLL_INTERVAL_MS  20  // Polling I2C a 50Hz (suficiente para rotação humana)
#define WHEEL_ANGLE_THRESHOLD   30  // Ângulo mínimo (de 4096) para registar 1 tick
```

### TO DO 2: clock.ino — variáveis globais e leitura I2C

```c
// ============= CLICK WHEEL (AS5600 Magnetic Encoder) =============
#if CLICK_WHEEL_ENABLED
#include <Wire.h>

int           lastAngle         = -1;    // Último ângulo lido (-1 = não inicializado)
int           accumAngle        = 0;     // Ângulo acumulado desde último tick
unsigned long lastWheelPollMs   = 0;     // Timestamp do último polling
volatile bool encoderBtnPressed = false;
volatile unsigned long encoderBtnLastMs = 0;

// Lê ângulo raw 12-bit do AS5600 via I2C
int readAS5600Angle() {
  Wire.beginTransmission(AS5600_I2C_ADDR);
  Wire.write(AS5600_RAW_ANGLE_H);
  if (Wire.endTransmission(false) != 0) return -1;  // NACK → sensor ausente

  Wire.requestFrom((uint8_t)AS5600_I2C_ADDR, (uint8_t)2);
  if (Wire.available() < 2) return -1;

  int high = Wire.read();
  int low  = Wire.read();
  return ((high & 0x0F) << 8) | low;  // 12-bit: 0–4095
}

// Verifica se o íman está detectado
bool as5600MagnetOK() {
  Wire.beginTransmission(AS5600_I2C_ADDR);
  Wire.write(AS5600_STATUS_REG);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom((uint8_t)AS5600_I2C_ADDR, (uint8_t)1);
  if (Wire.available() < 1) return false;

  uint8_t status = Wire.read();
  return (status & 0x20) != 0;  // Bit 5 = MD (Magnet Detected)
}

void IRAM_ATTR encoderBtnISR() {
  unsigned long now = millis();
  if ((now - encoderBtnLastMs) >= ENCODER_BTN_DEBOUNCE_MS) {
    encoderBtnPressed = true;
    encoderBtnLastMs  = now;
  }
}
#endif
```

### TO DO 3: clock.ino — setupClickWheel()

```c
void setupClickWheel() {
  // Verificar presença do AS5600 no bus I2C
  Wire.beginTransmission(AS5600_I2C_ADDR);
  if (Wire.endTransmission() == 0) {
    Serial.println("[WHEEL] AS5600 detectado no I2C (0x36)");

    if (as5600MagnetOK()) {
      Serial.println("[WHEEL] Íman detectado — OK");
    } else {
      Serial.println("[WHEEL] AVISO: Íman não detectado (verificar posição/air gap)");
    }

    lastAngle = readAS5600Angle();
  } else {
    Serial.println("[WHEEL] ERRO: AS5600 não encontrado no I2C!");
  }

  // Botão central — GPIO 39 input-only, pull-up externo
  pinMode(ENCODER_BTN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BTN_PIN), encoderBtnISR, FALLING);

  Serial.println("[WHEEL] Click wheel AS5600 inicializada");
}
```

Chamar em `setup()`, após `Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)`.

### TO DO 4: clock.ino — handleClickWheel()

```c
void handleClickWheel() {
  // --- Polling do ângulo (não-bloqueante, 50Hz) ---
  unsigned long now = millis();
  if (now - lastWheelPollMs < WHEEL_POLL_INTERVAL_MS) return;
  lastWheelPollMs = now;

  int angle = readAS5600Angle();
  if (angle < 0 || lastAngle < 0) {
    lastAngle = angle;
    return;  // Sensor não disponível
  }

  // Calcular delta com wrap-around (0 ↔ 4095)
  int delta = angle - lastAngle;
  if (delta > 2048)  delta -= 4096;   // Wrap CW → CCW
  if (delta < -2048) delta += 4096;   // Wrap CCW → CW

  #if ENCODER_INVERT_DIR
    delta = -delta;
  #endif

  lastAngle = angle;

  // Acumular ângulo — só dispara tick quando passa o threshold
  accumAngle += delta;

  int ticks = accumAngle / WHEEL_ANGLE_THRESHOLD;
  if (ticks != 0) {
    accumAngle -= ticks * WHEEL_ANGLE_THRESHOLD;  // Manter resto

    int brightnessChange = ticks * BRIGHTNESS_STEP;
    int newBrightness = constrain(configBrightness + brightnessChange, BRIGHTNESS_MIN, MAX_BRIGHTNESS_CAP);

    if (newBrightness != configBrightness) {
      configBrightness = newBrightness;
      setSafeBrightness(configBrightness);

      // Feedback visual: barra nas 2 linhas inferiores (verde → amber → laranja)
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

      Serial.printf("[WHEEL] Brilho: %d/%d (angle=%d)\n", configBrightness, MAX_BRIGHTNESS_CAP, angle);
    }
  }

  // --- Click central: change mode (reutiliza handleButton via flag) ---
  if (encoderBtnPressed) {
    encoderBtnPressed = false;
    buttonPressed = true;  // Dispara handleButton() no próximo ciclo
  }
}
```

Chamar em `loop()`, após `handleButton()`.

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
// Em setup(), após Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN):
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
- **Sem detents** — knob liso, rotação contínua sem clicks mecânicos
- **Alta resolução** — AS5600: 4096 posições/revolução (vs 96 do disco óptico)
- **Sem desgaste** — contactless, sem peças mecânicas a degradar
- **Posição absoluta** — ao ligar, sabe imediatamente o ângulo

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
// Medir velocidade angular (delta/intervalo)
float velocity = abs(delta) / (float)WHEEL_POLL_INTERVAL_MS;

// Aceleração: quanto mais rápido, maior o step
int step = BRIGHTNESS_STEP;
if (velocity > 2.0) step *= 3;
if (velocity > 6.0) step *= 6;

// Aplicar com momentum (decaimento exponencial)
momentum = delta * step + momentum * 0.7;
configBrightness = constrain(configBrightness + (int)momentum, BRIGHTNESS_MIN, MAX_BRIGHTNESS_CAP);
```

---

## Vantagens do AS5600 vs alternativas

### vs Encoder óptico (ITR8307 + disco)

| | Óptico (anterior) | AS5600 (actual) |
|--|-------------------|-----------------|
| Resolução | 96 PPR (384 c/ quadratura) | **4096/rev (12-bit)** |
| Disco custom | Sim (3D print, ~€5–15) | **Não (íman €0.10)** |
| Alinhamento | Crítico (2 sensores + disco ±0.1mm) | **Fácil (íman ±0.5mm)** |
| GPIOs | 2 (GPIO 34, 35) | **0 (I2C partilhado)** |
| Desgaste | Possível (pó, disco) | **Nenhum (contactless)** |
| Posição absoluta | Não | **Sim** |
| Custo total | ~€5–15 (com disco) | **~€1.35** |

### vs Encoder mecânico (EC11)

| | EC11 mecânico | AS5600 |
|--|---------------|--------|
| Feel | Click-click-click (tátil) | **Fluído, sem resistência** |
| Momentum possível | Limitado pelos detents | **Sim, total** |
| Desgaste | 15k–30k ciclos | **Sem desgaste** |
| Resolução | 20–30 PPR | **4096/rev** |

### vs Encoder mecânico (EC11)

O EC11 continua a ser uma opção válida para prototipagem rápida (sem PCB custom, sem íman). O AS5600 é superior para o produto final.
