# Click Wheel — Design & Implementação

## Contexto

Click wheel ao estilo iPod para o relógio circadiano. Controla brilho (rotação) e modo (botão central).

O iPod 1G era **capacitivo e completamente liso** — sem detents mecânicos. O "click" era o botão central. O prazer vinha do **momentum em software**. Este design segue o mesmo princípio: disco óptico + IR, sem detents.

---

## Hardware

### Variantes de encoder suportadas (mesmo pinout, mesma ISR)

| Hipótese | Tipo | Notas |
|----------|------|-------|
| H1 | EC11 / EC12 mecânico | Com detents — menos fluído |
| H2 | Hall Effect + anel magnético (A3144) | Sem desgaste, sem contacto |
| H3 | Encoder óptico IR (ITR8307) | **Recomendado** — fluído, preciso |

### Resolução do disco (H3)

```
24 slots  →  ok, mas "granular"
48 slots  →  bom
96 slots  →  excelente, suave       ← sweet spot
360 slots →  ultra-smooth (como mouse óptico)
```

Com quadratura (4× decoding): **96 slots → 384 PPR efectivos**.

### Sensor óptico — ITR8307

- Sensor reflectivo IR integrado (emissão + recepção)
- Polaridade com pull-up:
  - Dente (reflecte IR) → fototransístor conduz → **GPIO LOW**
  - Gap  (absorve IR)  → fototransístor ao corte → **GPIO HIGH**
- Eléctrica idêntica ao EC11 mecânico → mesma ISR para H1/H2/H3
- Se disco montado pelo avesso ou cores invertidas: `ENCODER_INVERT_DIR 1`

### BOM — Componentes JLCPCB

| Componente | LCSC | Basic/Extended | Setup fee |
|-----------|------|----------------|-----------|
| 100Ω 0402 | — | **Basic** | $0 |
| 10kΩ 0402 | — | **Basic** | $0 |
| 100nF 0402 | — | **Basic** | $0 |
| ITR8307 ×2 | C7474 | Extended (1 tipo) | **$3** |

Estratégia para $0 extra: hand-solder os ITR8307 (4 pads expostos de 1.5mm, soldável com ferro normal). AliExpress/LCSC directo: ~€0.10/unidade.

---

## Pinout

```c
// ESP32 Dev Module — ADC1, todos suportam interrupts
ENCODER_A_PIN   = GPIO32   // Canal A (ADC1_CH4)
ENCODER_B_PIN   = GPIO35   // Canal B (input-only, interrupt ok)
CENTER_BTN_PIN  = GPIO34   // Botão central (input-only, interrupt ok)

// Matrix Portal S3 — GPIOs livres no conector de expansão
ENCODER_A_PIN   = GPIO9
ENCODER_B_PIN   = GPIO10
CENTER_BTN_PIN  = GPIO11
```

---

## Firmware

### board_config.h — defines a adicionar

```c
// ============================================================
// CLICK WHEEL (Encoder Quadratura + Botão Central)
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  #define ENCODER_A_PIN    9
  #define ENCODER_B_PIN   10
  #define CENTER_BTN_PIN  11
#else
  #define ENCODER_A_PIN   32
  #define ENCODER_B_PIN   35
  #define CENTER_BTN_PIN  34
#endif

#define ENCODER_TICKS_PER_CLICK   1
#define BRIGHTNESS_STEP           10
#define BRIGHTNESS_MIN            10
#define CENTER_DEBOUNCE_MS        200
#define ENCODER_INVERT_DIR        0    // 1 = inverter CW/CCW
```

### clock.ino — variáveis globais

```c
volatile int  encoderTicks   = 0;
volatile int  lastEncoderA   = HIGH;
volatile bool centerBtnPressed  = false;
volatile unsigned long centerBtnLastMs = 0;
```

### ISR do encoder (quadratura 4×)

```c
void IRAM_ATTR encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  if (a != lastEncoderA) {
    int dir;
    if (a == HIGH) {
      dir = (b == LOW)  ? +1 : -1;
    } else {
      dir = (b == HIGH) ? +1 : -1;
    }
    #if ENCODER_INVERT_DIR
      dir = -dir;
    #endif
    encoderTicks += dir;
    lastEncoderA = a;
  }
}
```

Princípio: Canal A gera pulsos. Canal B está 90° desfasado. No flanco de A, o estado de B determina a direcção. Equivalente ao crankshaft position sensor — cada "dente" = 1 tick.

### ISR do botão central (debounce em ISR)

```c
void IRAM_ATTR centerBtnISR() {
  unsigned long now = millis();
  if ((now - centerBtnLastMs) >= CENTER_DEBOUNCE_MS) {
    centerBtnPressed = true;
    centerBtnLastMs  = now;
  }
}
```

### Setup

```c
void setupClickWheel() {
  pinMode(ENCODER_A_PIN,  INPUT_PULLUP);
  pinMode(ENCODER_B_PIN,  INPUT_PULLUP);
  pinMode(CENTER_BTN_PIN, INPUT_PULLUP);

  lastEncoderA = digitalRead(ENCODER_A_PIN);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN),  encoderISR,   CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN),  encoderISR,   CHANGE);
  attachInterrupt(digitalPinToInterrupt(CENTER_BTN_PIN), centerBtnISR, FALLING);
}
```

Chamar em `setup()`, após `attachInterrupt(BUTTON_PIN, ...)`.

### Handler (chamar em loop)

```c
void handleClickWheel() {
  // --- Rotação: ajuste de brilho ---
  int ticks = 0;
  noInterrupts();
    ticks        = encoderTicks;
    encoderTicks = 0;
  interrupts();

  if (ticks != 0) {
    int delta = ticks * BRIGHTNESS_STEP;
    int newBrightness = constrain(configBrightness + delta, BRIGHTNESS_MIN, MAX_BRIGHTNESS_CAP);

    if (newBrightness != configBrightness) {
      configBrightness = newBrightness;
      setSafeBrightness(configBrightness);

      // Feedback visual: barra na linha inferior (verde → amber → laranja)
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
    }
  }

  // --- Botão central: ciclar modo ---
  if (centerBtnPressed) {
    centerBtnPressed = false;
    currentMode = (Mode)((currentMode + 1) % 3); // AUTO_SOLAR → THERAPY_RED → OFF
    broadcastModeChange();
    showModeFeedback(currentMode);
    updateDisplay();
  }
}
```

Chamar em `loop()`, após `handleButton()`.

---

## O que torna a click wheel viciante

A diferença entre "funciona" e "não consigo parar de girar":

### Hardware
- **Sem detents** — disco liso, rotação contínua sem clicks mecânicos
- **Alta resolução** — 96+ slots no disco, quadratura 4× em software
- **Sensor rápido** — ITR8307: resposta ~10µs vs LDR: 10-200ms

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
// Medir velocidade angular (ticks/ms)
unsigned long now = millis();
float velocity = abs(ticks) / (float)(now - lastWheelMs);
lastWheelMs = now;

// Aceleração: quanto mais rápido, maior o step
int step = BRIGHTNESS_STEP;
if (velocity > 0.5) step *= 3;
if (velocity > 1.5) step *= 6;

// Aplicar com momentum (decaimento exponencial)
momentum = ticks * step + momentum * 0.7;
configBrightness = constrain(configBrightness + (int)momentum, BRIGHTNESS_MIN, MAX_BRIGHTNESS_CAP);
```

---

## Por que não LDR

| | LDR | Fototransístor (ITR8307) |
|--|-----|--------------------------|
| Tempo de resposta | 10–200 ms | ~10 µs |
| Freq. máx. fiável | ~5 Hz | >50 kHz |
| Luz ambiente | Interfere | Imune (par emissão/recepção) |
| Sinal | Analógico, sujo | Digital limpo com pull-up |
| Veredito | Perde pulsos a velocidade normal | OK |

---

## Por que não encoder mecânico (EC11)

Para um relógio circadiano decorativo onde o feel importa:

| | EC11 mecânico | Óptico (H3) |
|--|---------------|-------------|
| Feel | Click-click-click (tátil) | Fluído, sem resistência |
| Momentum possível | Limitado pelos detents | Sim, total |
| Desgaste | 15k–30k ciclos | Sem desgaste mecânico |
| Custo | €0.20–0.50 | ~€0.10 (só sensor) + disco |
| Veredito | Bom para menus | Melhor para experiência iPod |
