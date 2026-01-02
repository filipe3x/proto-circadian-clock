# PCB Design Guide - Circadian Clock v2.1

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

### 1.2 Alimentação e Regulação (USB-C Power Delivery)

> **Referência:** Arquitetura baseada em [POWER_SUPPLY_v2.md](./POWER_SUPPLY_v2.md) - USB-C Power Delivery
> com duas estratégias possíveis: 5V Direto ou Alta Tensão + Buck Converter.

#### Estratégia A: 5V Direto (Simples - Prototipagem)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Notas |
|-----|------------|---------------|---------|-----|-------------|-------|
| J1 | USB-C Receptacle | 16-pin, USB 2.0 | SMD | 1 | €0.40 | Alimentação + dados |
| R_CC1, R_CC2 | Resistor 5.1kΩ | 1% 0.1W | 0402 | 2 | €0.01 | USB-C sink detection (5V/3A) |
| U3 | **AP2112K-3.3** | LDO 3.3V 600mA | SOT-23-5 | 1 | €0.15 | Para ESP32 (baixo dropout) |
| C_IN | Capacitor | 10µF 10V X5R | 0603 | 1 | €0.02 | Entrada LDO |
| C_OUT | Capacitor | 10µF 10V X5R | 0603 | 1 | €0.02 | Saída LDO |
| J2 | Screw Terminal | 2-pin 5.0mm 10A | THT | 1 | €0.10 | Alimentação painel LED |

> **Nota:** O AP2112K-3.3 foi escolhido em vez do AMS1117 por ter menor dropout (0.4V vs 1.0V),
> pino de enable para soft-start, e package compacto (SOT-23-5). Usado no Adafruit MatrixPortal S3.

#### Estratégia B: Alta Tensão + Buck Converter (Robusto - Produto Final)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Notas |
|-----|------------|---------------|---------|-----|-------------|-------|
| J1 | USB-C Receptacle | 16-pin, USB 2.0 | SMD | 1 | €0.40 | Alimentação + dados |
| U_PD | IP2721 | PD Controller 5-20V | DFN-10 | 1 | €0.35 | Negociação USB PD |
| R_SET | Resistor | Ver tabela tensão | 0402 | 1 | €0.01 | Seleção tensão PD |
| U_BUCK | MP1584EN | Buck 3A module | Module | 1 | €0.80 | Step-down para 5V |
| U3 | **AP2112K-3.3** | LDO 3.3V 600mA | SOT-23-5 | 1 | €0.15 | Para ESP32 |
| C_BUCK_IN | Capacitor | 100µF 35V electrolítico | Φ8mm | 1 | €0.15 | Entrada buck |
| C_BULK | Capacitor | 470µF 10V electrolítico | Φ10mm | 1 | €0.15 | Bulk capacitor saída |
| J2 | Screw Terminal | 2-pin 5.0mm 10A | THT | 1 | €0.10 | Alimentação painel LED |

**Resistor SET para IP2721 (Seleção de Tensão PD):**

| Tensão Desejada | Resistor SET | Corrente Cabo | Notas |
|-----------------|--------------|---------------|-------|
| 20V | 0Ω (curto) | 1.25A @ 25W | Menor corrente, menos perdas |
| 15V | 10kΩ | 1.67A @ 25W | Boa compatibilidade |
| 12V | 20kΩ | 2.08A @ 25W | **Recomendado** |
| 9V | 33kΩ | 2.78A @ 25W | Compatível com muitas fontes |
| 5V | 56kΩ | 5A @ 25W | Não recomendado (alta corrente) |

### 1.3 Proteções (CRÍTICO)

> **Referência:** Proteção USB-C completa baseada em [POWER_SUPPLY_v2.md](./POWER_SUPPLY_v2.md) secção 2.3.1

#### Proteção VBUS (Linha de Alimentação)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **F1** | **MF-MSMF300** | PTC Fuse 3A hold, 6A trip | 1812 | 1 | €0.15 | **Proteção curto-circuito/overcurrent** |
| **D1** | **SMBJ5.0A** | TVS 5V 600W unidirectional | SMB | 1 | €0.20 | **Proteção sobretensão VBUS** |
| **D1_HV** | **SMBJ24A** | TVS 24V (só Estratégia B) | SMB | 1 | €0.25 | **Proteção sobretensão alta tensão** |

#### Proteção Linhas de Dados e CC (USB-C)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **D2** | **D3V3XA4B10LP** | TVS Array 4-ch ESD ±15kV | UDFN2510 | 1 | €0.25 | **Proteção ESD D+/D-/CC1/CC2** |

> **Porquê D3V3XA4B10LP?**
> - 4 canais num único chip (protege D+, D-, CC1, CC2)
> - Capacitância ultra-baixa: 0.28pF (não afeta USB 2.0 a 480Mbps)
> - ESD rating: ±15kV (HBM), ±8kV (contacto)
> - Package compacto UDFN2510 (2.5x1.0mm)
> - LCSC: C2827654

#### Proteção Terminais de Parafuso (Saída para Painel)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **Q_REV** | **Si2301CDS** | P-MOSFET -20V -2.8A | SOT-23 | 1 | €0.10 | **Proteção inversão polaridade** |
| **D_BODY** | (integrado) | Body diode Si2301 | - | 0 | - | Bloqueio polaridade invertida |

#### Proteção Adicional (I2C, GPIOs)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **U5** | **SRV05-4** | TVS Diode Array | SOT-23-6 | 1 | €0.25 | **Proteção ESD I2C** |
| **Z1** | **PESD5V0S1BA** | ESD protection | SOD-323 | 2 | €0.08 | **ESD GPIO0 (botão), EN** |
| **D3** | **1N5819** | Schottky flyback | SOD-123 | 2 | €0.05 | **Proteção indutiva (futuro)** |

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
| LED2 | Vermelho | 3.2V 20mA | 0603 | 1 | WiFi status |

### 1.7 Botões e Interface

| Ref | Componente | Especificação | Package | Qty | Notas |
|-----|------------|---------------|---------|-----|-------|
| SW1 | Tactile Switch | 6x6mm, 160gf | Through-hole | 1 | Mode button (GPIO0) |

### 1.8 Buzzer e Audio (NOVO - Startup Sound)

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **BZ1** | **MLT-5030** | Piezo passivo 3V 4kHz 80dB | SMD 5.2x5.7mm | 1 | €0.20 | Startup sound/alarme |
| **Q2** | **2N2222** | NPN transistor 40V 800mA | SOT-23 | 1 | €0.05 | Driver buzzer |
| **R11** | **1kΩ** | Resistência 1% 0.1W | 0402 | 1 | €0.01 | Base transistor |
| **D4** | **1N4148** | Diodo switching 100V 150mA | SOD-123 | 1 | €0.02 | Proteção flyback |

**Código LCSC MLT-5030:** `C95297`

**Alternativas:**
- PKM13EPYH4000 (Murata) - LCSC C94599 - Melhor para >5kHz
- CMT-5023S-SMT (CUI) - Mais alto, 90dB

**Funcionalidades:**
- Startup sound retro estilo NES/GameBoy (pulse waves 8-bit)
- Configurável via captive portal (enable/disable)
- Suporta melodias customizáveis e arpeggios
- Futuro: alarmes e notificações sonoras

### 1.9 Conectores

| Ref | Componente | Especificação | Package | Qty | Função |
|-----|------------|---------------|---------|-----|--------|
| J3 | HUB75 Header | 2x8 pins 2.54mm | Through-hole | 1 | Painel LED P10 |

### 1.10 Conectores HUB75 (Painel LED)

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

### 1.11 Level Shifters (NOVO - Baseado em Adafruit MatrixPortal S3)

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

### 1.12 USB-UART Bridge e Auto-Reset

> **Nota:** O ESP32-WROOM-32E **não tem USB nativo**. Precisa de um chip externo
> para converter USB ↔ UART para programação via Arduino IDE/PlatformIO.

#### Componentes USB-UART

| Ref | Componente | Especificação | Package | Qty | Preço Unit. | Função |
|-----|------------|---------------|---------|-----|-------------|--------|
| **U8** | **CH340C** | USB-UART bridge | SOP-16 | 1 | €0.30 | Converte USB para UART |
| **Q1** | **UMH3N** | Dual NPN digital transistor | SOT-363 | 1 | €0.05 | Auto-reset (DTR/RTS) |
| C16 | 100nF | Cerâmico X7R 16V | 0402 | 1 | €0.01 | Bypass CH340C |

**Alternativas ao CH340C:**

| Chip | Preço | Vantagem | Desvantagem |
|------|-------|----------|-------------|
| CH340C | €0.30 | Barato, sem cristal externo | Driver Windows pode ser necessário |
| CP2102 | €0.80 | Drivers universais | Mais caro |
| FT232RL | €2.50 | Premium, muito compatível | Caro |

#### Porquê Auto-Reset?

```
SEM Auto-Reset:
━━━━━━━━━━━━━━━
1. Pressionar botão BOOT (GPIO0 → LOW)
2. Pressionar botão RESET (EN → LOW → HIGH)
3. Largar BOOT
4. Fazer upload no Arduino IDE
5. Repetir para cada upload... 😤

COM Auto-Reset:
━━━━━━━━━━━━━━━
1. Clicar "Upload" no Arduino IDE
2. Esperar... ✓ 😊
```

#### Circuito Auto-Reset com UMH3N

O **UMH3N** (ROHM) é um dual NPN digital transistor com resistências integradas.
Substitui 2 transistores + 4 resistências num único chip SOT-363.

```
                                VCC (3.3V)
                                    │
                                    │
        ┌───────────────────────────┴───────────────────────────┐
        │                                                       │
        │              UMH3N (SOT-363)                         │
        │           ┌───────────────────┐                       │
        │           │                   │                       │
 CH340C │           │  ┌─────┐ ┌─────┐  │                       │
  DTR ──┼───────────┤──┤ R1  ├─┤ Q1  ├──┼───────────────────────┼──► EN (ESP32)
        │           │  │ 10k │ │ NPN │  │                       │
        │           │  └─────┘ └──┬──┘  │                       │
        │           │             │     │                       │
        │           │            GND    │                       │
        │           │                   │                       │
 CH340C │           │  ┌─────┐ ┌─────┐  │                       │
  RTS ──┼───────────┤──┤ R2  ├─┤ Q2  ├──┼───────────────────────┼──► GPIO0 (ESP32)
        │           │  │ 10k │ │ NPN │  │                       │
        │           │  └─────┘ └──┬──┘  │                       │
        │           │             │     │                       │
        │           │            GND    │                       │
        │           │     (pin 5)       │                       │
        │           └───────────────────┘                       │
        │                                                       │
        └───────────────────────────────────────────────────────┘

UMH3N Pinout (SOT-363):
━━━━━━━━━━━━━━━━━━━━━━━
Pin 1: B1 (Base Q1)     ← DTR do CH340C
Pin 2: B2 (Base Q2)     ← RTS do CH340C
Pin 3: C1 (Collector Q1) → EN do ESP32
Pin 4: C2 (Collector Q2) → GPIO0 do ESP32
Pin 5: E (Emitters)     → GND
Pin 6: Common           → VCC (3.3V)
```

#### Ligações CH340C

```
CH340C (SOP-16) Pinout:
━━━━━━━━━━━━━━━━━━━━━━━

        ┌────────────────────┐
   GND ─┤1                 16├─ VCC (3.3V)
   TXD ─┤2                 15├─ UD+ (USB D+)
   RXD ─┤3                 14├─ UD- (USB D-)
   V3  ─┤4                 13├─ DTR → UMH3N pin 1
  UD+ ──┤5                 12├─ RTS → UMH3N pin 2
  UD- ──┤6                 11├─ NC
    NC ─┤7                 10├─ NC
    NC ─┤8                  9├─ NC
        └────────────────────┘

Ligações:
━━━━━━━━━
CH340C Pin 2 (TXD) → ESP32 RX (GPIO3)
CH340C Pin 3 (RXD) → ESP32 TX (GPIO1)
CH340C Pin 13 (DTR) → UMH3N Pin 1 (B1)
CH340C Pin 12 (RTS) → UMH3N Pin 2 (B2)
CH340C Pin 1, Pin 4 (GND, V3) → GND
CH340C Pin 16 (VCC) → 3.3V
CH340C Pin 5 (UD+) → USB-C D+
CH340C Pin 6 (UD-) → USB-C D-

Bypass capacitor:
C16 (100nF) entre VCC (pin 16) e GND (pin 1)
```

#### Diagrama de Blocos USB-UART

```
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│   USB-C          CH340C              UMH3N           ESP32          │
│  ┌──────┐      ┌────────┐          ┌──────┐      ┌─────────┐       │
│  │      │      │        │          │      │      │         │       │
│  │  D+  ├──────┤UD+  TXD├──────────┼──────┼──────┤RX (IO3) │       │
│  │      │      │        │          │      │      │         │       │
│  │  D-  ├──────┤UD-  RXD├──────────┼──────┼──────┤TX (IO1) │       │
│  │      │      │        │          │      │      │         │       │
│  │ VBUS ├──┐   │    DTR ├──────────┤B1  C1├──────┤EN       │       │
│  │      │  │   │        │          │      │      │         │       │
│  │  GND ├──┼───┤GND RTS ├──────────┤B2  C2├──────┤GPIO0    │       │
│  │      │  │   │        │          │      │      │         │       │
│  └──────┘  │   │    VCC ├───3.3V   │  E   │      │         │       │
│            │   └────────┘          └──┬───┘      └─────────┘       │
│            │                          │                             │
│            │        5V rail           GND                           │
│            └──────────┘                                             │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

#### Sequência Auto-Reset

```
Quando Arduino IDE inicia upload:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

1. RTS → LOW  (via UMH3N: GPIO0 → LOW = boot mode)
2. DTR → LOW  (via UMH3N: EN → LOW = reset)
3. DTR → HIGH (EN → HIGH = chip inicia)
4. RTS → HIGH (GPIO0 livre, mas já em bootloader)
5. Upload do firmware via UART
6. Reset automático após upload
```

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

#### 2.3.1 Proteção USB-C Completa (VBUS + D+/D- + CC1/CC2)

> **Referência:** [POWER_SUPPLY_v2.md](./POWER_SUPPLY_v2.md) secção 2.3.1

```
Proteção USB-C Completa:
═══════════════════════════════════════════════════════════════

  USB-C Connector
  ┌─────────────────┐
  │                 │
  │ VBUS ───────────┼───┬───[F1 PTC]───[D1 SMBJ5.0A]───┬─► +5V
  │                 │   │                              │
  │                 │   │   ┌──────────────────────────┘
  │                 │   │   │
  │                 │   │   │    ┌─────────────────────────┐
  │                 │   │   │    │    D2: D3V3XA4B10LP     │
  │                 │   │   │    │    (4-channel ESD TVS)  │
  │                 │   │   │    │                         │
  │ D+ ─────────────┼───┼───┼────┤ CH1 ────────────────────┼─► D+ (CH340C)
  │                 │   │   │    │                         │
  │ D- ─────────────┼───┼───┼────┤ CH2 ────────────────────┼─► D- (CH340C)
  │                 │   │   │    │                         │
  │ CC1 ────────────┼───┼───┼────┤ CH3 ──┬──[5.1kΩ]── GND │
  │                 │   │   │    │       │                 │
  │ CC2 ────────────┼───┼───┼────┤ CH4 ──┼──[5.1kΩ]── GND │
  │                 │   │   │    │       │                 │
  │ GND ────────────┼───┴───┴────┤ GND ──┴─────────────────┘
  │                 │            │
  └─────────────────┘            └─────────────────────────┘

═══════════════════════════════════════════════════════════════

  Componentes de Proteção:
  ────────────────────────

  ┌─────────────┬────────────────┬─────────────────────────────┐
  │ Componente  │ Protege        │ Características             │
  ├─────────────┼────────────────┼─────────────────────────────┤
  │ SMBJ5.0A    │ VBUS (5V)      │ 600W surge, alta corrente   │
  │ D3V3XA4B10LP│ D+, D-, CC1/2  │ 0.28pF, ESD ±15kV          │
  │ PTC Fuse    │ Overcurrent    │ 3A hold, 6A trip           │
  └─────────────┴────────────────┴─────────────────────────────┘

  Porque dois TVS diferentes?
  ───────────────────────────
  • VBUS: Precisa de alta capacidade de corrente (surges de rede)
         → SMBJ5.0A (600W, pode absorver grandes surges)

  • D+/D-: Precisa de baixa capacitância (USB 2.0 = 480Mbps)
         → D3V3XA4B10LP (0.28pF, não afecta sinal)

  • CC1/CC2: Comunicação com carregador PD
         → D3V3XA4B10LP (protege negociação PD)

═══════════════════════════════════════════════════════════════
```

#### 2.3.2 Proteção I2C

```
Proteção I2C com SRV05-4
═══════════════════════════════════════════════════════════════

              ┌─────────────────────────┐
    I2C_SDA ──┤1    SRV05-4 (U5)      4├─── VCC (3.3V)
    I2C_SCL ──┤2                      5├─── NC
    GND ──────┤3                      6├─── NC
              └─────────────────────────┘

Especificações SRV05-4:
- Tensão de clamping: 8.5V @ 1A
- Capacitância: 0.5pF (baixa, boa para dados)
- ESD rating: ±15kV (ar), ±8kV (contacto)
```

#### 2.3.3 Proteção GPIO Críticos (Botão, etc.)

```
Proteção GPIO0 (Botão Mode)
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

### 2.6 Circuito Buzzer para Startup Sound (NOVO)

```
Driver Transistor para Buzzer Piezo Passivo
═══════════════════════════════════════════════════════════════

                          VCC (+5V)
                            │
                            ├────────┐
                            │        │
                        ┌───┴───┐   ─┴─ D4 (1N4148)
                        │  BZ1  │   ─┬─ Flyback diode
                        │MLT5030│    │
                        │ Piezo │    │
                        └───┬───┘    │
                            │        │
                            └────────┤
                                     │ C (Collector)
    ESP32                          ┌─┴─┐
    GPIO18 ──────[R11]─────────────┤ B │ Q2 (2N2222)
                  1kΩ               └─┬─┘ NPN SOT-23
                                     │ E (Emitter)
                                     │
                                    GND

Especificações:
───────────────
- GPIO: GPIO 18 (PWM channel 0, suporta ledcWrite)
- R11: 1kΩ (limita corrente base, Ib ≈ 3mA)
- Q2: 2N2222 (hFE ≈ 100-300, Ic_max = 800mA, mais que suficiente)
- D4: 1N4148 (proteção flyback contra picos indutivos)
- BZ1: MLT-5030 (Vop = 3V, I = 100mA @ ressonância 4kHz)

**Nota:** GPIO 18 foi escolhido porque GPIO 6-11 estão internamente
ligados à flash SPI do ESP32-WROOM-32E e não devem ser usados.

Funcionamento:
──────────────
1. GPIO18 = HIGH (3.3V via PWM):
   - Corrente base: Ib = (3.3V - 0.7V) / 1kΩ ≈ 2.6mA
   - Corrente coletor: Ic = Ib × hFE ≈ 2.6mA × 100 = 260mA (suficiente!)
   - Transistor satura → Buzzer toca

2. GPIO18 = LOW (0V):
   - Transistor corta → Buzzer silencioso
   - D4 dissipa energia indutiva residual

3. PWM para melodias:
   - Frequências: 20Hz - 40kHz (ledcWriteTone)
   - Duty cycle: 12.5%, 25%, 50% (pulse waves retro)
   - Ressonância ótima: 4kHz (MLT-5030 spec)

Técnicas de Som Retro:
──────────────────────
- **Pulse Wave 25%**: Som clássico NES (duty = 64/256)
- **Arpeggios**: C-E-G rápido = acorde "falso"
- **Pitch Bend**: Sweep de frequência (laser, power-up)
- **Vibrato**: Oscilação ±10Hz @ 20Hz

Exemplo código:
───────────────
void setup() {
  ledcSetup(0, 2000, 8);      // Canal 0, 2kHz, 8-bit
  ledcAttachPin(18, 0);        // GPIO18
}

void startupSound() {
  // Melodia: C5-C5-C5-C6 (GameBoy style)
  tone(523, 100, 64);   // C5, 100ms, duty 25%
  delay(120);
  tone(523, 100, 64);
  delay(120);
  tone(523, 150, 64);
  delay(180);
  tone(1047, 300, 64);  // C6
}

void tone(int freq, int dur, int duty) {
  ledcWriteTone(0, freq);
  ledcWrite(0, duty);
  delay(dur);
  ledcWrite(0, 0);  // Silêncio
}

Layout Tips PCB:
────────────────
1. Q2 próximo do buzzer BZ1 (minimizar trace Collector)
2. R11 próximo do GPIO18 header
3. D4 paralelo ao buzzer, o mais próximo possível
4. Ground return path curto e direto
5. Buzzer longe da antena WiFi (ruído EMI)

Configuração Captive Portal:
────────────────────────────
- Toggle ON/OFF: /api/buzzer/enable
- Volume (via duty): /api/buzzer/volume (12.5%, 25%, 50%)
- Melodia select: /api/buzzer/melody (startup, alarm, error)
```

### 2.7 Inrush Current Limiting (Soft-Start)

> **Referência:** [POWER_SUPPLY_v2.md](./POWER_SUPPLY_v2.md) secção 4.3

```
Proteção contra Inrush Current:
═══════════════════════════════════════════════════════════════

  Problema: Ao ligar, condensadores grandes causam pico de corrente
            que pode disparar proteções da fonte USB.

  Solução 1: NTC Thermistor (Simples e Económico)
  ───────────────────────────────────────────────

  +5V_USB ──[NTC 10Ω@25°C]───► +5V_LOAD
                │
               ═╧═
           C_BULK 470µF

  - Frio: alta resistência, limita corrente (~0.5A inicial)
  - Aquece: baixa resistência (<1Ω), operação normal
  - Custo: €0.10
  - Desvantagem: não recupera instantaneamente após desligar

═══════════════════════════════════════════════════════════════

  Solução 2: P-MOSFET com RC Delay (Melhor performance)
  ─────────────────────────────────────────────────────

  +5V_USB ──┬───[R1 10Ω]───┬───────────────────────► +5V_LOAD
            │               │
            │        ┌──────┴──────┐
            │        │   Q1 P-FET  │
            │        │  (Si2301)   │
            │        │   S     D   │
            │        └──────┬──────┘
            │               │
            │     ┌─────────┘
            │     │
            │     │   ┌─────────────┐
            │     │   │   RC Timer  │
            │     └───┤   100ms     │
            │         │   R=100k    │
            │         │   C=1µF     │
            │         └──────┬──────┘
            │                │
  GND ──────┴────────────────┴────────────────────────► GND

  Sequência:
  1. t=0: Fonte liga, corrente passa por R1 (limitada a 0.5A)
  2. t=100ms: Timer ativa Q1, MOSFET bypassa R1
  3. t>100ms: Corrente flui livremente pelo MOSFET (<0.1Ω)

═══════════════════════════════════════════════════════════════
```

### 2.8 Proteção nos Terminais de Parafuso (Inversão de Polaridade)

> **Referência:** [POWER_SUPPLY_v2.md](./POWER_SUPPLY_v2.md) secção 5.2

```
Proteção contra Ligação Inversa nos Terminais:
═══════════════════════════════════════════════════════════════

  ⚠️ Utilizadores podem ligar fios invertidos!

  Solução: P-MOSFET com Body Diode
  ─────────────────────────────────

  +5V_INTERNO ──┬──[Q1 Si2301]──┬──[TERMINAL +]
                │    P-MOSFET   │
                │               │
               ─┴─ D1          ═╧═ C1
            Body diode         100µF
                │               │
  GND ──────────┴───────────────┴──[TERMINAL -]

  Funcionamento:
  - Polaridade correcta: Vgs negativo → MOSFET conduz (Rds ~50mΩ)
  - Polaridade invertida: Vgs positivo → MOSFET bloqueia
  - Perda: ~0.05W @ 1A (muito menor que díodo Schottky)

═══════════════════════════════════════════════════════════════

  Componentes:
  ────────────
  Q1: Si2301CDS (P-ch MOSFET, -20V, -2.8A, Rds=80mΩ)
  R_GATE: 10kΩ (pull-down gate para GND)

  Alternativa simples (com mais perda):
  ─────────────────────────────────────

  +5V_INTERNO ──[D1 SS54 Schottky]──[TERMINAL +]

  Perda: 0.3V × 3A = 0.9W (aceitável mas gera calor)

═══════════════════════════════════════════════════════════════
```

### 2.9 Level Shifter 3.3V → 5V para HUB75

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

> **Referência:** Arquitetura baseada em [POWER_SUPPLY_v2.md](./POWER_SUPPLY_v2.md)

### 3.1 Estratégia A: 5V Direto (Prototipagem)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CIRCADIAN CLOCK PCB v2.1 - ESTRATÉGIA A                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌────────────┐      ┌─────────────────────────────────────────────────┐   │
│  │            │      │                    PCB                          │   │
│  │  FONTE     │      │                                                 │   │
│  │  USB-C     │      │   ┌────────┐    ┌─────────┐    ┌────────────┐  │   │
│  │  5V/3A     ├──────┤──►│ USB-C  │───►│ FUSÍVEL │───►│ TERMINAIS  │  │   │
│  │  (15W)     │      │   │ Recept.│    │+TVS+ESD │    │ PARAFUSO   │  │   │
│  │            │      │   └────┬───┘    └────┬────┘    └─────┬──────┘  │   │
│  └────────────┘      │        │             │               │         │   │
│                      │        │  D3V3XA4B10LP               │         │   │
│                      │        │  (ESD D+/D-/CC)             ▼         │   │
│                      │        ▼             │         ┌───────────┐   │   │
│                      │   ┌────────┐         │         │  PAINEL   │   │   │
│                      │   │ CH340C │         │         │   P10     │   │   │
│                      │   │ USB-   │         │         │ 32x16 RGB │   │   │
│                      │   │ UART   │         │         └───────────┘   │   │
│                      │   └────┬───┘         ▼                         │   │
│                      │        │       ┌──────────┐                    │   │
│                      │        │       │ AP2112K  │                    │   │
│                      │        │       │  3.3V    │                    │   │
│                      │        │       └────┬─────┘                    │   │
│                      │        │            │                          │   │
│                      │        ▼            ▼                          │   │
│                      │   ┌─────────────────────┐                      │   │
│                      │   │      ESP32          │◄──► DS3231 RTC       │   │
│                      │   │   (3.3V logic)      │                      │   │
│                      │   └─────────┬───────────┘                      │   │
│                      │             │                                  │   │
│                      │             ▼                                  │   │
│                      │   ┌──────────────────┐                         │   │
│                      │   │   74AHCT245 (x2) │──► HUB75 Header        │   │
│                      │   │   3.3V → 5V      │                         │   │
│                      │   └──────────────────┘                         │   │
│                      └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Estratégia B: Alta Tensão + Buck (Produto Final)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CIRCADIAN CLOCK PCB v2.1 - ESTRATÉGIA B                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌────────────┐                                                            │
│   │            │      ┌──────────────────────────────────────────────────┐ │
│   │  FONTE     │      │                      PCB                         │ │
│   │  USB-C PD  │      │                                                  │ │
│   │  45-65W    │      │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐ │ │
│   │            ├──────┤─►│ USB-C  │─►│  PD    │─►│ BUCK   │─►│TERMINAL│ │ │
│   │  9V/12V/   │      │  │ Recept.│  │IP2721  │  │MP1584EN│  │PARAFUSO│ │ │
│   │  15V/20V   │      │  └────┬───┘  └────┬───┘  └────┬───┘  └───┬────┘ │ │
│   │            │      │       │           │           │          │      │ │
│   └────────────┘      │       │  D3V3XA4B10LP         │          │      │ │
│                       │       │  (ESD D+/D-/CC)       │          ▼      │ │
│                       │       ▼           │           ▼    ┌────────┐   │ │
│                       │  ┌────────┐       │     ┌────────┐ │ PAINEL │   │ │
│                       │  │ CH340C │       │     │ +5V    │ │  P10   │   │ │
│                       │  │ USB-   │       │     │ Rail   │ │ (5V)   │   │ │
│                       │  │ UART   │       │     └───┬────┘ └────────┘   │ │
│                       │  └────┬───┘       │         │                   │ │
│                       │       │           ▼         ▼                   │ │
│                       │       │     ┌──────────┐  ┌───────────┐         │ │
│                       │       │     │ AP2112K  │  │ 74AHCT245 │         │ │
│                       │       │     │  3.3V    │  │   (x2)    │         │ │
│                       │       │     └────┬─────┘  └─────┬─────┘         │ │
│                       │       │          │              │               │ │
│                       │       ▼          ▼              ▼               │ │
│                       │  ┌────────────────────────────────────┐         │ │
│                       │  │            ESP32 + DS3231          │         │ │
│                       │  │                                    │         │ │
│                       │  │  ← UART      3.3V logic    HUB75 →│         │ │
│                       │  └────────────────────────────────────┘         │ │
│                       │                                                  │ │
│                       └──────────────────────────────────────────────────┘ │
│                                                                             │
│   Vantagens Estratégia B:                                                   │
│   ✓ Fontes 45-65W USB-C são MUITO comuns (laptops, tablets)                │
│   ✓ Menor corrente no cabo = menores perdas (I²R)                          │
│   ✓ Buck converter fornece 5V estável com alta eficiência (>90%)           │
│   ✓ Maior margem para picos de corrente                                    │
│   ✓ Brilho 100% disponível sem restrições                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. Software para Design de PCB

### 4.1 Opções Recomendadas

| Software | Custo | Nível | Prós | Contras | Recomendação |
|----------|-------|-------|------|---------|--------------|
| **KiCad 9** | Grátis | Intermédio | Open-source, completo, JLCPCB integração | Curva aprendizagem | ⭐⭐⭐⭐⭐ **RECOMENDADO** |
| **EasyEDA** | Grátis | Iniciante | Browser-based, integração JLCPCB/LCSC | Limitado offline | ⭐⭐⭐⭐ Bom para começar |
| **Altium Designer** | €3000+/ano | Profissional | Industry standard, poderoso | Caro, complexo | ⭐⭐⭐ Overkill |
| **Eagle** | €500/ano | Intermédio | Estabelecido, boas libs | Autodesk lock-in | ⭐⭐⭐ |
| **Fusion 360 Electronics** | Grátis* | Iniciante | Integrado com CAD mecânico | Limitações grátis | ⭐⭐⭐ |

### 4.2 Workflow Recomendado: KiCad 9

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

**Instalação KiCad 9:**
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

### 6.3 BOM USB-C Power (Resumo)

> **Referência:** BOM completo em [POWER_SUPPLY_v2.md](./POWER_SUPPLY_v2.md) secção 7

#### Estratégia A: 5V Direto (€1.82 total power + proteção)

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC |
|-----|------------|---------------|---------|-----|-------|------|
| J1 | USB-C Receptacle | 16-pin, USB 2.0 | SMD | 1 | €0.40 | C165948 |
| R_CC1, R_CC2 | Resistor 5.1kΩ | 1% 0.1W | 0402 | 2 | €0.01 | C25905 |
| F1 | PTC Fuse | 3A hold, 6A trip | 1812 | 1 | €0.15 | C369159 |
| D1 | TVS SMBJ5.0A | 5V 600W (VBUS) | SMB | 1 | €0.20 | C123799 |
| D2 | TVS D3V3XA4B10LP | 4-ch ESD (D+/D-/CC) | UDFN2510 | 1 | €0.25 | C2827654 |
| C_BULK | Capacitor | 470µF 16V electrolítico | Φ10mm | 1 | €0.15 | C134768 |
| U3 | AP2112K-3.3 | LDO 3.3V 600mA | SOT-23-5 | 1 | €0.15 | C51118 |
| J2 | Screw Terminal | 2-pin 5.0mm 10A | THT | 1 | €0.10 | C474881 |
| LED1, LED2 | LED Verde | 0805 ~2V 20mA | 0805 | 2 | €0.04 | C2297 |
| R_LED1, R_LED2 | Resistor | 1kΩ 1% | 0402 | 2 | €0.02 | C11702 |

#### Estratégia B: Alta Tensão + Buck (€3.45 total power + proteção)

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC |
|-----|------------|---------------|---------|-----|-------|------|
| J1 | USB-C Receptacle | 16-pin, USB 2.0 | SMD | 1 | €0.40 | C165948 |
| U_PD | IP2721 | PD Controller 5-20V | DFN-10 | 1 | €0.35 | C2889848 |
| R_SET | Resistor | 20kΩ (para 12V) | 0402 | 1 | €0.01 | - |
| U_BUCK | MP1584EN | Buck 3A module | Module | 1 | €0.80 | Módulo |
| F1 | PTC Fuse | 3A hold | 1812 | 1 | €0.15 | C369159 |
| D1 | TVS SMBJ24A | 24V (VBUS HV) | SMB | 1 | €0.25 | C114152 |
| D2 | TVS D3V3XA4B10LP | 4-ch ESD (D+/D-/CC) | UDFN2510 | 1 | €0.25 | C2827654 |
| C_BUCK_IN | Capacitor | 100µF 35V electrolítico | Φ8mm | 1 | €0.15 | C249490 |
| C_BULK | Capacitor | 470µF 10V electrolítico | Φ10mm | 1 | €0.15 | C134768 |
| U3 | AP2112K-3.3 | LDO 3.3V 600mA | SOT-23-5 | 1 | €0.15 | C51118 |
| J2 | Screw Terminal | 2-pin 5.0mm 10A | THT | 1 | €0.10 | C474881 |
| Q_REV | Si2301CDS | P-MOSFET inversão | SOT-23 | 1 | €0.10 | C10487 |
| LED1, LED2 | LED Verde | 0805 ~2V 20mA | 0805 | 2 | €0.04 | C2297 |
| R_LED1, R_LED2 | Resistor | 1kΩ 1% | 0402 | 2 | €0.02 | C11702 |

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

1. **Semana 1:** Instalar KiCad 9 + bibliotecas
2. **Semana 2:** Desenhar esquemático completo
3. **Semana 3:** Layout PCB + DRC/ERC
4. **Semana 4:** Revisão + encomendar protótipos
5. **Semana 5-6:** Receber e testar protótipos
6. **Semana 7:** Iterar se necessário

---

*Documento criado: Dezembro 2025*
*Última atualização: Janeiro 2026*
*Versão: 2.1*

**Changelog v2.1 (Jan 2026):**
- **Secção 1.8: Adicionada** Buzzer e Audio para startup sound retro estilo NES/GameBoy
  - MLT-5030 piezo passivo (LCSC C95297) com driver 2N2222
  - BOM completo: BZ1, Q2, R11, D4
  - Alternativas: PKM13EPYH4000 (Murata), CMT-5023S-SMT (CUI)
- **Secção 2.6: Adicionada** Circuito Buzzer completo com driver transistor NPN
  - GPIO 18 (escolhido porque GPIO 6-11 são flash SPI)
  - Exemplo código PWM com melodias retro
  - Técnicas: Pulse waves, arpeggios, pitch bend, vibrato
  - Layout tips PCB e configuração captive portal
- Renumeração de secções: 1.8→1.9 (Conectores), 2.6→2.7 (Inrush), 2.7→2.8 (Terminais), 2.8→2.9 (Level Shifter)

**Changelog v2.0 (Dez 2025):**
- Secção 1.2: Atualizada com USB-C Power Delivery (Estratégias A e B), AP2112K, PD controller IP2721, Buck converter MP1584EN, terminais de parafuso
- Secção 1.3: Atualizada com proteção ESD USB-C completa (D3V3XA4B10LP para D+/D-/CC1/CC2), proteção VBUS (SMBJ5.0A/SMBJ24A), proteção terminais (Si2301CDS)
- Secção 2.3: Adicionada proteção USB-C detalhada com circuito completo
- Secção 2.7: Adicionada proteção inrush current com NTC thermistor e P-MOSFET soft-start
- Secção 2.8: Adicionada proteção inversão polaridade nos terminais de parafuso
- Secção 3: Atualizado esquema de blocos com duas estratégias (A: 5V Direto, B: Alta Tensão + Buck)
- Secção 6.3: Adicionado BOM resumo para USB-C Power (Estratégias A e B)
- Referências cruzadas com POWER_SUPPLY_v2.md

**Changelog v1.1 (Dez 2025):**
- Adicionada secção 1.11 (Level Shifters) e 2.9 (Circuito Level Shifter) baseado em Adafruit MatrixPortal S3
