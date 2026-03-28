# DevKit Display + Buttons — Ecrã I2C e 4 Botões no Verso da PCB

## Conceito

Transformar a PCB do Proto Circadian Clock num **kit de desenvolvimento Arduino** adicionando no lado oposto (bottom side):

- **1x Ecrã OLED monocromático** (I2C, partilha SDA/SCL com o RTC DS3231)
- **4x Botões táteis** (A, B, L, R) para navegação/menus

Tudo hand-soldered, sem alterar o circuito existente do lado top.

> **Compatibilidade com Click Wheel:** Os botões A, B e R partilham os mesmos GPIOs (34, 35, 39) que o encoder óptico documentado em `product/CLICK_WHEEL.md`. Isto é **intencional** — o circuito elétrico é idêntico (input-only + pull-up externo 10kΩ a 3.3V + active LOW). Na PCB, os mesmos pads servem para ambas as configurações. Uma flag de compilação (`INPUT_MODE`) seleciona o firmware adequado. Ver [secção 8](#8-partilha-de-pinos-com-click-wheel) para detalhes.

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

## 3. Botões — 4x Táteis (A, B, L, R)

### Atribuição de Pinos

| Botão | GPIO | Tipo       | Partilhado com Click Wheel? | Função Sugerida    |
|-------|------|------------|-----------------------------|--------------------|
| **A** | 34   | Input only | Sim — ENCODER_A             | Confirmar / Select |
| **B** | 35   | Input only | Sim — ENCODER_B             | Cancelar / Back    |
| **L** | 36   | Input only | Não — exclusivo DevKit      | Esquerda / Menos   |
| **R** | 39   | Input only | Sim — ENCODER_BTN           | Direita / Mais     |

> GPIO 32 fica de reserva para expansão futura (ex: I2S DIN para MAX98357A, sensor adicional).
>
> **Nota:** Os GPIOs 34, 35 e 39 são partilhados com a click wheel (`product/CLICK_WHEEL.md`). O circuito (pull-up 10kΩ + active LOW) é idêntico — os mesmos pads e resistências servem ambas as configurações. A seleção é feita por `#define INPUT_MODE` no firmware.

### Circuito por Botão

```
3.3V ─── R (10kΩ) ──┬── GPIO 34/35/36/39
                     │
                   [BTN]
                     │
                    GND
```

**Importante:** GPIO 34-39 **não têm pull-up interno**, portanto as resistências de 10kΩ são obrigatórias. Sem elas, o pino flutua quando o botão não está premido.

### Debounce

Recomendação: debounce por software (20-50ms) no firmware. Opcional: condensador cerâmico de 100nF em paralelo com cada botão para debounce por hardware.

---

## 4. BOM (Bill of Materials)

### Componentes Obrigatórios

| # | Componente | Quantidade | JLCPCB Part | Tipo | Preço Est. |
|---|-----------|------------|-------------|------|------------|
| 1 | OLED 0.96" 128×64 SSD1306 I2C | 1 | Módulo — hand solder | N/A | ~$1.00 |
| 2 | Botão tátil 6×6mm THT | 4 | C136662 (TS-1187A) | Basic | ~$0.02/un |
| 3 | Resistência 10kΩ 0402/0603 | 4 | C25744 (0402) ou C25804 (0603) | Basic | ~$0.002/un |
| 4 | Header fêmea 1×4 2.54mm | 1 | C124413 | Basic | ~$0.05 |

### Componentes Opcionais

| # | Componente | Quantidade | JLCPCB Part | Notas |
|---|-----------|------------|-------------|-------|
| 5 | Condensador 100nF 0402 | 4 | C1525 | Debounce HW (opcional) |
| 6 | Header macho 1×4 2.54mm | 1 | C124378 | Alternativa a soldar direto |

### Custo Estimado por Placa

| Item           | Custo   |
|----------------|---------|
| OLED SSD1306   | ~$1.00  |
| 4× Botões      | ~$0.08  |
| 4× Resistências| ~$0.01  |
| Header         | ~$0.05  |
| **Total**      | **~$1.14** |

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

### 5.2 Footprints

**Para o OLED (módulo com header):**
- Usar footprint `PinHeader_1x04_P2.54mm_Vertical` (GND, VCC, SCL, SDA)
- Colocar no bottom layer (B.Cu, B.Fab, B.Silk)

**Para os botões:**
- Footprint: `SW_Push_1P1T_NO_6x6mm_H9.5mm` (THT, 4 pads)
- Colocar no bottom layer

**Para as resistências:**
- Footprint: `R_0603_1608Metric` (hand solder friendly)
- Colocar no bottom layer junto aos botões

**Importar footprints em falta:**
```bash
# Botão tátil TS-1187A (se não existir no KiCad)
python3 -m easyeda2kicad --symbol --footprint --3d --lcsc_id=C136662

# Resistência 0603 (já incluída no KiCad por defeito)
# Header 1x4 (já incluído no KiCad por defeito)
```

### 5.3 PCB Layout (Bottom Side)

```
┌──────────────────────────────────┐
│          BOTTOM SIDE             │
│                                  │
│   [L]                    [R]     │  ← Botões esquerda/direita
│                                  │
│         ┌──────────┐             │
│         │  OLED    │             │
│         │ 128×64   │             │
│         │ 0.96"    │             │
│         └──────────┘             │
│                                  │
│   [B]                    [A]     │  ← Botões B (cancel) / A (confirm)
│                                  │
└──────────────────────────────────┘
```

**Dicas de layout:**
- Manter OLED centrado no bottom
- Botões L/R nas extremidades horizontais (layout tipo gamepad)
- Botões A/B abaixo do ecrã
- Resistências pull-up junto aos pads dos botões (minimizar trace length)
- Traces: 0.25mm para sinais, 0.5mm para alimentação
- Vias para ligar ao barramento I2C e GPIOs do top layer

### 5.4 Passos no KiCad (Step-by-Step)

1. **Abrir esquemático** → Add Symbol → Colocar `SW_Push` ×4 e `R` ×4
2. **Ligar** cada botão: 3V3 → R(10k) → junction → GPIO + BTN → GND
3. **Colocar** símbolo OLED (ou connector 1×4) → ligar a SDA, SCL, 3V3, GND
4. **Anotar** esquemático (Tools → Annotate Schematic)
5. **Gerar netlist** e abrir PCB editor
6. **Update PCB** from schematic (Tools → Update PCB)
7. **Mover footprints** para o bottom layer:
   - Selecionar componente → Propriedades → Layer: `B.Cu`
   - Ou: Right-click → Flip (atalho: `F`)
8. **Posicionar** conforme layout acima
9. **Rotear traces** no bottom layer (B.Cu)
10. **Adicionar vias** onde necessário para ligar a nets do top layer
11. **DRC** (Design Rules Check) antes de gerar Gerbers

---

## 6. Firmware — Exemplo de Código

### Flag de Modo de Input

```cpp
// Em board_config.h — adicionar:
// ============================================================
// INPUT MODE — Seleciona hardware de input nos GPIOs partilhados
// ============================================================
// Os GPIOs 34, 35, 39 são partilhados entre:
//   - Click Wheel (encoder óptico/mecânico) — ver product/CLICK_WHEEL.md
//   - DevKit Buttons (4 botões táteis A/B/L/R)
// O circuito elétrico é idêntico (pull-up 10kΩ + active LOW).
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
    #define ENCODER_A_PIN   34
    #define ENCODER_B_PIN   35
    #define ENCODER_BTN_PIN 39
    #define CLICK_WHEEL_ENABLED 1
    // BTN_L (GPIO36) continua disponível como botão extra
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

> **Click Wheel mode:** Quando `INPUT_MODE == INPUT_CLICK_WHEEL`, usa-se o código de `product/CLICK_WHEEL.md` (ISRs, setupClickWheel, handleClickWheel). O GPIO 36 (BTN_L) continua disponível como botão auxiliar.

---

## 7. Considerações de Design

### Compatibilidade I2C

| Dispositivo | Endereço | Conflito? |
|-------------|----------|-----------|
| DS3231 RTC  | 0x68     | Não       |
| SSD1306 OLED| 0x3C     | Não       |
| LIS3DH (S3) | 0x19     | N/A (outra placa) |

Os endereços não conflituam. O barramento I2C suporta múltiplos dispositivos sem problemas a 400kHz (Fast Mode).

### Consumo Energético Adicional

| Componente     | Consumo      |
|----------------|--------------|
| SSD1306 OLED   | ~20mA        |
| 4× Pull-ups    | ~1.3mA total (0.33mA cada quando premido) |
| **Total extra** | **~21mA**   |

Negligível comparado com o painel P10 (~2-5A).

### Mecânica

- O OLED de 0.96" (27×27mm) cabe confortavelmente no verso de uma PCB para painel P10
- Os botões 6×6mm THT são robustos para hand soldering
- Altura total adicional no verso: ~10mm (OLED) + ~9.5mm (botões) — verificar clearance com o painel

### Riscos e Mitigações

| Risco | Mitigação |
|-------|-----------|
| Pull-ups duplos (RTC + OLED módulo) | Valor efetivo ~2.35kΩ, OK para I2C 400kHz |
| GPIO 36/39 glitch (errata ESP32) | Resolvido em rev. 3 do ESP32; adicionar 100nF |
| OLED interfere com refresh do P10 | I2C opera a 400kHz, P10 usa HUB75 paralelo — sem interferência |
| Altura dos botões no verso | Usar botões de 4.5mm de altura se necessário |

---

## 8. Partilha de Pinos com Click Wheel

### Mapa de Partilha

| GPIO | Pull-up | Click Wheel (`INPUT_MODE=0`) | DevKit Buttons (`INPUT_MODE=1`) |
|------|---------|------------------------------|---------------------------------|
| **34** | 10kΩ externo | ENCODER_A (canal A quadratura) | BTN_A (confirmar) |
| **35** | 10kΩ externo | ENCODER_B (canal B quadratura) | BTN_B (cancelar)  |
| **36** | 10kΩ externo | _(não usado)_ — livre como BTN extra | BTN_L (esquerda)  |
| **39** | 10kΩ externo | ENCODER_BTN (click central)  | BTN_R (direita)   |

### Porquê funciona sem conflito

1. **Circuito idêntico** — Tanto os botões como os sensores ópticos (ITR8307) usam o mesmo esquema: pull-up 10kΩ a 3.3V, sinal active LOW. O encoder mecânico (EC11) também.
2. **Mesmo footprint de resistência** — As 3 resistências pull-up (R nos GPIO 34, 35, 39) servem ambos os modos. Só a R do GPIO 36 é exclusiva do DevKit.
3. **Seleção por firmware** — `#define INPUT_MODE 0` ou `1` em `board_config.h`. Sem alteração de hardware.
4. **Sem curto-circuito possível** — Todos são input-only. Mesmo que ambos os componentes estivessem soldados (botão + sensor óptico no mesmo GPIO), o pior caso é leitura ambígua — não há dano.

### PCB: Dois footprints, mesma net

No KiCad, o mesmo net label (ex: `ENC_A` / `BTN_A` no GPIO 34) liga a:
- Footprint do botão tátil (bottom layer)
- Pads para o sensor ITR8307 (top layer, junto ao disco)

O utilizador solda **um ou outro**. A resistência pull-up é partilhada.

```
              3.3V
               │
            R 10kΩ         ← Partilhada (1 resistência por GPIO)
               │
         ┌─────┼─────┐
         │     │     │
      [BTN]   [ITR]  ├── GPIO 34/35/39
      (bottom) (top)  │
         │     │     │
        GND   GND    │
                     ESP32
```

### GPIO 36 — Exclusivo DevKit

O GPIO 36 não é usado pela click wheel (que só precisa de 3 pinos: A, B, BTN). No modo click wheel, o GPIO 36 fica disponível como botão auxiliar (ex: toggle OLED on/off, menu rápido).

---

## 9. Resumo

Esta adição transforma a PCB existente num kit de desenvolvimento versátil:

- **Custo:** ~$1.14 extra por placa (botões) | ~$0.20 extra (click wheel, sensores hand-solder)
- **Pinos usados:** 4 GPIOs (34, 35, 36, 39) — todos input-only
- **Partilha:** 3 GPIOs (34, 35, 39) partilhados com click wheel — circuito idêntico, seleção por firmware
- **I2C:** OLED partilhado com RTC, sem conflito de endereços (0x3C vs 0x68)
- **Funcionalidade original:** 100% preservada
- **GPIO 32:** Livre para expansão futura (I2S DIN, sensor, output)

| Modo | Componentes soldados | Input |
|------|---------------------|-------|
| `INPUT_DEVKIT_BUTTONS` | 4 botões + OLED (bottom) | A, B, L, R — navegação tipo gamepad |
| `INPUT_CLICK_WHEEL` | Encoder + OLED (top/bottom) | Rotação (brilho) + click (modo) + BTN_L extra |

O ecrã OLED permite debug visual, menus de configuração, e display de informações sem necessitar de Serial Monitor. A mesma PCB serve como relógio circadiano (com click wheel) ou como dev board Arduino (com botões e ecrã).
