# PCB Design Guide - Circadian Clock v2.0

## Sumário

Este documento descreve o design de uma PCB integrada profissional para o Circadian Clock, incluindo proteções robustas contra inversão de polaridade, curto-circuito, ESD e sobretensão.

---

## 1. Bill of Materials (BOM) Completo

### 1.1 Microcontrolador e Módulos

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Fornecedor |
|-----|------------|---------------|---------|-----|-------------|------------|
| U1 | ESP32-WROOM-32E | 4MB Flash, WiFi+BLE | Module | 1 | €2.50 | LCSC, Mouser |
| U2 | DS3231SN | RTC I2C, ±2ppm | SOIC-16 | 1 | €1.20 | LCSC |
| Y1 | Crystal 32.768kHz | Para DS3231 (integrado) | - | 0 | - | - |
| BT1 | CR2032 Holder | SMD battery holder | SMD | 1 | €0.30 | LCSC |

### 1.2 Alimentação e Regulação

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Notas |
|-----|------------|---------------|---------|-----|-------------|-------|
| U3 | AMS1117-3.3 | LDO 3.3V 1A | SOT-223 | 1 | €0.10 | Para ESP32 |
| U4 | MP1584EN | Buck 3A adjustable | Module/QFN | 1 | €0.80 | 5V para LEDs (opcional) |
| J1 | DC Barrel Jack | 5.5x2.1mm | Through-hole | 1 | €0.20 | Entrada principal |
| J2 | USB-C Connector | USB 2.0 power+data | SMD | 1 | €0.40 | Alternativa/programação |

### 1.3 Proteções (CRÍTICO)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **D1** | **SS54 / SS56** | Schottky 5A 40V | SMC | 1 | €0.15 | **Proteção inversão polaridade** |
| **F1** | **MF-MSMF250** | PTC Fuse 2.5A resettable | 1206 | 1 | €0.20 | **Proteção curto-circuito** |
| **U5** | **SRV05-4** | TVS Diode Array | SOT-23-6 | 2 | €0.25 | **Proteção ESD (USB, I2C)** |
| **D2** | **SMBJ5.0A** | TVS 5V unidirectional | SMB | 1 | €0.30 | **Proteção sobretensão** |
| **D3** | **1N5819** | Schottky flyback | SOD-123 | 2 | €0.05 | **Proteção indutiva** |
| **Z1** | **PESD5V0S1BA** | ESD protection | SOD-323 | 4 | €0.08 | **ESD nos GPIOs críticos** |

### 1.4 Condensadores

| Ref | Valor | Especificação | Package | Qty | Notas |
|-----|-------|---------------|---------|-----|-------|
| C1, C2 | 100µF | Electrolítico 16V | 8x10mm | 2 | Entrada/saída power |
| C3, C4 | 22µF | Cerâmico X5R 10V | 0805 | 2 | Bypass LDO |
| C5-C10 | 100nF | Cerâmico X7R 16V | 0402 | 6 | Decoupling |
| C11, C12 | 10µF | Cerâmico X5R 10V | 0603 | 2 | ESP32 bypass |
| C13 | 1µF | Cerâmico X7R 16V | 0402 | 1 | RTC bypass |

### 1.5 Resistências

| Ref | Valor | Especificação | Package | Qty | Notas |
|-----|-------|---------------|---------|-----|-------|
| R1, R2 | 4.7kΩ | 1% 0.1W | 0402 | 2 | I2C pull-up |
| R3 | 10kΩ | 1% 0.1W | 0402 | 1 | ESP32 EN pull-up |
| R4 | 10kΩ | 1% 0.1W | 0402 | 1 | Button pull-up (backup) |
| R5, R6 | 5.1kΩ | 1% 0.1W | 0402 | 2 | USB-C CC1/CC2 |
| R7 | 0Ω | Jumper | 0402 | 1 | Opcional bypass |
| R8-R10 | 330Ω | 5% 0.1W | 0402 | 3 | LED indicators |

### 1.6 LEDs Indicadores

| Ref | Cor | Especificação | Package | Qty | Função |
|-----|-----|---------------|---------|-----|--------|
| LED1 | Verde | 3.0V 20mA | 0603 | 1 | Power ON |
| LED2 | Azul | 3.2V 20mA | 0603 | 1 | WiFi status |
| LED3 | Vermelho | 2.0V 20mA | 0603 | 1 | Error/mode |

### 1.7 Botões e Interface

| Ref | Componente | Especificação | Package | Qty | Notas |
|-----|------------|---------------|---------|-----|-------|
| SW1 | Tactile Switch | 6x6mm, 160gf | Through-hole | 1 | Mode button (GPIO0) |
| SW2 | Tactile Switch | 6x6mm, 160gf | Through-hole | 1 | Reset button (EN) |
| SW3 | Tactile Switch | 3.5x6mm SMD | SMD | 1 | Boot button (GPIO0) - programação |

### 1.8 Conectores

| Ref | Componente | Especificação | Package | Qty | Função |
|-----|------------|---------------|---------|-----|--------|
| J3 | HUB75 Header | 2x8 pins 2.54mm | Through-hole | 1 | Painel LED P10 |
| J4 | I2C Header | 1x4 pins 2.54mm | Through-hole | 1 | Expansão sensores |
| J5 | UART Header | 1x4 pins 2.54mm | Through-hole | 1 | Debug/programação |
| J6 | GPIO Header | 1x6 pins 2.54mm | Through-hole | 1 | Expansão futura |

### 1.9 Conectores HUB75 (Painel LED)

| Pin HUB75 | Sinal | GPIO ESP32 |
|-----------|-------|------------|
| 1 | R1 | GPIO 25 |
| 2 | G1 | GPIO 26 |
| 3 | B1 | GPIO 27 |
| 4 | GND | GND |
| 5 | R2 | GPIO 14 |
| 6 | G2 | GPIO 12 |
| 7 | B2 | GPIO 13 |
| 8 | GND | GND |
| 9 | A | GPIO 23 |
| 10 | B | GPIO 19 |
| 11 | C | GPIO 5 |
| 12 | D | GPIO 17 |
| 13 | CLK | GPIO 16 |
| 14 | LAT | GPIO 4 |
| 15 | OE | GPIO 15 |
| 16 | GND | GND |

### 1.10 Level Shifters (NOVO - Baseado em Adafruit MatrixPortal S3)

> **Referência:** O design do [Adafruit MatrixPortal S3](https://github.com/adafruit/Adafruit-MatrixPortal-S3-PCB)
> utiliza level shifting integrado para converter sinais 3.3V do ESP32 para 5V do painel HUB75.

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **U6** | **74AHCT245PW** | Octal bus transceiver | TSSOP-20 | 1 | €0.30 | Level shift sinais RGB + Row |
| **U7** | **74AHCT245PW** | Octal bus transceiver | TSSOP-20 | 1 | €0.30 | Level shift CLK, LAT, OE, C, D |
| U8 | CH340C | USB-UART bridge | SOP-16 | 1 | €0.40 | Programação via USB |
| C14, C15 | 100nF | Cerâmico X7R 16V | 0402 | 2 | €0.02 | Bypass level shifters |

**Porquê 74AHCT245?**
- Aceita entrada 3.3V (Vih = 2.0V) ✓
- Saída 5V quando alimentado a 5V ✓
- 8 canais bidirecionais (usamos unidirecional) ✓
- Propagation delay: ~7ns (rápido para LEDs) ✓
- Disponível e barato na LCSC ✓

**Alternativas:**
| Componente | Canais | Vantagem | Desvantagem |
|------------|--------|----------|-------------|
| 74AHCT245 | 8 | Standard, barato | Precisa 2 ICs |
| 74HCT245 | 8 | Ainda mais barato | Mais lento |
| TXB0108 | 8 | Bidirecional auto | Mais caro, sensível |
| SN74LV1T34 | 1 | Pequeno | Precisa 13 ICs! |

---

## 2. Circuitos de Proteção Detalhados

### 2.1 Proteção Contra Inversão de Polaridade

```
Método 1: Díodo Schottky Série (Recomendado para simplicidade)
═══════════════════════════════════════════════════════════════

    VIN+ ──────┬──────[D1 SS54]──────┬────── +5V_PROTECTED
               │                     │
              ┌┴┐                   ═╧═
         C1   │ │ 100µF             GND
              └┬┘
               │
    VIN- ──────┴─────────────────────────── GND

Características:
- Vf = 0.3-0.5V @ 2A (baixa queda)
- Se polaridade invertida: díodo bloqueia
- Perda: ~0.5W @ 1A

═══════════════════════════════════════════════════════════════

Método 2: P-MOSFET (Recomendado para eficiência)
═══════════════════════════════════════════════════════════════

    VIN+ ──────┬──────────┬──[Q1 Si2301]S────── +5V_PROTECTED
               │          │         │
               │         ┌┴┐        │G
              ┌┴┐    R1  │ │10k     │
         C1   │ │100µF   └┬┘       ─┴─ D
              └┬┘         │        Body diode
               │          │
    VIN- ──────┴──────────┴───────────────── GND

Componentes:
- Q1: Si2301CDS (P-ch MOSFET, -20V, -2.8A, Rds=80mΩ)
- R1: 10kΩ (gate pull-down)

Funcionamento:
- Polaridade correta: Vgs negativo → MOSFET conduz → Rds×I² perda mínima
- Polaridade invertida: Vgs positivo → MOSFET bloqueia
- Perda: ~0.08W @ 1A (vs 0.5W do díodo)
```

### 2.2 Proteção Contra Curto-Circuito

```
Fusível PTC Resettable
═══════════════════════════════════════════════════════════════

    +5V_IN ────[F1 MF-MSMF250]──────┬────── +5V_FUSED
                                    │
                                   ═╧═
                                   GND

Especificações F1 (MF-MSMF250):
- Corrente de hold: 2.5A (funciona normal)
- Corrente de trip: 5.0A (dispara)
- Tempo de trip: <1s @ 10A
- Auto-reset: Sim (arrefece e recupera)
- Resistência: 0.03Ω (perda mínima)

Alternativa - Fusível + Circuito de Crowbar:
═══════════════════════════════════════════════════════════════

                    ┌─────────────────┐
    +5V ──[F1]──┬───┤ TL431 (2.5V)    ├───┐
                │   └────────┬────────┘   │
               ┌┴┐           │           ─┴─
          R1   │ │22k       ─┴─         SCR (crowbar)
               └┬┘          GND          │
                │                        │
    GND ────────┴────────────────────────┘

Se tensão > limite: TL431 dispara SCR → curto-circuito → fusível abre
```

### 2.3 Proteção ESD (Electrostatic Discharge)

```
Proteção USB-C e I2C
═══════════════════════════════════════════════════════════════

              ┌─────────────────────────┐
    USB_D+ ───┤1    SRV05-4 (U5)      4├─── VCC (5V)
    USB_D- ───┤2                      5├─── I2C_SDA
    GND ──────┤3                      6├─── I2C_SCL
              └─────────────────────────┘

Especificações SRV05-4:
- Tensão de clamping: 8.5V @ 1A
- Capacitância: 0.5pF (baixa, boa para dados)
- ESD rating: ±15kV (ar), ±8kV (contacto)

Proteção GPIO Críticos (Botão, etc.)
═══════════════════════════════════════════════════════════════

    GPIO0 (Button) ───┬───[PESD5V0S1BA]───┬─── GND
                      │                    │
                     ┌┴┐                  ═╧═
                 R4  │ │10k               GND
                     └┬┘
                      │
                     SW1 ─── GND
```

### 2.4 Proteção Contra Sobretensão (Overvoltage)

```
TVS Diode na Entrada
═══════════════════════════════════════════════════════════════

    +VIN ──[F1]──[D1]──┬────────────────┬────── +5V_PROTECTED
                       │                │
                      ─┴─              ┌┴┐
                   D2 SMBJ5.0A    C1   │ │100µF
                      ─┬─              └┬┘
                       │                │
    GND ───────────────┴────────────────┴────── GND

Especificações SMBJ5.0A:
- Standoff voltage: 5.0V (não conduz)
- Breakdown voltage: 6.4V (começa a conduzir)
- Clamping voltage: 9.2V @ 10A
- Potência: 600W (10/1000µs)

Sequência de proteção:
1. Tensão normal (5V): TVS não conduz
2. Spike até 6V: TVS começa a conduzir
3. Spike > 6V: TVS clampa, F1 limita corrente
4. Spike muito alto: F1 abre (protege tudo)
```

### 2.5 Proteção Indutiva (Flyback)

```
Para cargas indutivas (relés, motores futuros)
═══════════════════════════════════════════════════════════════

    GPIO ──────┬──────────────────┐
               │                  │
              ┌┴┐                ─┴─
          R   │ │1k           D3 1N5819 (flyback)
              └┬┘                ─┬─
               │                  │
               └────[Q NPN]───────┴───── LOAD+
                      │
                      └───────────────── LOAD- (VCC)

Quando GPIO = LOW:
- Corrente indutiva tenta continuar
- D3 conduz e dissipa energia
- Protege transistor e GPIO
```

### 2.6 Level Shifter 3.3V → 5V para HUB75 (NOVO)

> **Baseado no design Adafruit MatrixPortal S3**
> Fonte: https://github.com/adafruit/Adafruit-MatrixPortal-S3-PCB

```
ANTES (Design Original - Ligação Direta):
═══════════════════════════════════════════════════════════════

    ESP32 (3.3V logic)              P10 Panel (5V logic)
    ─────────────────               ─────────────────────
    GPIO 25 ─────────────────────── R1   ⚠️ PROBLEMA!
    GPIO 26 ─────────────────────── G1
    GPIO 27 ─────────────────────── B1   Os painéis HUB75 esperam
    GPIO 14 ─────────────────────── R2   sinais 5V (Vih ≈ 3.5V)
    GPIO 12 ─────────────────────── G2
    GPIO 13 ─────────────────────── B2   ESP32 output = 3.3V
    GPIO 23 ─────────────────────── A    Pode funcionar, mas é
    GPIO 19 ─────────────────────── B    marginal e instável!
    GPIO 5  ─────────────────────── C
    GPIO 17 ─────────────────────── D
    GPIO 16 ─────────────────────── CLK
    GPIO 4  ─────────────────────── LAT
    GPIO 15 ─────────────────────── OE

Problemas:
- Sinais 3.3V são marginais para lógica 5V
- Painéis diferentes têm thresholds diferentes
- Pode causar flickering ou cores erradas
- Falhas intermitentes difíceis de diagnosticar

═══════════════════════════════════════════════════════════════

DEPOIS (Design Melhorado - Com Level Shifter):
═══════════════════════════════════════════════════════════════

                              VCC = 5V
                                 │
                  ┌──────────────┴──────────────┐
                  │         74AHCT245 (U6)      │
                  │           VCC=5V            │
                  │                             │
    ESP32         │                             │         HUB75
    ─────         │  DIR  OE                    │         ─────
    GPIO25 ───────┤A1  │   │                 B1├───────── R1
    GPIO26 ───────┤A2  │   │                 B2├───────── G1
    GPIO27 ───────┤A3  │   │                 B3├───────── B1
    GPIO14 ───────┤A4  │   │                 B4├───────── R2
    GPIO12 ───────┤A5  │   │                 B5├───────── G2
    GPIO13 ───────┤A6  │   │                 B6├───────── B2
    GPIO23 ───────┤A7  │   │                 B7├───────── A
    GPIO19 ───────┤A8  │   │                 B8├───────── B
                  │    │   │                    │
                  │   GND GND                   │
                  │    │   │                    │
                  └────┴───┴────────────────────┘
                       │
                      GND
                       │
                  ┌────┴───┬────────────────────┐
                  │    │   │                    │
                  │   GND GND                   │
                  │    │   │                    │
                  │  DIR  OE                    │
    GPIO5  ───────┤A1  │   │                 B1├───────── C
    GPIO17 ───────┤A2  │   │                 B2├───────── D
    GPIO4  ───────┤A3  │   │                 B3├───────── LAT
    GPIO15 ───────┤A4  │   │                 B4├───────── OE
    GPIO16 ───────┤A5  │   │                 B5├───────── CLK
    NC ────────────┤A6                       B6├───────── NC
    NC ────────────┤A7                       B7├───────── NC
    NC ────────────┤A8                       B8├───────── NC
                  │                             │
                  │         74AHCT245 (U7)      │
                  │           VCC=5V            │
                  └──────────────┬──────────────┘
                                 │
                                GND

Ligações 74AHCT245:
- Pin 1 (OE active low): GND (sempre ativo)
- Pin 19 (DIR): GND (A→B, unidirecional)
- Pin 10 (GND): GND
- Pin 20 (VCC): +5V

Condensadores bypass:
- C14: 100nF entre VCC e GND do U6 (o mais perto possível)
- C15: 100nF entre VCC e GND do U7 (o mais perto possível)

═══════════════════════════════════════════════════════════════

Vantagens do Design Melhorado:
✓ Sinais robustos 5V para o painel
✓ Compatível com qualquer painel HUB75
✓ Sem flickering ou cores erradas
✓ Protege GPIOs do ESP32 de backfeed
✓ Custo adicional: apenas €0.60

═══════════════════════════════════════════════════════════════
```

**Pinout 74AHCT245 (TSSOP-20):**

```
        ┌────────────────────┐
   OE ──┤1                 20├── VCC (5V)
   A1 ──┤2                 19├── DIR
   A2 ──┤3                 18├── B1
   A3 ──┤4                 17├── B2
   A4 ──┤5                 16├── B3
   A5 ──┤6                 15├── B4
   A6 ──┤7                 14├── B5
   A7 ──┤8                 13├── B6
   A8 ──┤9                 12├── B7
  GND ──┤10                11├── B8
        └────────────────────┘

OE = Output Enable (active LOW) → GND
DIR = Direction (LOW = A→B) → GND
```

**Layout Tips (Adafruit Style):**

```
┌─────────────────────────────────────────────────────────────┐
│                     PCB LAYOUT TIPS                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. POSICIONAMENTO                                          │
│     ┌─────────┐                                             │
│     │  ESP32  │──── curto ────┌─────────┐                   │
│     └─────────┘      <2cm     │74AHCT245│─── curto ──[HUB75]│
│                               └─────────┘     <2cm          │
│                                                             │
│  2. BYPASS CAPS - O mais perto possível do IC               │
│                                                             │
│     ┌─────────────────────────┐                             │
│     │      74AHCT245          │                             │
│     │  VCC ────┬──── GND      │                             │
│     │         ═╪═             │                             │
│     │       [100nF]           │   ← Mesmo lado do IC        │
│     │          │              │                             │
│     └──────────┼──────────────┘                             │
│               GND                                           │
│                                                             │
│  3. GROUND PLANE - Retorno de corrente direto               │
│                                                             │
│     Top Layer: Sinais                                       │
│     Bottom Layer: GND plane (contínuo sob os ICs)           │
│                                                             │
│  4. TRACE WIDTH                                             │
│     - Sinais: 0.2mm (8mil) mínimo                          │
│     - VCC/GND: 0.4mm (16mil) ou mais                       │
│     - HUB75 CLK: manter curto, evitar vias                 │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Esquema de Blocos

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CIRCADIAN CLOCK PCB v2.0                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────┐     ┌──────────────────────────────────────────────────┐ │
│  │   POWER IN   │     │              PROTEÇÕES                           │ │
│  │              │     │  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐             │ │
│  │  DC 5V 3A    ├────►│  │ D1  │─►│ F1  │─►│ D2  │─►│ TVS │──►+5V_SAFE  │ │
│  │  ou USB-C    │     │  │Schot│  │ PTC │  │ TVS │  │ ESD │             │ │
│  │              │     │  └─────┘  └─────┘  └─────┘  └─────┘             │ │
│  └──────────────┘     └──────────────────────────────────────────────────┘ │
│                                        │                                    │
│                                        ▼                                    │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                         POWER REGULATION                              │ │
│  │   +5V_SAFE ───► [AMS1117-3.3] ───► +3.3V (ESP32, RTC)                │ │
│  │   +5V_SAFE ───► Direct ───► +5V (LED Panel via HUB75)                │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                        │                                    │
│           ┌────────────────────────────┼────────────────────────────┐      │
│           ▼                            ▼                            ▼      │
│  ┌─────────────────┐     ┌─────────────────────────┐    ┌───────────────┐ │
│  │    DS3231 RTC   │     │     ESP32-WROOM-32E     │    │  74AHCT245    │ │
│  │                 │◄───►│                         │───►│  Level Shift  │ │
│  │  I2C (SDA/SCL)  │     │  GPIO 25-27: R1,G1,B1   │    │  (U6 + U7)    │ │
│  │  3.3V + CR2032  │     │  GPIO 12-14: R2,G2,B2   │    │  3.3V → 5V    │ │
│  └─────────────────┘     │  GPIO 4,5,15-17,19,23   │    └───────┬───────┘ │
│                          │                         │            │         │
│                          │                         │            ▼         │
│                          │                         │    ┌───────────────┐ │
│                          │                         │    │  P10 LED      │ │
│                          │                         │    │  PANEL        │ │
│                          │                         │    │  (HUB75)      │ │
│                          │                         │    │  32x16 RGB    │ │
│                          │                         │    └───────────────┘ │
│                          │  GPIO 0: Mode Button    │                       │
│  ┌─────────────────┐     │  GPIO 21,22: I2C        │    ┌───────────────┐ │
│  │   BUTTONS       │     │  EN: Reset Button       │    │  STATUS LEDs  │ │
│  │  SW1: Mode      │────►│                         │───►│  PWR/WiFi/ERR │ │
│  │  SW2: Reset     │     │  USB-C: UART + Power    │    └───────────────┘ │
│  │  SW3: Boot      │     └─────────────────────────┘                       │
│  └─────────────────┘                                                        │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                         EXPANSION HEADERS                             │ │
│  │   J4: I2C (3.3V, GND, SDA, SCL) - Sensores futuros                   │ │
│  │   J5: UART (3.3V, GND, TX, RX) - Debug                               │ │
│  │   J6: GPIO (3.3V, GND, GPIO32, GPIO33, GPIO34, GPIO35) - Expansão    │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Software para Design de PCB

### 4.1 Opções Recomendadas

| Software | Custo | Nível | Prós | Contras | Recomendação |
|----------|-------|-------|------|---------|--------------|
| **KiCad 8** | Grátis | Intermédio | Open-source, completo, JLCPCB integração | Curva aprendizagem | ⭐⭐⭐⭐⭐ **RECOMENDADO** |
| **EasyEDA** | Grátis | Iniciante | Browser-based, integração JLCPCB/LCSC | Limitado offline | ⭐⭐⭐⭐ Bom para começar |
| **Altium Designer** | €3000+/ano | Profissional | Industry standard, poderoso | Caro, complexo | ⭐⭐⭐ Overkill |
| **Eagle** | €500/ano | Intermédio | Estabelecido, boas libs | Autodesk lock-in | ⭐⭐⭐ |
| **Fusion 360 Electronics** | Grátis* | Iniciante | Integrado com CAD mecânico | Limitações grátis | ⭐⭐⭐ |

### 4.2 Workflow Recomendado: KiCad 8

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  Schematic  │───►│   Symbols   │───►│  PCB Layout │───►│   Gerbers   │
│   Editor    │    │   + Libs    │    │   Editor    │    │   Export    │
│  (Eeschema) │    │             │    │  (Pcbnew)   │    │             │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
                                                                │
                                                                ▼
                                                         ┌─────────────┐
                                                         │  JLCPCB     │
                                                         │  Order      │
                                                         └─────────────┘
```

**Instalação KiCad 8:**
```bash
# Ubuntu/Debian
sudo add-apt-repository ppa:kicad/kicad-8.0-releases
sudo apt update
sudo apt install kicad

# Windows/Mac
# Download: https://www.kicad.org/download/

# Bibliotecas adicionais (ESP32, etc.)
git clone https://github.com/espressif/kicad-libraries.git
```

### 4.3 Bibliotecas Necessárias

| Biblioteca | Componentes | URL |
|------------|-------------|-----|
| Espressif KiCad | ESP32-WROOM-32E | github.com/espressif/kicad-libraries |
| SnapEDA | DS3231, USB-C, etc. | snapeda.com |
| Ultra Librarian | Componentes LCSC | ultralibrarian.com |
| KiCad Official | Básicos | Incluído no KiCad |

### 4.4 Plugins Úteis para KiCad

| Plugin | Função |
|--------|--------|
| **JLCPCB Tools** | Exportar BOM e CPL para JLCPCB |
| **Interactive HTML BOM** | Gerar BOM visual interativo |
| **KiCost** | Calcular custos de componentes |
| **Teardrops** | Melhorar fiabilidade das vias |
| **RF Tools** | Para antena WiFi do ESP32 |

---

## 5. Regras de Design PCB

### 5.1 Especificações para JLCPCB

| Parâmetro | Valor Mínimo | Recomendado |
|-----------|--------------|-------------|
| Largura de pista | 0.127mm (5mil) | 0.25mm (10mil) |
| Espaçamento | 0.127mm (5mil) | 0.2mm (8mil) |
| Via drill | 0.3mm | 0.4mm |
| Via pad | 0.6mm | 0.8mm |
| Camadas | 2 | 4 (se complexo) |
| Espessura | 1.6mm | 1.6mm |
| Cobre | 1oz | 2oz (power) |

### 5.2 Boas Práticas para ESP32

```
Layout Crítico:
═══════════════════════════════════════════════════════════════

1. ANTENA WiFi
   ┌────────────────────────────────────────────────────────┐
   │  ╔═══════════════════════════════════════════════════╗│
   │  ║                                                   ║│ ← Keep-out zone
   │  ║     [Antena ESP32]   NADA AQUI!                  ║│   15mm mínimo
   │  ║                      Sem cobre, vias, traços     ║│
   │  ╚═══════════════════════════════════════════════════╝│
   │                                                        │
   │  [ESP32-WROOM-32E]                                     │
   │                                                        │
   └────────────────────────────────────────────────────────┘

2. DECOUPLING - Condensadores o mais próximo possível dos pinos

   [ESP32]─────┬─────[100nF]─────GND
               │
               └─────[10µF]──────GND

3. POWER PLANES (se 4 camadas)
   Layer 1: Sinais + Componentes
   Layer 2: GND (plano contínuo)
   Layer 3: 3.3V / 5V
   Layer 4: Sinais + Componentes

4. I2C - Manter curto, pull-ups perto do master

   [ESP32]──────────────[4.7k]──────┬───── SDA ──────[DS3231]
           < 10cm                   │
                                   3.3V
```

### 5.3 Checklist Antes de Fabricar

- [ ] DRC (Design Rule Check) passa sem erros
- [ ] ERC (Electrical Rule Check) passa sem erros
- [ ] Verificar footprints vs datasheet
- [ ] Verificar polaridade de díodos e condensadores
- [ ] Antena ESP32 sem obstruções
- [ ] Furos de montagem (M3, 3.2mm)
- [ ] Silkscreen legível e correto
- [ ] Fiducials para SMT assembly
- [ ] Testpoints para debug

---

## 6. Ordem de Fabrico

### 6.1 Opções de Fabricante

| Fabricante | PCB 5pcs | Assembly | Lead Time | Notas |
|------------|----------|----------|-----------|-------|
| **JLCPCB** | $2-5 | $8+ setup | 5-7 dias | ⭐ Mais económico |
| **PCBWay** | $5-10 | $10+ setup | 5-7 dias | Boa qualidade |
| **ALLPCB** | $5-8 | Sim | 5-7 dias | Alternativa |
| **OSH Park** | $10+ | Não | 12+ dias | Made in USA |
| **Seeed Fusion** | $5-10 | Sim | 7-10 dias | Bom para protos |

### 6.2 Ficheiros para Encomendar

```
Para PCB:
├── Gerbers/
│   ├── project-F_Cu.gtl       (Top copper)
│   ├── project-B_Cu.gbl       (Bottom copper)
│   ├── project-F_Mask.gts     (Top solder mask)
│   ├── project-B_Mask.gbs     (Bottom solder mask)
│   ├── project-F_SilkS.gto    (Top silkscreen)
│   ├── project-B_SilkS.gbo    (Bottom silkscreen)
│   ├── project-Edge_Cuts.gm1  (Board outline)
│   └── project.drl            (Drill file)
│
Para Assembly (SMT):
├── BOM.csv                    (Bill of Materials)
└── CPL.csv                    (Component Placement List)
```

---

## 7. Estimativa de Custos

### 7.1 Protótipo (5 PCBs)

| Item | Custo |
|------|-------|
| PCB fabricação (5 pcs) | €5 |
| SMT assembly (opcional) | €15 |
| Componentes (5 sets) | €100 |
| Shipping | €15-30 |
| **Total** | **€135-150** |

### 7.2 Produção (100 pcs)

| Item | Custo Unit. | Total |
|------|-------------|-------|
| PCB | €0.50 | €50 |
| SMT assembly | €3.00 | €300 |
| Componentes | €8.00 | €800 |
| Shipping | - | €50 |
| **Total** | **€12/unidade** | **€1,200** |

---

## 8. Próximos Passos

1. **Semana 1:** Instalar KiCad 8 + bibliotecas
2. **Semana 2:** Desenhar esquemático completo
3. **Semana 3:** Layout PCB + DRC/ERC
4. **Semana 4:** Revisão + encomendar protótipos
5. **Semana 5-6:** Receber e testar protótipos
6. **Semana 7:** Iterar se necessário

---

*Documento criado: Dezembro 2024*
*Última atualização: Dezembro 2024*
*Versão: 1.1*
*Changelog: Adicionada secção 1.10 (Level Shifters) e 2.6 (Circuito Level Shifter) baseado em Adafruit MatrixPortal S3*
