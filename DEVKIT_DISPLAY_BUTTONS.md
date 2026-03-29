# DevKit Display + Buttons — Ecrã I2C e 4 Botões no Verso da PCB

## Conceito

Transformar a PCB do Proto Circadian Clock num **kit de desenvolvimento Arduino** adicionando no lado oposto (bottom side):

- **1x Ecrã OLED monocromático** (I2C, partilha SDA/SCL com o RTC DS3231)
- **4x Copper contact pads** (A, B, L, R) com borrachas condutivas — estilo gamepad, perfil baixo, ideal para case 3D printed

OLED hand-soldered via header. Botões sem componentes a soldar — pads de cobre expostos na PCB + rubber domes (borracha condutiva) pressionados pelo case 3D printed.

> **Compatibilidade com Click Wheel:** O botão R partilha o GPIO 39 com o click central do encoder magnético (AS5600) documentado em `product/CLICK_WHEEL.md`. O AS5600 comunica via I2C (endereço 0x36), partilhando o barramento com RTC e OLED — sem conflito. Os GPIOs 34/35 ficam livres no modo click wheel. Uma flag de compilação (`INPUT_MODE`) seleciona o firmware adequado. Ver [secção 8](#8-partilha-de-pinos-com-click-wheel) para detalhes.

---

## 1. GPIOs Disponíveis (ESP32 Dev Module)

### Pinos Ocupados

| GPIO | Função           | GPIO | Função           |
|------|------------------|------|------------------|
| 0    | BOOT Button      | 17   | HUB75 D          |
| 1    | DEBUG TX         | 18   | Buzzer           |
| 2    | LED_BUILTIN      | 19   | HUB75 B          |
| 3    | DEBUG RX         | 21   | I2C SDA          |
| 4    | HUB75 LAT       | 22   | I2C SCL          |
| 5    | HUB75 C          | 23   | HUB75 A          |
| 6-11 | Flash SPI (N/A)  | 25   | HUB75 R1         |
| 12   | HUB75 G2         | 26   | HUB75 G1         |
| 13   | HUB75 B2         | 27   | HUB75 B1         |
| 14   | HUB75 R2         | 33   | VBUS_SENSE (ADC) |
| 15   | HUB75 OE         |      |                  |
| 16   | HUB75 CLK        |      |                  |

### Pinos Livres

| GPIO | Tipo           | Notas                              |
|------|----------------|------------------------------------|
| **32** | Input/Output | ADC1_CH4, touch, totalmente livre |
| **34** | Input only   | ADC1_CH6, sem pull-up interno     |
| **35** | Input only   | ADC1_CH7, sem pull-up interno     |
| **36** | Input only   | SVP, ADC1_CH0, sem pull-up interno|
| **39** | Input only   | SVN, ADC1_CH3, sem pull-up interno|

**Total: 5 GPIOs livres** — 4 para botões, 1 de reserva.

> **Nota:** GPIO 34-39 são input-only e **não têm pull-up interno**. Precisam de resistências pull-up externas (10kΩ) na PCB.

---

## 2. Ecrã OLED — SSD1306 0.96" 128x64 I2C

### Porquê SSD1306?

| Critério              | SSD1306 0.96"          |
|-----------------------|------------------------|
| Interface             | I2C (2 fios, partilha com RTC) |
| Resolução             | 128×64 pixels          |
| Endereço I2C          | 0x3C (default) — sem conflito com DS3231 @ 0x68 |
| Alimentação           | 3.3V ou 5V             |
| Preço JLCPCB          | ~$0.80-1.50 (módulo)   |
| Tamanho               | 27×27mm (ecrã ativo)   |
| Biblioteca Arduino    | Adafruit SSD1306 ou U8g2 (bem suportadas) |
| Consumo               | ~20mA típico           |

### Alternativas Consideradas

| Ecrã            | Pros                    | Contras                     |
|-----------------|-------------------------|-----------------------------|
| SSD1306 0.91" 128×32 | Mais pequeno, barato | Menos resolução vertical  |
| SH1110 1.3" 128×64   | Maior, melhor leitura | Mais caro, ocupa espaço   |
| ST7567 128×64 COG     | Sem moldura, fino     | Mais complexo, SPI melhor |

**Recomendação:** SSD1306 0.96" 128×64 — melhor relação preço/funcionalidade/disponibilidade.

### Ligação Elétrica

```
ESP32 GPIO 21 (SDA) ──── SDA  ┐
ESP32 GPIO 22 (SCL) ──── SCL  ├── OLED SSD1306 (0x3C)
3.3V ────────────────── VCC   │
GND ─────────────────── GND   ┘

(Mesmo barramento I2C que o DS3231 @ 0x68 — sem conflito)
```

**Pull-ups I2C:** O módulo DS3231 típico já inclui pull-ups de 4.7kΩ. A maioria dos módulos OLED também inclui. Se ambos tiverem, ficam em paralelo (~2.35kΩ) — funciona bem a 100/400kHz. Se usar OLED bare (sem módulo), adicionar 4.7kΩ a SDA e SCL.

---

## 3. Botões — 4x Copper Contact Pads + Rubber Domes (A, B, L, R)

### Conceito

Em vez de tactile switches soldados, cada botão é um par de **pads de cobre expostos** (sem solder mask) na PCB. Uma **borracha condutiva (rubber dome com carbon pill)** faz ponte entre os dois pads quando pressionada. É o mesmo princípio usado em comandos de TV, GameBoy, e calculadoras.

```
   Case 3D printed
   ┌─────────────────┐
   │   ┌────────┐    │  ← Botão moldado no case (guiado, solto)
   │   │  cap   │    │
   └───┤        ├────┘
       └───┬────┘
       ┌───▼────┐
       │ RUBBER │        ← Dome de borracha condutiva (carbon pill na base)
       │  DOME  │
   ────┤●  gap ●├─── PCB (bottom side)
       pad1   pad2       ← Dois pads cobre expostos, sem solder mask
```

**Vantagens para case 3D printed:**
- Perfil ultra-baixo (~1-2mm vs ~9.5mm de um tactile switch)
- Sem componentes a soldar nos botões — só as resistências pull-up
- Feel customizável pela dureza/forma da borracha
- Silencioso (sem click mecânico)
- O case 3D imprime os botões integrados ou como peças soltas guiadas

### Atribuição de Pinos

| Botão | GPIO | Tipo       | Partilhado com Click Wheel? | Função Sugerida    |
|-------|------|------------|-----------------------------|--------------------|
| **A** | 34   | Input only | Não — livre no click wheel  | Confirmar / Select |
| **B** | 35   | Input only | Não — livre no click wheel  | Cancelar / Back    |
| **L** | 36   | Input only | Não — exclusivo DevKit      | Esquerda / Menos   |
| **R** | 39   | Input only | Sim — ENCODER_BTN           | Direita / Mais     |

> GPIO 32 fica de reserva para expansão futura (ex: I2S DIN para MAX98357A, sensor adicional).
>
> **Nota:** Apenas o GPIO 39 é partilhado com a click wheel (`product/CLICK_WHEEL.md`) — o click central do encoder. O AS5600 (encoder magnético) comunica via I2C (0x36), não usa GPIOs dedicados. A seleção é feita por `#define INPUT_MODE` no firmware.

### Circuito por Botão

```
3.3V ─── R (10kΩ) ──┬── GPIO 34/35/36/39
                     │
                   pad 1 ┐
                          ├── rubber dome (ponte condutiva quando premido)
                   pad 2 ┘
                     │
                    GND
```

- **Repouso:** Pull-up mantém GPIO HIGH (3.3V). Os pads não se tocam.
- **Premido:** Carbon pill na base do rubber dome faz ponte pad1↔pad2 → GPIO puxa para GND → lê LOW.

**Importante:** GPIO 34-39 **não têm pull-up interno**, portanto as resistências de 10kΩ são obrigatórias. Sem elas, o pino flutua quando o contacto está aberto.

### Debounce

Rubber domes têm transição mais suave que switches mecânicos — menos bounce. Debounce por software (20-50ms) no firmware é suficiente. Não são necessários condensadores.

---

## 4. BOM (Bill of Materials)

### Componentes na PCB (assembly JLCPCB)

| # | Componente | Qtd | JLCPCB Part | Tipo | Notas |
|---|-----------|-----|-------------|------|-------|
| 1 | Resistência 10kΩ 0603 | 4 | C25804 | Basic | Pull-up para GPIO 34/35/36/39 |

> **Nota:** Os contact pads dos botões são apenas cobre exposto na PCB — não há componente a montar. A JLCPCB fabrica os pads automaticamente ao produzir a PCB (são áreas sem solder mask definidas nos Gerbers).

### Componentes hand-solder (comprados à parte)

| # | Componente | Qtd | Fonte | Preço Est. |
|---|-----------|-----|-------|------------|
| 2 | OLED 0.96" 128×64 SSD1306 I2C | 1 | AliExpress / LCSC | ~$1.00 |
| 3 | Header fêmea 1×4 2.54mm | 1 | C124413 (JLCPCB) ou AliExpress | ~$0.05 |
| 4 | Rubber dome pads 6-8mm (com carbon pill) | 4 | AliExpress "conductive rubber dome button" | ~$0.50/pack 50un |

### Custo Estimado por Placa

| Item              | Custo   |
|-------------------|---------|
| 4× Resistências   | ~$0.01  |
| OLED SSD1306      | ~$1.00  |
| Header 1×4        | ~$0.05  |
| 4× Rubber domes   | ~$0.04  |
| **Total**         | **~$1.10** |

> Sem tactile switches. Sem condensadores de debounce. O custo de "botões" é essencialmente $0 na PCB — só os rubber domes comprados à parte (~$0.01/unidade em packs).

---

## 5. Implementação no KiCad

### 5.1 Esquemático

1. **Adicionar símbolos:**
   - `SSD1306` (ou genérico `OLED_I2C_128x64`) — ligar SDA, SCL, VCC, GND
   - 4× `SW_Push` — um por botão
   - 4× `R` (10kΩ) — pull-ups
   - Ligar aos global labels `SDA`, `SCL`, `3V3`, `GND` existentes
   - Criar net labels: `BTN_A` (GPIO34), `BTN_B` (GPIO35), `BTN_L` (GPIO36), `BTN_R` (GPIO39)

2. **Ligação ao ESP32:**
   ```
   OLED:
     SDA → Net "SDA" (já ligada a GPIO21)
     SCL → Net "SCL" (já ligada a GPIO22)
     VCC → 3V3
     GND → GND

   Botões:
     BTN_A → GPIO34 (via R pull-up a 3V3)
     BTN_B → GPIO35 (via R pull-up a 3V3)
     BTN_L → GPIO36 (via R pull-up a 3V3)
     BTN_R → GPIO39 (via R pull-up a 3V3)
   ```

### 5.2 Footprints e Estratégia Bottom Side

#### Footprints a usar

| Componente | Footprint KiCad | Library | Notas |
|-----------|----------------|---------|-------|
| Header OLED 1×4 | `Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical` | Built-in | THT, furos ø1.0mm |
| Contact pad (botão) | **Custom** — criar no Footprint Editor | — | 2 pads SMD expostos por botão |
| Resistência pull-up | `Resistor_SMD:R_0603_1608Metric_Pad0.98x0.95mm_HandSolder` | Built-in | SMD no bottom layer |

> O header OLED é THT — os furos atravessam a PCB e fazem de via automática entre F.Cu e B.Cu.
> Os contact pads e resistências são SMD no B.Cu (bottom).

#### Criar o footprint custom do contact pad

Não existe um footprint standard no KiCad para rubber dome pads. É fácil criar:

**Footprint Editor → File → New Footprint → nome: `ContactPad_RubberDome_8mm`**

1. **Pad 1** (sinal — GPIO):
   - Type: SMD
   - Shape: Roundrect (cantos arredondados)
   - Size: 3mm × 4mm
   - Layer: `B.Cu`
   - Net: GPIO (34/35/36/39)
   - Solder Mask: expandir 0.1mm (expõe o cobre — **essencial**)

2. **Pad 2** (GND):
   - Type: SMD
   - Shape: Roundrect
   - Size: 3mm × 4mm
   - Layer: `B.Cu`
   - Net: GND
   - Solder Mask: expandir 0.1mm

3. **Gap entre pads:** ~1mm (centro a centro: 4mm)

4. **Contornos:**
   - `B.CrtYd`: retângulo 8mm × 6mm (courtyard)
   - `B.SilkS`: label do botão (ex: "A", "B", "L", "R")
   - `B.Fab`: contorno do rubber dome (referência, ø8mm)

```
  Footprint (visto de baixo — B.Cu):
  ┌─────────────────────────┐
  │                         │  B.CrtYd (8×6mm)
  │   ┌───────┐ ┌───────┐  │
  │   │       │ │       │  │
  │   │ pad 1 │ │ pad 2 │  │  ← Cobre exposto (sem solder mask)
  │   │ GPIO  │ │  GND  │  │    3mm × 4mm cada, gap 1mm
  │   │       │ │       │  │
  │   └───────┘ └───────┘  │
  │         "A"             │  B.SilkS
  └─────────────────────────┘
```

> **Solder mask override** é o passo crítico. Sem ele, a máscara verde cobre os pads e o rubber dome não faz contacto com o cobre. No KiCad: Pad Properties → Solder Mask Override → `0.1` (mm).

#### Colocar footprints no bottom layer no KiCad

1. **Tools → Update PCB from Schematic** para importar os novos componentes
2. Os footprints aparecem no top layer por defeito
3. Selecionar o componente no PCB editor
4. Premir **`F`** (atalho para Flip) — o componente passa para B.Cu
5. Verificar: o componente aparece **espelhado** e em **azul** (cor do B.Cu)

O que acontece ao fazer Flip:
- **Silk screen** → B.SilkS (impresso no verso)
- **Courtyard** → B.CrtYd
- **Pads SMD** → B.Cu (bottom copper)
- **Pads THT** (header OLED) → mantêm furos em ambos os layers

#### Resistências SMD no bottom

As resistências 0603 ficam no B.Cu junto aos contact pads. Ao fazer Flip:
- Pads em B.Cu, silk em B.SilkS
- Traces roteados em B.Cu ligando: 3.3V → R pull-up → pad 1 (GPIO) do contact pad
- Via para ligar ao GPIO do ESP32 no top layer

### 5.3 PCB Layout (Bottom Side)

```
┌──────────────────────────────────────────┐
│              BOTTOM SIDE                 │
│          (visto de baixo)                │
│                                          │
│  R_L ─(L)                     (R)─ R_R  │  ← Contact pads + resistências
│                                          │
│           ┌──────────────┐               │
│           │    OLED      │               │
│           │   128×64     │               │
│           │   0.96"      │               │
│           │  [GND VCC    │               │
│           │   SCL SDA]   │               │  ← Header 1×4 THT
│           └──────────────┘               │
│                                          │
│  R_B ─(B)                     (A)─ R_A  │  ← Contact pads + resistências
│                                          │
└──────────────────────────────────────────┘

Legenda:
  (X)  = Contact pad (2 pads cobre expostos, ø~8mm total)
  R_X  = Resistência 10kΩ 0603 SMD (B.Cu)
  Tudo em B.Cu — perfil ~0mm (flush com a PCB)
```

#### Routing Strategy

Todos os componentes dos botões são SMD no bottom — routing inteiro em B.Cu:

```
    F.Cu (top layer)
    ┌─────────────────────────────────┐
    │  ESP32 GPIO34 ──┐               │
    │  3.3V ──────────┼──┐            │
    └─────────────────┼──┼────────────┘
                    [via][via]          ← Vias top→bottom
    ┌─────────────────┼──┼────────────┐
    │                 ▼  ▼            │
    │           ┌─[R 10k]─┐          │
    │           │          │          │
    │           ▼          │          │   B.Cu (bottom layer)
    │     ┌──────────┐     │          │
    │     │●pad1  pad2●│────┘          │
    │     │  GPIO   GND│              │   ← Contact pads (cobre exposto)
    │     └──────────┘                │
    │                                 │
    └─────────────────────────────────┘
```

Vantagem: routing dos botões **não interfere** com os traces HUB75 do top layer.

### 5.4 Passos no KiCad (Step-by-Step)

**Passo 1 — Criar o footprint custom (uma vez):**

1. **Footprint Editor → File → New Footprint**
2. Nome: `ContactPad_RubberDome_8mm`
3. **Add Pad** (pad 1):
   - Number: `1`, Type: `SMD`, Shape: `Roundrect`
   - Size X: `3`, Size Y: `4`, Layer: `B.Cu`
   - Pad Properties → Solder Mask Expansion (override): `0.1` mm
4. **Add Pad** (pad 2):
   - Number: `2`, Type: `SMD`, Shape: `Roundrect`
   - Size X: `3`, Size Y: `4`, Layer: `B.Cu`
   - Position X: `4` (offset 4mm do pad 1 → gap de 1mm)
   - Solder Mask Expansion (override): `0.1` mm
5. Adicionar contorno em `B.CrtYd` (8mm × 6mm)
6. Adicionar label em `B.SilkS` (referência: `%R`)
7. **File → Save** na library local do projeto

**Passo 2 — Esquemático:**

1. **Place → Add Symbol** → `SW_Push` → colocar 4 instâncias (representam o contacto)
2. **Place → Add Symbol** → `R` → colocar 4 instâncias (10kΩ)
3. **Place → Add Symbol** → `Conn_01x04` → colocar 1 (header OLED)
4. **Ligar** cada botão:
   ```
   3V3 ── R(10k) ──┬── net BTN_x (→ GPIO)
                    │
                  [SW]     ← Símbolo SW_Push (representa rubber dome)
                    │
                   GND
   ```
5. **Ligar** connector OLED: pin 1 → GND, pin 2 → 3V3, pin 3 → SCL, pin 4 → SDA
6. **Anotar** (Tools → Annotate Schematic)
7. **Atribuir footprints** (Tools → Assign Footprints):
   - SW1-SW4 → `ContactPad_RubberDome_8mm` (custom)
   - R → `R_0603_1608Metric_Pad0.98x0.95mm_HandSolder`
   - J_OLED → `PinHeader_1x04_P2.54mm_Vertical`

**Passo 3 — PCB:**

8. **Tools → Update PCB from Schematic** — componentes aparecem agrupados
9. **Selecionar todos os novos componentes** (contact pads + resistências + header)
10. **Premir `F`** para fazer Flip para o bottom layer
    - Contact pads e resistências: ficam em B.Cu (azul)
    - Header OLED (THT): furos visíveis em ambos os lados, silk em B.SilkS
11. **Posicionar** conforme layout da secção 5.3:
    - Header OLED centrado
    - Contact pads L/R nas laterais, A/B em baixo
    - Resistências junto ao contact pad de cada botão
12. **Rotear traces** (tecla `X`):
    - Selecionar layer B.Cu antes de começar
    - Traçar: via do GPIO (top→bottom) → R pull-up → pad 1 do contact pad
    - Traçar: pad 2 do contact pad → GND
    - Tecla `V` durante routing para adicionar via se necessário
13. **Adicionar plano GND no bottom** (se não existir): Place → Zone → B.Cu → net GND
14. **DRC** (Inspect → Design Rules Check) antes de gerar Gerbers
15. **3D Viewer** (View → 3D Viewer) — verificar:
    - Contact pads e OLED no verso (bottom)
    - Pads de cobre visíveis (cor cobre, sem máscara verde)
    - ESP32 e HUB75 no top

---

## 6. Firmware — Exemplo de Código

### Flag de Modo de Input

```cpp
// Em board_config.h — adicionar:
// ============================================================
// INPUT MODE — Seleciona hardware de input
// ============================================================
// O GPIO 39 é partilhado entre:
//   - Click Wheel (encoder magnético AS5600 I2C + botão central) — ver product/CLICK_WHEEL.md
//   - DevKit Buttons (4 botões táteis A/B/L/R)
// O AS5600 comunica via I2C (0x36), GPIO 34/35 ficam livres no modo click wheel.
// Apenas o firmware muda.
//
//   0 = INPUT_CLICK_WHEEL  — encoder + click (brilho + modo)
//   1 = INPUT_DEVKIT_BUTTONS — 4 botões táteis (A/B/L/R)
//
#ifndef INPUT_MODE
  #define INPUT_MODE 1  // Default: DevKit buttons
#endif

#define INPUT_CLICK_WHEEL    0
#define INPUT_DEVKIT_BUTTONS 1

#if !BOARD_MATRIXPORTAL_S3
  // GPIO 36 é sempre exclusivo do DevKit (não usado pela click wheel)
  #define BTN_L_PIN   36   // Input only, pull-up externo (SVP)

  #if INPUT_MODE == INPUT_DEVKIT_BUTTONS
    #define BTN_A_PIN   34   // Partilhado com ENCODER_A
    #define BTN_B_PIN   35   // Partilhado com ENCODER_B
    #define BTN_R_PIN   39   // Partilhado com ENCODER_BTN
    #define DEVKIT_BUTTON_COUNT 4
  #elif INPUT_MODE == INPUT_CLICK_WHEEL
    #define AS5600_I2C_ADDR 0x36
    #define ENCODER_BTN_PIN 39
    #define CLICK_WHEEL_ENABLED 1
    // GPIO 34, 35, 36 ficam livres como botões auxiliares
  #endif
#endif
```

### Inicialização do OLED

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDR 0x3C

Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void setupOLED() {
  // I2C já inicializado pelo RTC (Wire.begin(21, 22))
  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    #if INPUT_MODE == INPUT_DEVKIT_BUTTONS
      oled.println("DevKit Ready");
    #else
      oled.println("Click Wheel Ready");
    #endif
    oled.display();
  }
}
```

### Setup e Loop (DevKit Buttons)

```cpp
#if INPUT_MODE == INPUT_DEVKIT_BUTTONS

void setupDevKitButtons() {
  // Todos input-only com pull-up externo
  pinMode(BTN_A_PIN, INPUT);
  pinMode(BTN_B_PIN, INPUT);
  pinMode(BTN_L_PIN, INPUT);
  pinMode(BTN_R_PIN, INPUT);
}

bool readButton(uint8_t pin) {
  return digitalRead(pin) == LOW;  // Active LOW com pull-up
}

void loopDevKitButtons() {
  static uint32_t lastDebounce = 0;
  if (millis() - lastDebounce < 50) return;

  if (readButton(BTN_A_PIN)) { lastDebounce = millis(); /* Confirmar */ }
  if (readButton(BTN_B_PIN)) { lastDebounce = millis(); /* Cancelar  */ }
  if (readButton(BTN_L_PIN)) { lastDebounce = millis(); /* Esquerda  */ }
  if (readButton(BTN_R_PIN)) { lastDebounce = millis(); /* Direita   */ }
}

#endif // INPUT_DEVKIT_BUTTONS
```

> **Click Wheel mode:** Quando `INPUT_MODE == INPUT_CLICK_WHEEL`, usa-se o código de `product/CLICK_WHEEL.md` (AS5600 via I2C polling + ISR para botão central). Os GPIOs 34, 35 e 36 ficam disponíveis como botões auxiliares.

---

## 7. Considerações de Design

### Compatibilidade I2C

| Dispositivo | Endereço | Conflito? |
|-------------|----------|-----------|
| DS3231 RTC  | 0x68     | Não       |
| SSD1306 OLED| 0x3C     | Não       |
| AS5600      | 0x36     | Não       |
| LIS3DH (S3) | 0x19     | N/A (outra placa) |

Os endereços não conflituam. O barramento I2C suporta múltiplos dispositivos sem problemas a 400kHz (Fast Mode).

### Consumo Energético Adicional

| Componente     | Consumo      |
|----------------|--------------|
| SSD1306 OLED   | ~20mA        |
| 4× Pull-ups    | ~1.3mA total (0.33mA cada quando premido) |
| **Total extra** | **~21mA**   |

Negligível comparado com o painel P10 (~2-5A).

### Mecânica e Case 3D Printed

- O OLED de 0.96" (27×27mm) cabe no verso de uma PCB para painel P10
- **Contact pads são flush com a PCB** — perfil ~0mm (só o cobre)
- **Rubber domes** adicionam ~1-2mm de altura — muito menos que tactile switches (~9.5mm)
- Altura total no verso: ~10mm (OLED com header) + ~2mm (rubber domes)
- O **case 3D printed** deve ter:
  - Cavidade para o OLED (recesso ~11mm de profundidade)
  - Botões moldados ou peças soltas que pressionam os rubber domes
  - Guias para alinhar os rubber domes com os contact pads

```
Case 3D (corte lateral):

   ┌────────────┐
   │  Botão cap │ ← Peça solta no case, guiada por paredes
   └─────┬──────┘
         │
   ┌─────▼──────┐
   │ Rubber dome│ ← Assente sobre os contact pads
   └────────────┘
   ═══●══gap══●═══ ← PCB bottom (pads cobre expostos)
   ════════════════ ← PCB top
   ┌────────────┐
   │  ESP32 /   │ ← Componentes top side
   │  HUB75     │
   └────────────┘
```

### Riscos e Mitigações

| Risco | Mitigação |
|-------|-----------|
| Pull-ups duplos (RTC + OLED módulo) | Valor efetivo ~2.35kΩ, OK para I2C 400kHz |
| GPIO 36/39 glitch (errata ESP32) | Resolvido em rev. 3 do ESP32; adicionar 100nF |
| OLED interfere com refresh do P10 | I2C opera a 400kHz, P10 usa HUB75 paralelo — sem interferência |
| Rubber dome desalinhado | Case 3D deve ter guias de posicionamento para os domes |
| Oxidação dos contact pads | Aplicar ENIG (gold) ou HASL nos pads; solder mask override garante cobre exposto |

---

## 8. Partilha de Pinos com Click Wheel

### Mapa de Partilha

| GPIO/Bus | Pull-up | Click Wheel (`INPUT_MODE=0`) | DevKit Buttons (`INPUT_MODE=1`) |
|----------|---------|------------------------------|---------------------------------|
| **I2C** (21/22) | Já existente (RTC) | AS5600 @ 0x36 (rotação) | — |
| **34** | 10kΩ externo | _(não usado)_ | BTN_A (confirmar) |
| **35** | 10kΩ externo | _(não usado)_ | BTN_B (cancelar)  |
| **36** | 10kΩ externo | _(não usado)_ — livre como BTN extra | BTN_L (esquerda)  |
| **39** | 10kΩ externo | ENCODER_BTN (click central)  | BTN_R (direita)   |

### Porquê funciona sem conflito

1. **AS5600 no barramento I2C** — O encoder magnético comunica via I2C (endereço 0x36), partilhando o barramento com RTC (0x68) e OLED (0x3C). Sem conflito de endereços.
2. **GPIO 39 partilhado** — O botão central do click wheel e o BTN_R do DevKit partilham o mesmo GPIO com circuito idêntico (pull-up 10kΩ + active LOW).
3. **GPIO 34/35 livres no modo click wheel** — Com AS5600 no I2C, os GPIO 34/35 ficam livres e podem ser usados como botões auxiliares.
4. **Seleção por firmware** — `#define INPUT_MODE 0` ou `1` em `board_config.h`. Sem alteração de hardware para o botão central.

### GPIO 34/35/36 — Livres no modo Click Wheel

O AS5600 usa I2C para rotação + GPIO 39 para click. Os GPIOs 34, 35 e 36 ficam todos disponíveis como botões auxiliares no modo click wheel (ex: toggle OLED, menu rápido, presets de brilho).

---

## 9. Resumo

Esta adição transforma a PCB existente num kit de desenvolvimento versátil:

- **Custo:** ~$1.14 extra por placa (botões) | ~$1.60 extra (click wheel AS5600 + ímã)
- **Pinos usados:** 4 GPIOs (34, 35, 36, 39) — todos input-only
- **Partilha:** GPIO 39 partilhado com click wheel (botão central). AS5600 usa I2C (0x36) — sem GPIOs dedicados
- **I2C:** OLED + AS5600 partilhados com RTC, sem conflito de endereços (0x3C / 0x36 / 0x68)
- **Funcionalidade original:** 100% preservada
- **GPIO 32:** Livre para expansão futura (I2S DIN, sensor, output)

| Modo | Componentes | Input |
|------|-------------|-------|
| `INPUT_DEVKIT_BUTTONS` | OLED (header hand-solder) + 4 rubber domes sobre contact pads (bottom) | A, B, L, R — navegação tipo gamepad |
| `INPUT_CLICK_WHEEL` | AS5600 encoder magnético (top, SMD) + ímã + OLED (bottom) | Rotação (brilho) + click (modo) + 3 BTN extra |

O ecrã OLED permite debug visual, menus de configuração, e display de informações sem necessitar de Serial Monitor. A mesma PCB serve como relógio circadiano (com click wheel) ou como dev board Arduino (com contact pads + rubber domes e ecrã), com case 3D printed.
