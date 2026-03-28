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

> **Decisão final (v3):** GPIO 32 foi libertado para I2S DIN (MAX98357A).
> Todos os pins do click wheel são agora **input-only** e precisam de **pullup externo 10kΩ a 3.3V**.
> BOOT (GPIO0) mantém-se livre para flash/debug — o click do encoder assume mode change.

```c
// ESP32 Dev Module — pins input-only, pullup externo 10kΩ a 3.3V
ENCODER_A_PIN   = GPIO34   // Canal A (input-only, interrupt ok)
ENCODER_B_PIN   = GPIO35   // Canal B (input-only, interrupt ok)
ENCODER_BTN_PIN = GPIO39   // SENSOR_VN — click central do encoder

// Matrix Portal S3 — GPIOs livres no conector de expansão
ENCODER_A_PIN   = GPIO9
ENCODER_B_PIN   = GPIO10
ENCODER_BTN_PIN = GPIO11
```

### Evolução do pinout (histórico de decisões)

| Versão | A | B | Click | Notas |
|--------|---|---|-------|-------|
| v1 | GPIO32 | GPIO35 | GPIO34 | Design inicial, GPIO32 com pullup interno |
| v2 | GPIO34 | GPIO35 | _(BOOT)_ | Simplificado: BOOT como mode, GPIO32 livre para I2S |
| v3 | GPIO34 | GPIO35 | GPIO39 | **Final:** click do encoder = mode, BOOT livre para flash |

---

## Firmware (TO DO — ainda não implementado)

> O código abaixo é referência de design. Ainda **não foi aplicado** ao `clock.ino` / `board_config.h`.

### TO DO 1: board_config.h — adicionar defines do click wheel

```c
// ============================================================
// CLICK WHEEL (Encoder + Botão Central)
// ============================================================
// Rotação = brilho, click central = change mode
// BOOT (GPIO0) mantém-se livre para flash/debug
// Compatível com EC11 mecânico, Hall Effect ou óptico (ITR8307)
#if BOARD_MATRIXPORTAL_S3
  #define ENCODER_A_PIN    9
  #define ENCODER_B_PIN   10
  #define ENCODER_BTN_PIN 11
#else
  // GPIO 34/35/39: input-only, precisam de pullup externo 10kΩ a 3.3V
  #define ENCODER_A_PIN   34
  #define ENCODER_B_PIN   35
  #define ENCODER_BTN_PIN 39   // SENSOR_VN — click do encoder
#endif

#define CLICK_WHEEL_ENABLED    1    // 1 = click wheel activa, 0 = só botão BOOT
#define ENCODER_INVERT_DIR     0    // 1 = inverter CW/CCW
#define BRIGHTNESS_STEP        10   // Incremento por tick do encoder
#define BRIGHTNESS_MIN         10   // Brilho mínimo
#define ENCODER_BTN_DEBOUNCE_MS 200 // Debounce do click central
```

### TO DO 2: clock.ino — variáveis globais e ISRs

```c
// ============= CLICK WHEEL (Encoder + Botão Central) =============
#if CLICK_WHEEL_ENABLED
volatile int           encoderTicks      = 0;
volatile int           lastEncoderA      = HIGH;
volatile bool          encoderBtnPressed = false;
volatile unsigned long encoderBtnLastMs  = 0;

void IRAM_ATTR encoderISR() {
  int a = digitalRead(ENCODER_A_PIN);
  int b = digitalRead(ENCODER_B_PIN);
  if (a != lastEncoderA) {
    int dir = (a == HIGH) ? ((b == LOW) ? +1 : -1)
                          : ((b == HIGH) ? +1 : -1);
    #if ENCODER_INVERT_DIR
      dir = -dir;
    #endif
    encoderTicks += dir;
    lastEncoderA = a;
  }
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

Princípio: Canal A gera pulsos. Canal B está 90° desfasado. No flanco de A, o estado de B determina a direcção. Equivalente ao crankshaft position sensor — cada "dente" = 1 tick.

### TO DO 3: clock.ino — setupClickWheel()

```c
void setupClickWheel() {
  // GPIO 34/35/39 são input-only sem pullup interno — pullup externo 10kΩ a 3.3V
  pinMode(ENCODER_A_PIN,   INPUT);
  pinMode(ENCODER_B_PIN,   INPUT);
  pinMode(ENCODER_BTN_PIN, INPUT);

  lastEncoderA = digitalRead(ENCODER_A_PIN);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN),   encoderISR,    CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN),   encoderISR,    CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BTN_PIN), encoderBtnISR, FALLING);

  Serial.println("[WHEEL] Click wheel configurada (encoder + click GPIO39)");
}
```

Chamar em `setup()`, após `attachInterrupt(BUTTON_PIN, ...)`.

### TO DO 4: clock.ino — handleClickWheel()

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

      Serial.printf("[WHEEL] Brilho: %d/%d\n", configBrightness, MAX_BRIGHTNESS_CAP);
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
// Em setup(), após attachInterrupt(BUTTON_PIN, ...):
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
