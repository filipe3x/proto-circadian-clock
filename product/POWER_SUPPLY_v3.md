# Power Supply v3 - USB-C PD 30W com Buck de Alta Corrente

Este documento descreve a arquitetura de alimentação v3 baseada em USB-C Power Delivery, otimizada para 30W máximo com buck de alta corrente (8A).

---

## Changelog

| Data | Versão | Alterações |
|------|--------|------------|
| Jan 2026 | 3.2 | **Substituição buck converter**: TPS56838 → **TPS56838** (TI). Motivo: coil whine em modo PFM a light load. TPS56838 opera em FCCM nativo, D-CAP3 sem compensação externa, 28V max. Adicionadas lições PCB: Zone Keepout sob indutor, minimizar cobre LX, C_FF obrigatório. |
| Jan 2026 | 3.1 | **Corrigido valores feedback**: R_FB1=22kΩ/R_FB2=3kΩ (era 16.2kΩ/3.01kΩ - erro de cálculo). Atualizado L1 LCSC para C2831487. Atualizadas referências para coincidir com KiCad: U10 (IP2721), U12 (TPS56838), Q3 (MOSFET). |
| Jan 2025 | 3.0 | Versão inicial com IP2721 + TPS56838 |

---

## 1. Visão Geral

### 1.1 Filosofia de Design

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      POWER SUPPLY v2 vs v3                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   v2 (Anterior)                        v3 (Novo)                            │
│   ═════════════                        ═════════                            │
│                                                                             │
│   • Duas estratégias separadas         • Uma estratégia unificada           │
│     (5V direto OU PD+Buck)               (PD+Buck com fallback inteligente) │
│                                                                             │
│   • IP2721 ou CYPD3177                 • IP2721 + AO3400A MOSFET            │
│     (configuração complexa)              (power-path controlado, 1 resistor) │
│                                                                             │
│   • MP1584EN (3A) ou TPS54531 (5A)     • TPS56838 (8A síncrono)          │
│     (margem limitada)                    (folga para picos HUB75)           │
│                                                                             │
│   • 5V/5A ou tensão alta               • 15V preferido, 9V fallback         │
│     (dependia da fonte)                  (30W garantido na maioria)         │
│                                                                             │
│   • Brilho fixo ou manual              • Brilho automático baseado          │
│                                          na potência disponível             │
│                                                                             │
│   Objectivo v3:                                                             │
│   ─────────────                                                             │
│   ✓ 30W máximo (15V @ 2A ou 9V @ 3A)                                       │
│   ✓ Buck robusto com 8A de capacidade                                      │
│   ✓ Fallback automático para fontes menores                                │
│   ✓ "Funciona com quase tudo" - qualquer PD 27W+                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Especificações Alvo

| Parâmetro | Mínimo | Típico | Máximo | Notas |
|-----------|--------|--------|--------|-------|
| Potência entrada | 27W | 30W | 45W | USB-C PD |
| Tensão entrada | 9V | 15V | 20V | PD negociado |
| Tensão saída | 4.9V | 5.0V | 5.1V | ±2% regulação |
| Corrente saída | - | 5A | 8A | Picos HUB75 |
| Eficiência | 90% | 93% | - | @ 15V→5V, 3A |

### 1.3 Perfis de Potência Suportados

```
Estratégia de Negociação PD:
═══════════════════════════════════════════════════════════════

  PRIORIDADE 1: 20V (Preferido) ★
  ────────────────────────────────
  • 20V @ 1.5A = 30W → 5V @ 5.6A (após buck 93%)
  • 20V @ 3A = 60W → 5V @ 11A (limitado pelo buck a 8A)

  ✓ Disponível em ~85% das fontes PD (laptops, tablets)
  ✓ Menor corrente no cabo = menores perdas I²R
  ✓ Maior margem de potência disponível
  ✓ Ratio 4:1 ainda com boa eficiência (~91%)

  PRIORIDADE 2: 15V (Fallback)
  ─────────────────────────────
  • 15V @ 2A = 30W → 5V @ 5.6A
  • 15V @ 3A = 45W → 5V @ 8.4A

  ✓ Disponível em ~70% das fontes PD
  ✓ Ratio 3:1 ligeiramente melhor eficiência (~93%)

  PRIORIDADE 3: 9V (Fallback)
  ───────────────────────────
  • 9V @ 3A = 27W → 5V @ 4.8A (após buck 90%)

  ✓ Disponível em ~50% das fontes PD
  ⚠️ Brilho máximo ~80% (capado em software)
  ⚠️ Corrente mais alta no cabo (3A)

  FALLBACK FINAL: 5V (Emergência)
  ────────────────────────────────
  • 5V @ 3A = 15W → Bypass direto (sem buck)

  ⚠️ Brilho máximo ~40% (capado em software)
  ⚠️ Apenas para carregadores básicos
  ⚠️ Requer detecção de tensão para bypass

═══════════════════════════════════════════════════════════════
```

---

## 2. Arquitectura do Sistema

### 2.1 Diagrama de Blocos

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    PSU v3: ARQUITECTURA COMPLETA                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌────────────┐                                                            │
│   │            │      ┌──────────────────────────────────────────────────┐ │
│   │  FONTE     │      │                      PCB                         │ │
│   │  USB-C PD  │      │                                                  │ │
│   │  27-45W    │      │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐ │ │
│   │            ├──────┤─►│ USB-C  │─►│ IP2721 │─►│TPS56838 │─►│TERMINAL│ │ │
│   │  9V/15V/   │      │  │ Recept.│  │ +MOSFET│  │ BUCK   │  │PARAFUSO│ │ │
│   │  20V       │      │  │        │  │AO3400A │  │ 8A     │  │        │ │ │
│   │            │      │  └────┬───┘  └───┬────┘  └───┬────┘  └───┬────┘ │ │
│   └────────────┘      │       │          │           │           │      │ │
│                       │       │   20V (ou 15V/9V)    │    5V     │      │ │
│                       │       ▼          │           ▼           ▼      │ │
│                       │  ┌────────┐      │     ┌─────────┐  ┌────────┐  │ │
│                       │  │ CH340C │      │     │ +5V     │  │ PAINEL │  │ │
│                       │  │ USB-   │      │     │ RAIL    │  │  P10   │  │ │
│                       │  │ UART   │      │     │ (8A)    │  │ HUB75  │  │ │
│                       │  └────┬───┘      │     └────┬────┘  └────────┘  │ │
│                       │       │          │          │                   │ │
│                       │       │          │          ├──► AP2112K 3.3V   │ │
│                       │       │          │          │                   │ │
│                       │       │          │          ├──► 74AHCT245 (x2) │ │
│                       │       │          │          │                   │ │
│                       │       ▼          ▼          ▼                   │ │
│                       │  ┌─────────────────────────────────────┐        │ │
│                       │  │              ESP32                  │        │ │
│                       │  │                                     │        │ │
│                       │  │  ┌─────────────┐  ┌──────────────┐  │        │ │
│                       │  │  │ VSENSE_ADC  │  │ Power Manager│  │        │ │
│                       │  │  │ (GPIO36)    │──│ (Firmware)   │  │        │ │
│                       │  │  └─────────────┘  └──────────────┘  │        │ │
│                       │  │                                     │        │ │
│                       │  └─────────────────────────────────────┘        │ │
│                       │                                                  │ │
│                       └──────────────────────────────────────────────────┘ │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Fluxo de Potência

```
Fluxo de Energia (15V @ 2A = 30W entrada):
═══════════════════════════════════════════════════════════════

  USB-C         IP2721          TPS56838          Cargas
  Fonte         PD Trigger      Buck Sync
  ─────         ──────────      ─────────        ──────
    │                │               │               │
    │    VBUS        │               │               │
    ├────15V @ 2A────►               │               │
    │    (30W)       │               │               │
    │                │               │               │
    │                │  VIN = 15V    │               │
    │                ├───────────────►               │
    │                │               │               │
    │                │               │  VOUT = 5V    │
    │                │               ├───────────────►
    │                │               │  Iout = 5.6A  │
    │                │               │  (28W)        │
    │                │               │               │
    │                │    Perdas:    │               │
    │                │    Buck ~2W   │               │
    │                │    (93% eff)  │               │
    │                │               │               │
                                                     │
                                     ┌───────────────┤
                                     │               │
                              Painel P10       ESP32 +
                              ~5A (25W)        Periféricos
                                               ~0.5A (2.5W)

═══════════════════════════════════════════════════════════════
```

---

## 3. Componentes Principais

### 3.1 U10 - IP2721 (PD Trigger) + Q3 - AO3400A/AO3404A (Power MOSFET)

O IP2721 é um controlador USB-C PD 2.0/3.0 sink com controlo de MOSFET externo integrado via pino VBUSG.

```
IP2721 - USB-C PD Sink Controller:
═══════════════════════════════════════════════════════════════

  Características:
  ─────────────────
  • Suporta USB PD 2.0/3.0
  • Tensões máximas: 5V, 15V, 20V (via pino SEL)
  • Fallback automático para tensão máxima suportada
  • Controlo de MOSFET externo via VBUSG
  • Protecção integrada (OCP, OVP, short)
  • Package: TSSOP-16 (4.4x5mm, pitch 0.65mm)
  • LCSC: C603176 (Extended)

  Pinout IP2721:
  ──────────────

        ┌──────────────────────────────────┐
        │          IP2721 (TSSOP-16)        │
        │                                  │
   VIN ─┤1   VIN               VBUS   16├── VBUS (entrada USB-C)
  VOUT ─┤2   VOUT              GND    15├── GND
   N/C ─┤3   N/C               GND    14├── GND
 VBUSG ─┤4   VBUSG             CC2    13├── CC2 (do USB-C)
   GND ─┤5   GND               CC1    12├── CC1 (do USB-C)
   GND ─┤6   GND               N/C    11├── N/C
   SEL ─┤7   SEL               N/C    10├── N/C
   N/C ─┤8   N/C               N/C     9├── N/C
        │                                  │
        └──────────────────────────────────┘

  Configuração de Tensão (pino SEL):
  ──────────────────────────────────

  ┌─────────────────┬─────────────────┬─────────────────────────┐
  │ Estado SEL      │ Tensão Máxima   │ Comportamento           │
  ├─────────────────┼─────────────────┼─────────────────────────┤
  │ High (100kΩ→VIN)│ 20V ★           │ Fallback: 15V→12V→9V→5V │
  │ Floating        │ 15V             │ Fallback: 12V→9V→5V     │
  │ GND             │ 5V              │ Sem PD (USB básico)     │
  └─────────────────┴─────────────────┴─────────────────────────┘

  ★ Configuração recomendada para v3: 20V (SEL → 100kΩ → VIN)

  💡 Usar jumper/0Ω para facilitar mudança de tensão em produção

═══════════════════════════════════════════════════════════════

AO3400A - N-Channel MOSFET (Power Path):
═══════════════════════════════════════════════════════════════

  Características:
  ─────────────────
  • Vds: 30V (adequado para 20V PD)
  • Id: 5.7A contínua (suficiente para aplicação)
  • Rds(on): 26mΩ @ Vgs=4.5V
  • Vgs(th): 1.0-2.5V
  • Package: SOT-23
  • LCSC: C20917 (Basic - muito comum!)

  Porquê MOSFET externo?
  ──────────────────────
  • IP2721 controla o gate via pino VBUSG
  • Liga MOSFET quando CC connection estabelecida
  • Desliga MOSFET quando USB-C desconectado
  • Soft-start controlado (limita inrush current)
  • Protecção contra back-feed

═══════════════════════════════════════════════════════════════

  Circuito de Aplicação (20V max):
  ────────────────────────────────

                        USB-C Connector
                       ┌─────────────────┐
        VBUS ──────────┤ VBUS            │
                       │                 │
        CC1 ───────────┤ CC1             │
        CC2 ───────────┤ CC2             │
                       │                 │
        D+ ────────────┤ D+              │──► CH340C
        D- ────────────┤ D-              │──► CH340C
                       │                 │
        GND ───────────┤ GND             │
                       └─────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
              ▼               ▼               ▼
         ┌─────────────────────────────────────────┐
         │                 IP2721                  │
         │                                         │
   VBUS ─┤ VBUS(16)                   CC1(12) ├───► CC1
         │                                         │
   VIN* ─┤ VIN(1)                     CC2(13) ├───► CC2
         │  * VIN liga ao Source do MOSFET (após switch)
         │                                         │
         │ VBUSG(4) ──────────────────────┐        │
         │                                │        │
  [100k] │ SEL(7) ───[100kΩ]─── VIN      │        │
         │             (20V max)          │        │
   VOUT ◄┤ VOUT(2)                       │        │
         │                                │        │
   GND ──┤ GND(5,6,14,15)                │        │
         │                                │        │
         └────────────────────────────────│────────┘
                                          │
                                          │ GATE
                                          ▼
                    VBUS ────────┬────────────────────────┐
                                 │                        │
                                 │   ┌─────────────────┐  │
                                 │   │    AO3400A      │  │
                                 │   │    N-MOSFET     │  │
                                 │   │                 │  │
                                 └───┤ D (Drain)       │  │
                                     │                 │  │
                    VBUSG ───────────┤ G (Gate)        │  │
                                     │                 │  │
                    VIN_BUCK ◄───────┤ S (Source)      │  │
                                     │                 │  │
                                     └─────────────────┘  │
                                          │               │
                                          │               │
                                          ▼               │
                                    Para TPS56838         │
                                    (VIN = até 20V)      │
                                                         │
                    GND ─────────────────────────────────┘

═══════════════════════════════════════════════════════════════

  Fallback Automático:
  ────────────────────

  O IP2721 negocia automaticamente a tensão máxima suportada
  por AMBOS (fonte PD e configuração SEL).

  Exemplo (SEL = 20V max):
  ┌─────────────────────────┬──────────────────┐
  │ Fonte PD suporta        │ IP2721 negocia   │
  ├─────────────────────────┼──────────────────┤
  │ 20V, 15V, 9V, 5V        │ 20V ✓            │
  │ 15V, 9V, 5V             │ 15V ✓ (fallback) │
  │ 9V, 5V                  │ 9V ✓ (fallback)  │
  │ 5V só                   │ 5V ✓ (fallback)  │
  └─────────────────────────┴──────────────────┘

  O firmware do ESP32 detecta a tensão real via ADC e ajusta
  o brilho máximo em conformidade.

═══════════════════════════════════════════════════════════════
```

### 3.2 U12 - TPS56838 (Buck Síncrono 8A, FCCM)

O TPS56838 (Texas Instruments, família TPS5683x) é um buck converter síncrono de alta eficiência
com MOSFETs integrados e controlo D-CAP3 (sem compensação externa).

**Porquê TPS56838 (substituiu SY8368AQQC):**
- SY8368 entrava em pulse-skipping (PFM) a light load → **coil whine audível**
- SY8368 não tem pin MODE para forçar PWM contínuo
- TPS56838 opera em **FCCM nativo** (Forced Continuous Conduction Mode)
- D-CAP3: sem rede de compensação externa (COMP pin não existe)
- 28V máximo (vs 18V do SY8368) → margem segura para 20V PD
- Package mais compacto: 10-pin 3×3mm HotRod QFN (vs QFN-20)

```
TPS56838 - Buck Converter 8A (FCCM):
═══════════════════════════════════════════════════════════════

  Características:
  ─────────────────
  • Tensão entrada: 4.5V - 28V (margem para 20V PD!)
  • Tensão saída: 0.6V - 13V
  • Corrente: 8A contínua
  • Frequência: 500kHz / 800kHz / 1200kHz (selecionável via MODE)
  • Controlo: D-CAP3 (sem compensação externa!)
  • Modo: FCCM nativo (sem coil whine a light load)
  • RDS(ON): 20.4mΩ (high-side) / 9.5mΩ (low-side)
  • VFB: 0.6V ±1% @ 25°C
  • Soft-start: 1.8ms default (ajustável via SS cap)
  • Proteções: OV, UV, OC, OT, UVLO (não-latched)
  • Package: VQFN-HR 10-pin (3×3mm) - HotRod QFN
  • LCSC: C37533416

  Pinout (VQFN-HR 10-pin, vista de cima):
  ────────────────────────────────────────

              ┌──────────────────────┐
              │   TPS56838 (10-pin)  │
              │    VQFN-HR 3×3mm     │
              │                      │
         EN ─┤1                  10├─ MODE
         SS ─┤2                   9├─ PGND
         FB ─┤3                   8├─ VIN
         PG ─┤4                   7├─ SW
       AGND ─┤5                   6├─ BOOT
              │                      │
              │   [THERMAL PAD]      │
              │      PGND            │
              └──────────────────────┘

  Descrição dos Pinos:
  ────────────────────
  │ Pin │ Nome │ Tipo │ Descrição                                    │
  │  1  │ EN   │  I   │ Enable. Float ou HIGH = ligado. Divisor      │
  │     │      │      │ resistivo para UVLO externo.                 │
  │  2  │ SS   │  I   │ Soft-start. Cap → AGND ou float (1.8ms).    │
  │  3  │ FB   │  I   │ Feedback. Divisor resistivo de VOUT.         │
  │  4  │ PG   │  O   │ Power-Good (open-drain). LOW se fora spec.  │
  │  5  │ AGND │  G   │ Ground analógico. Ligar a PGND num ponto.   │
  │  6  │ BOOT │  I   │ Bootstrap. 100nF entre BOOT e SW.           │
  │  7  │ SW   │  P   │ Switch node. Liga ao indutor.               │
  │  8  │ VIN  │  P   │ Entrada. Caps CIN entre VIN e PGND.         │
  │  9  │ PGND │  G   │ Power ground. Source do low-side MOSFET.     │
  │ 10  │ MODE │  I   │ Seleção frequência + current limit.         │
  │ PAD │ PGND │  G   │ Thermal pad. Vias térmicas para GND.        │

  Seleção MODE (resistor RMODE → AGND):
  ──────────────────────────────────────
  O pino MODE aceita 6 configurações via resistor para AGND.
  Combina 3 frequências × 2 limites de corrente.
  Consultar tabela no datasheet SLVSGM3B para valores RMODE.

  Recomendação: 500kHz com ILIM standard para 20V→5V @ 8A.

  Circuito de Aplicação (20V → 5V @ 8A):
  ──────────────────────────────────────

                     VIN = 5-20V (do IP2721 via AO3404A)
                          │
                          │
            ┌─────────────┼──────────────────────────────┐
            │             │                              │
           ═╧═           ═╧═          ═╧═                │
       C_IN1          C_IN2       C_HF                   │
      22µF/25V       22µF/25V    100nF/50V               │
       (1210)         (1210)      (0402)                 │
            │             │          │                   │
            └──────┬──────┴──────────┘                   │
                   │                                     │
                   │  ┌──────────────────────────────┐   │
                   │  │        TPS56838              │   │
                   │  │     VQFN-HR 10-pin           │   │
                   │  │                              │   │
                   ├──┤ VIN (8)                      │   │
                   │  │                              │   │
                   │  │ EN (1) ◄──[R_EN1]──┬         │   │
                   │  │          (float OK) │         │   │
                   │  │                    │         │   │
                   │  │ SS (2) ─[C_SS]──── AGND      │   │
                   │  │        (float=1.8ms default)  │   │
                   │  │                              │   │
                   │  │ MODE (10) ──[RMODE]── AGND   │   │
                   │  │  (seleciona freq + ILIM)     │   │
                   │  │                              │   │
                   │  │ FB (3) ◄──────────┤          │   │
                   │  │                   │          │   │
                   │  │ PG (4) ─○ (opcional ao ESP32)│   │
                   │  │                   │          │   │
                   │  │ BOOT (6)──[100nF]─┤          │   │
                   │  │                   │          │   │
                   │  │ SW (7) ───────────┤          │   │
                   │  │                   │          │   │
                   │  │ AGND (5) ─── GND  │          │   │
                   │  │ PGND (9,PAD) ─ GND│          │   │
                   │  │                   │          │   │
                   │  └───────────────────│──────────┘   │
                   │                      │              │
                   │                      │ SW           │
                   │              ┌───────┴───────┐      │
                   │              │               │      │
                   │             ═╪═             ═╧═     │
                   │          L1 2.2µH       C_BOOT      │
                   │         (shielded)     100nF        │
                   │              │               │      │
                   │              ├───────────────┘      │
                   │              │                      │
                   │              │ VOUT = 5V            │
                   │              │                      │
            ┌──────┴──────────────┼──────────────────┐   │
            │                     │                  │   │
           ═╧═                   ═╧═                ═╧═  │
       C_OUT1                C_OUT2              C_OUT3   │
      22µF/10V              22µF/10V            22µF/10V  │
       (1206)                (1206)              (1206)   │
            │                     │                  │   │
            └─────────────────────┴──────────────────┘   │
                                  │                      │
                                 ═╧═                     │
                            C_BULK                       │
                          470-1000µF                     │
                          10V Elect.                     │
                          (opcional)                     │
                                  │                      │
      GND ────────────────────────┴──────────────────────┘


  Rede de Feedback (5.0V):
  ────────────────────────

                          VOUT = 5V
                              │
                              │
                        ┌────[C_FF]────┐
                        │    22pF      │
                        │              │
                       [R_FB1] 22kΩ (1%)
                        │
                        ├────────────► FB (pino 3)
                        │
                       [R_FB2] 3kΩ (1%)
                        │
                       GND

  Cálculo: VOUT = 0.6V × (1 + R_FB1/R_FB2) = 0.6 × (1 + 22/3) = 5.0V ✓

  C_FF (feedforward): 22pF em paralelo com R_FB1
  → Zero a f = 1/(2π × 22kΩ × 22pF) ≈ 329kHz
  → Melhora estabilidade em transientes de carga
  → OBRIGATÓRIO para evitar instabilidade!

  ⚠️ SEM REDE DE COMPENSAÇÃO EXTERNA!
  ─────────────────────────────────────
  O TPS56838 usa controlo D-CAP3 que NÃO necessita de
  componentes COMP/R_C/C_C externos. Isto simplifica o
  design e elimina problemas de tuning de loop.

═══════════════════════════════════════════════════════════════
```

### 3.3 L1 - Indutor 2.2µH / 10A

```
Indutor para Buck Converter:
═══════════════════════════════════════════════════════════════

  Especificações:
  ───────────────
  • Valor: 2.2µH
  • Corrente saturação: ≥10A
  • Corrente RMS: ≥8A
  • DCR: <15mΩ (baixa perda)
  • Package: Shielded (baixo EMI)

  Componente Recomendado:
  ───────────────────────

  ┌─────────────────────────────────────────────────────────────┐
  │  CKST0603-2.2uH/M                                           │
  │                                                             │
  │  • Indutância: 2.2µH                                        │
  │  • Isat: 10.5A                                              │
  │  • Irms: 9.2A                                               │
  │  • DCR: 8.5mΩ                                               │
  │  • Package: 6.6×6.6×3mm (SMD)                               │
  │  • LCSC: C3002634 (Extended)                                │
  │                                                             │
  └─────────────────────────────────────────────────────────────┘

  Alternativas (Basic Stock):
  ───────────────────────────

  ┌──────────────────────┬────────┬───────┬───────┬─────────────────┐
  │ Part Number          │ Value  │ Isat  │ DCR   │ LCSC            │
  ├──────────────────────┼────────┼───────┼───────┼─────────────────┤
  │ **SRP1265A-2R2M** ★  │ 2.2µH  │ 22A   │ 4.2mΩ │ **C2831487**    │
  │ SWPA6045S2R2MT       │ 2.2µH  │ 8.5A  │ 16mΩ  │ C408335         │
  │ WPN5040S2R2MT        │ 2.2µH  │ 10A   │ 12mΩ  │ C495537         │
  └──────────────────────┴────────┴───────┴───────┴─────────────────┘

  ★ Usado no KiCad v3 - Bourns SRP1265A-2R2M (Extended, LCSC C2831487)

  Dimensionamento:
  ────────────────

  Frequência: f_sw = 500kHz
  Ripple desejado: ΔI = 30% × I_out = 0.3 × 8A = 2.4A

  L = (VIN - VOUT) × VOUT / (VIN × f_sw × ΔI)
  L = (15 - 5) × 5 / (15 × 500k × 2.4)
  L = 50 / 18M = 2.78µH

  → 2.2µH é adequado (ripple ~35%, dentro do aceitável)

═══════════════════════════════════════════════════════════════
```

### 3.4 Condensadores

```
Condensadores de Entrada e Saída:
═══════════════════════════════════════════════════════════════

  ENTRADA (perto do TPS56838):
  ──────────────────────────

  Requisito: Suportar ripple de corrente de entrada do buck
  Mínimo: 2× 22µF/25V MLCC (1210)
  Recomendado: 4× 22µF/25V para menor ESR

  ┌─────────────────────────────────────────────────────────────┐
  │  CL32A226KAJNNNE (Samsung)                                  │
  │                                                             │
  │  • Capacitância: 22µF                                       │
  │  • Tensão: 25V                                              │
  │  • Package: 1210                                            │
  │  • Dielétrico: X5R                                          │
  │  • LCSC: C52306 (Basic)                                     │
  │  • Preço: ~€0.08/un                                         │
  │                                                             │
  │  Quantidade: 2-4 unidades em paralelo                       │
  │                                                             │
  └─────────────────────────────────────────────────────────────┘


  SAÍDA (perto do TPS56838):
  ─────────────────────────

  Requisito: Baixo ESR para resposta transitória rápida
  Mínimo: 4× 22µF/10V MLCC (1206)
  Recomendado: 6-8× para melhor resposta a picos

  ┌─────────────────────────────────────────────────────────────┐
  │  CL31A226KAHL (Samsung)                                     │
  │                                                             │
  │  • Capacitância: 22µF                                       │
  │  • Tensão: 10V (ou 25V para margem extra)                   │
  │  • Package: 1206                                            │
  │  • Dielétrico: X5R                                          │
  │  • LCSC: C12891 (Basic)                                     │
  │  • Preço: ~€0.03/un                                         │
  │                                                             │
  │  Quantidade: 4-8 unidades em paralelo                       │
  │                                                             │
  └─────────────────────────────────────────────────────────────┘


  BULK (perto do HUB75 / terminais):
  ──────────────────────────────────

  Requisito: Absorver "coices" de corrente do painel HUB75
  Tipo: Electrolítico ou Polymer de baixo ESR

  ┌─────────────────────────────────────────────────────────────┐
  │  Electrolítico 470µF-1000µF / 10-16V                        │
  │                                                             │
  │  Opções:                                                    │
  │  • 470µF/16V: LCSC C3339 (Φ8×10.5mm)                       │
  │  • 1000µF/10V: LCSC C249287 (Φ10×12.5mm)                   │
  │  • 1000µF/16V: LCSC C249288 (Φ10×16mm)                     │
  │                                                             │
  │  ⚠️ IMPORTANTE:                                             │
  │  Deixar footprint mesmo que não monte de início.            │
  │  Painéis HUB75 causam picos de corrente que podem           │
  │  provocar instabilidade sem bulk capacitance.               │
  │                                                             │
  └─────────────────────────────────────────────────────────────┘


  Layout Recomendado:
  ───────────────────

                    VIN (15V)                    VOUT (5V)
                       │                            │
       ┌───────────────┼───────────────┐   ┌───────┼───────────────────────┐
       │               │               │   │       │                       │
      ═╧═             ═╧═             ═╧═ ═╧═     ═╧═                      │
   C_IN1           C_IN2           C_IN3 C_OUT1 C_OUT2                    │
   22µF            22µF            22µF  22µF   22µF                      │
       │               │               │   │       │                       │
       └───────────────┴───────────────┴───┴───────┤                       │
                       │                           │                       │
                       ▼                           ▼                       │
                 Perto do IC              Perto do IC                      │
                                                                           │
                                                              ┌────────────┤
                                                              │            │
                                                             ═╧═          ═╧═
                                                          C_BULK      C_TERM
                                                         470-1000µF   100µF
                                                         (opcional)   (no HUB75)
                                                              │            │
                                                              │            │
                                         Perto do terminal ◄──┴────────────┘
                                         de parafuso / HUB75

═══════════════════════════════════════════════════════════════
```

---

## 4. Circuito Completo

### 4.1 Esquema

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    PSU v3 - ESQUEMA COMPLETO                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                          USB-C Connector (J1)                               │
│                         ┌─────────────────────┐                             │
│     VBUS (A4,A9,B4,B9) ─┤ VBUS                │                             │
│                         │                     │                             │
│              GND ───────┤ GND                 │                             │
│                         │                     │                             │
│              D+ ────────┤ D+ (A6,B6)          │──► CH340C (dados USB)       │
│              D- ────────┤ D- (A7,B7)          │──► CH340C (dados USB)       │
│                         │                     │                             │
│             CC1 ────────┤ CC1 (A5)            │                             │
│             CC2 ────────┤ CC2 (B5)            │                             │
│                         └─────────────────────┘                             │
│                                │                                            │
│                    ┌───────────┼───────────┐                                │
│                    │           │           │                                │
│                    ▼           ▼           ▼                                │
│               ┌─────────────────────────────────┐                           │
│               │     IP2721 (U10) + AO3404A (Q3) │                           │
│               │          PD + Power Path        │                           │
│               │                                 │                           │
│     VBUS ─────┤ VBUS(16)              CC1(12)───┼─► CC1                     │
│               │                                 │                           │
│     VBUS ─────┤ VIN(1)               CC2(13) ───┼─► CC2                     │
│               │                                 │                           │
│               │ VBUSG(4)─────────┐   GND ───────┼─► GND                     │
│               │                  │              │                           │
│  [100kΩ]──VIN─┤ SEL(7)           │              │                           │
│               │                  │              │                           │
│               └──────────────────│──────────────┘                           │
│                                  │                                          │
│                                  │ GATE                                     │
│                                  ▼                                          │
│                    VBUS ────┬────────────────────────┐                      │
│                             │   ┌─────────────────┐  │                      │
│                             │   │    AO3400A      │  │                      │
│                             └───┤ D          S ├──┼──► VIN_BUCK             │
│                    VBUSG ───────┤ G              │  │                       │
│                                 └─────────────────┘  │                      │
│                                                      │                      │
│                         │ VIN = 5-20V (após MOSFET)                         │
│                         │                                                   │
│         ┌───────────────┼───────────────────────────────┐                   │
│         │               │                               │                   │
│        ═╧═             ═╧═                              │                   │
│    C_IN1 22µF      C_IN2 22µF                           │                   │
│    25V (1210)      25V (1210)                           │                   │
│         │               │                               │                   │
│         └───────┬───────┘                               │                   │
│                 │                                       │                   │
│                 │    ┌──────────────────────────────────┤                   │
│                 │    │                                  │                   │
│                 │    │  ┌─────────────────────────────┐ │                   │
│                 │    │  │      TPS56838 (U12)       │ │                   │
│                 │    │  │       Buck 8A               │ │                   │
│                 │    │  │                             │ │                   │
│                 ├────┼──┤ VIN (19,20)                 │ │                   │
│                 │    │  │                             │ │                   │
│                 │    │  │ EN (18) ◄──[10k]──┬         │ │                   │
│                 │    │  │                   │         │ │                   │
│                 │    │  │ SS (17) ──[10nF]──┴─ GND    │ │                   │
│                 │    │  │                             │ │                   │
│                 │    │  │ MODE (11) ────────── GND    │ │                   │
│                 │    │  │                             │ │                   │
│                 │    │  │ FB (14) ◄──┤                │ │                   │
│                 │    │  │            │                │ │                   │
│                 │    │  │ COMP (15)──┼─[Rede Comp]    │ │                   │
│                 │    │  │            │                │ │                   │
│                 │    │  │ PGOOD (16)─○ (ao ESP32)     │ │                   │
│                 │    │  │            │                │ │                   │
│                 │    │  │ BOOT (1)───┼──[100nF]──┐    │ │                   │
│                 │    │  │            │           │    │ │                   │
│                 │    │  │ SW (2,3,4)─┼───────────┤    │ │                   │
│                 │    │  │            │           │    │ │                   │
│                 │    │  │ PGND ──────┴───────── GND   │ │                   │
│                 │    │  │                             │ │                   │
│                 │    │  └─────────────────────────────┘ │                   │
│                 │    │               │                  │                   │
│                 │    │               │ SW               │                   │
│                 │    │               │                  │                   │
│                 │    │       ┌───────┴───────┐          │                   │
│                 │    │       │               │          │                   │
│                 │    │      ═╪═             ═╧═         │                   │
│                 │    │   L1 2.2µH       C_BOOT          │                   │
│                 │    │    10A           100nF           │                   │
│                 │    │       │               │          │                   │
│                 │    │       ├───────────────┘          │                   │
│                 │    │       │                          │                   │
│                 │    │       │ VOUT = 5V                │                   │
│                 │    │       │                          │                   │
│         ┌───────┼────┼───────┼──────────────────────────┘                   │
│         │       │    │       │                                              │
│         │       │    │       │                                              │
│         │      ═╧═  ═╧═     ═╧═                                             │
│         │   R1      R2    C_OUT (x4-8)                                      │
│         │  22k     3k      22µF/10V                                         │
│         │       │    │       │                                              │
│         │       └────┤       │                                              │
│         │            │       │                                              │
│         │         ───┴───    │                                              │
│         │           FB       │                                              │
│         │                    │                                              │
│         │                    │      +5V RAIL                                │
│         │                    ├──────────────────────────────────────────►   │
│         │                    │                                              │
│         │           ┌────────┼────────────────┬───────────────┐             │
│         │           │        │                │               │             │
│         │          ═╧═      ═╧═              ═╧═              │             │
│         │      C_BULK    C_TERM1         C_TERM2              │             │
│         │     470-1000µF  100µF           22µF                │             │
│         │      (opt)      (opt)          (1206)               │             │
│         │           │        │                │               │             │
│         │           │        │                │               │             │
│         └───────────┴────────┴────────────────┴───────────────┴─► GND       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Divisor de Tensão para Sensing (ADC)

```
Medição de Tensão de Entrada (para detecção de perfil):
═══════════════════════════════════════════════════════════════

  O ESP32 mede a tensão de entrada para detectar o perfil PD
  negociado e ajustar o brilho máximo automaticamente.

  VIN após MOSFET (5V-20V)
          │
          │
         [R3]  100kΩ (1%)
          │
          ├────────────────► GPIO36 (ADC1_CH0) do ESP32
          │
         [R4]  22kΩ (1%)
          │
          │
         ═╧═  C_FILTER
        100nF
          │
          │
         GND

  Divisor: 22k / (100k + 22k) = 0.18

  ┌──────────────────────────────────────────────────────────┐
  │ Tensão PD │ Após Divisor │ ADC (12-bit) │ Perfil         │
  ├───────────┼──────────────┼──────────────┼────────────────┤
  │ 5V        │ 0.90V        │ ~1117        │ USB_5V_3A      │
  │ 9V        │ 1.62V        │ ~2011        │ PD_9V (27W)    │
  │ 12V       │ 2.16V        │ ~2681        │ PD_12V (36W)   │
  │ 15V       │ 2.70V        │ ~3351        │ PD_15V (45W)   │
  │ 20V       │ 3.60V        │ ~4095 (sat)  │ PD_20V (60W+)  │
  └──────────────────────────────────────────────────────────┘

  ⚠️ Nota: Para 20V, o divisor satura o ADC.
  Pode usar 100k/15k (0.13) para range completo, ou
  aceitar que 20V = "saturado = potência máxima".

═══════════════════════════════════════════════════════════════
```

---

## 5. Gestão de Brilho por Software

### 5.1 Perfis de Potência

```
Mapeamento Potência → Brilho Máximo:
═══════════════════════════════════════════════════════════════

  ┌─────────────────┬────────────┬──────────────┬──────────────┐
  │ Perfil          │ Potência   │ Corrente 5V  │ Brilho Max   │
  │                 │ Disponível │ Disponível   │ (Software)   │
  ├─────────────────┼────────────┼──────────────┼──────────────┤
  │ PD_20V_3A       │ 60W        │ 10A+ (cap 8A)│ 100%         │
  │ PD_15V_3A       │ 45W        │ 8A           │ 100%         │
  │ PD_15V_2A       │ 30W        │ 5.6A         │ 100%         │
  │ PD_12V_3A       │ 36W        │ 6.5A         │ 100%         │
  │ PD_9V_3A        │ 27W        │ 4.8A         │ 80%  ⚠️      │
  │ USB_5V_3A       │ 15W        │ 2.8A (bypass)│ 40%  ⚠️      │
  │ USB_5V_1.5A     │ 7.5W       │ 1.4A         │ 20%  ⚠️      │
  └─────────────────┴────────────┴──────────────┴──────────────┘

  ⚠️ = Brilho capado em software para evitar overcurrent

═══════════════════════════════════════════════════════════════
```

### 5.2 Código Firmware

```cpp
// firmware/power_manager_v3.h
// ═══════════════════════════════════════════════════════════════

#ifndef POWER_MANAGER_V3_H
#define POWER_MANAGER_V3_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Pino ADC para medir tensão de entrada (após divisor 100k/22k)
#define VSENSE_PIN 36  // GPIO36 (ADC1_CH0)

// Divisor de tensão: 22k / (100k + 22k) = 0.18
#define VOLTAGE_DIVIDER_RATIO 0.18f

// Thresholds de tensão (com histerese)
#define THRESH_20V  17.0f   // >17V = 20V PD
#define THRESH_15V  13.0f   // >13V = 15V PD
#define THRESH_12V  10.5f   // >10.5V = 12V PD
#define THRESH_9V    7.5f   // >7.5V = 9V PD
#define THRESH_5V    4.0f   // >4V = 5V USB

enum PowerProfile {
    PROFILE_UNKNOWN = 0,
    PROFILE_USB_5V,      // 5V USB direto (bypass buck)
    PROFILE_PD_9V,       // 9V @ 3A = 27W
    PROFILE_PD_12V,      // 12V @ 3A = 36W
    PROFILE_PD_15V,      // 15V @ 2-3A = 30-45W (preferido)
    PROFILE_PD_20V       // 20V @ 3A = 60W
};

struct PowerConfig {
    PowerProfile profile;
    float inputVoltage;
    float maxBrightness;    // 0.0 - 1.0
    uint16_t maxCurrentMA;  // mA disponíveis no rail 5V
    bool buckActive;        // true se buck está em uso
};

class PowerManagerV3 {
private:
    MatrixPanel_I2S_DMA* display;
    PowerConfig config;

    // Calibração ADC (ajustar em produção)
    float adcCalibration = 1.0f;

public:
    PowerManagerV3(MatrixPanel_I2S_DMA* dma_display) {
        display = dma_display;
        config.profile = PROFILE_UNKNOWN;
        config.inputVoltage = 0;
        config.maxBrightness = 0.25f;  // Conservador até detectar
        config.maxCurrentMA = 1500;
        config.buckActive = false;
    }

    // Medir tensão de entrada via ADC
    float measureInputVoltage() {
        // Média de múltiplas leituras para estabilidade
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) {
            sum += analogRead(VSENSE_PIN);
            delayMicroseconds(100);
        }
        float adcValue = sum / 16.0f;

        // Converter para tensão real
        // ADC 12-bit: 0-4095 = 0-3.3V (com atenuação default)
        float adcVoltage = (adcValue / 4095.0f) * 3.3f;
        config.inputVoltage = (adcVoltage / VOLTAGE_DIVIDER_RATIO) * adcCalibration;

        return config.inputVoltage;
    }

    // Detectar perfil de potência
    PowerProfile detectProfile() {
        float vin = measureInputVoltage();

        PowerProfile newProfile;

        if (vin >= THRESH_20V) {
            newProfile = PROFILE_PD_20V;
            config.maxBrightness = 1.0f;
            config.maxCurrentMA = 8000;
            config.buckActive = true;
        }
        else if (vin >= THRESH_15V) {
            newProfile = PROFILE_PD_15V;
            config.maxBrightness = 1.0f;
            config.maxCurrentMA = 8000;
            config.buckActive = true;
        }
        else if (vin >= THRESH_12V) {
            newProfile = PROFILE_PD_12V;
            config.maxBrightness = 1.0f;
            config.maxCurrentMA = 6500;
            config.buckActive = true;
        }
        else if (vin >= THRESH_9V) {
            newProfile = PROFILE_PD_9V;
            config.maxBrightness = 0.80f;  // ⚠️ Capado a 80%
            config.maxCurrentMA = 4800;
            config.buckActive = true;
        }
        else if (vin >= THRESH_5V) {
            newProfile = PROFILE_USB_5V;
            config.maxBrightness = 0.40f;  // ⚠️ Capado a 40%
            config.maxCurrentMA = 2800;
            config.buckActive = false;  // Bypass direto
        }
        else {
            newProfile = PROFILE_UNKNOWN;
            config.maxBrightness = 0.20f;  // Muito conservador
            config.maxCurrentMA = 1000;
            config.buckActive = false;
        }

        // Log mudança de perfil
        if (newProfile != config.profile) {
            config.profile = newProfile;
            logProfileChange();
        }

        return config.profile;
    }

    // Aplicar brilho máximo ao display
    void applyBrightness(float requestedBrightness = 1.0f) {
        // Limitar ao máximo permitido pelo perfil
        float actualBrightness = min(requestedBrightness, config.maxBrightness);
        uint8_t brightness8 = (uint8_t)(actualBrightness * 255);

        display->setBrightness8(brightness8);

        Serial.printf("[PWR] Brilho: %.0f%% (pedido: %.0f%%, max: %.0f%%)\n",
                      actualBrightness * 100,
                      requestedBrightness * 100,
                      config.maxBrightness * 100);
    }

    // Brilho adaptativo baseado no conteúdo
    void adjustForContent(uint16_t activePixels, uint16_t totalPixels) {
        float fillRatio = (float)activePixels / totalPixels;

        // Quanto mais pixels activos, mais conservador
        float contentFactor = 1.0f;
        if (fillRatio > 0.7f) contentFactor = 0.7f;
        else if (fillRatio > 0.5f) contentFactor = 0.85f;
        else if (fillRatio > 0.3f) contentFactor = 0.95f;

        float effectiveBrightness = config.maxBrightness * contentFactor;
        uint8_t brightness8 = (uint8_t)(effectiveBrightness * 255);

        display->setBrightness8(brightness8);
    }

    // Getters
    PowerConfig getConfig() const { return config; }
    float getMaxBrightness() const { return config.maxBrightness; }
    float getInputVoltage() const { return config.inputVoltage; }
    bool isBuckActive() const { return config.buckActive; }

    // Calibração (chamar com tensão conhecida)
    void calibrate(float knownVoltage) {
        float measured = measureInputVoltage();
        if (measured > 0) {
            adcCalibration = knownVoltage / (measured / adcCalibration);
            Serial.printf("[PWR] Calibração: factor = %.3f\n", adcCalibration);
        }
    }

private:
    void logProfileChange() {
        const char* profileNames[] = {
            "UNKNOWN", "USB_5V", "PD_9V", "PD_12V", "PD_15V", "PD_20V"
        };

        Serial.println("╔══════════════════════════════════════════╗");
        Serial.printf( "║  Power Profile: %-24s  ║\n", profileNames[config.profile]);
        Serial.printf( "║  Input Voltage: %.1fV                     ║\n", config.inputVoltage);
        Serial.printf( "║  Max Current:   %dmA                   ║\n", config.maxCurrentMA);
        Serial.printf( "║  Max Brightness: %.0f%%                     ║\n", config.maxBrightness * 100);
        Serial.printf( "║  Buck Active:   %s                      ║\n", config.buckActive ? "Yes" : "No ");
        Serial.println("╚══════════════════════════════════════════╝");
    }
};

#endif // POWER_MANAGER_V3_H
```

### 5.3 Exemplo de Uso

```cpp
// main.cpp - Exemplo de integração
// ═══════════════════════════════════════════════════════════════

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "power_manager_v3.h"

MatrixPanel_I2S_DMA* display = nullptr;
PowerManagerV3* powerMgr = nullptr;

void setup() {
    Serial.begin(115200);

    // Inicializar display
    HUB75_I2S_CFG mxconfig(32, 16, 1);
    display = new MatrixPanel_I2S_DMA(mxconfig);
    display->begin();

    // Inicializar gestor de energia
    powerMgr = new PowerManagerV3(display);

    // Esperar estabilização da fonte (soft-start do buck)
    delay(100);

    // Detectar perfil de potência
    powerMgr->detectProfile();

    // Aplicar brilho máximo permitido
    powerMgr->applyBrightness(1.0f);  // Pede 100%, recebe o permitido
}

void loop() {
    // Re-verificar perfil periodicamente (hot-plug de fonte diferente)
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 5000) {  // A cada 5 segundos
        powerMgr->detectProfile();
        powerMgr->applyBrightness(1.0f);
        lastCheck = millis();
    }

    // ... resto do código do relógio ...

    // Exemplo: ajustar brilho baseado no conteúdo
    // uint16_t activePixels = countActivePixels();  // Implementar
    // powerMgr->adjustForContent(activePixels, 32 * 16);
}
```

---

## 6. Bill of Materials (BOM)

### 6.1 Componentes Principais PSU

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC | Stock |
|-----|------------|---------------|---------|-----|-------|------|-------|
| U10 | IP2721 | USB-C PD Trigger | TSSOP-16 | 1 | €0.40 | C603176 | Extended |
| Q3 | AO3400A / AO3404A | N-MOSFET 30V ~5A | SOT-23 | 1 | €0.03 | C20917 | Basic |
| U12 | **TPS56838** (TI) | Buck Sync 8A FCCM D-CAP3 | VQFN-HR 10-pin 3x3 | 1 | ~€1.00 | **C37533416** | Extended |
| L1 | **Bourns SRP1265A-2R2M** | Indutor 2.2µH 22A | 12.5x12.5x6.5mm | 1 | €0.30 | **C2831487** | Extended |
| | *Alt: CKST0603-2.2uH/M* | *2.2µH 10A* | *6.6x6.6x3mm* | 1 | €0.25 | C3002634 | Extended |
| C_BOOT1 | 100nF 50V | MLCC X7R | 0402 | 1 | €0.01 | C307331 | Basic |
| C14,C15 | CL32A226KAJNNNE | MLCC 22µF 25V | 1210 | 2 | €0.08 | C52306 | Basic |
| C_OUT1-4 | CL31A226KAHL | MLCC 22µF 25V | 1206 | 4 | €0.03 | C52306 | Basic |
| C_BULK | 470µF 16V | Electrolítico | Φ8x10mm | 1 | €0.08 | C3339 | Basic |
| C17 | 1µF 50V | VCC Buck bypass | 0603 | 1 | €0.01 | C15849 | Basic |
| R_FB1 | 22kΩ 1% | Resistor (FB upper) | 0603 | 1 | €0.01 | C31850 | Basic |
| R_FB2 | 3kΩ 1% | Resistor (FB lower) | 0603 | 1 | €0.01 | C4211 | Basic |
| R9 | 100kΩ 1% | SEL→VIN (20V max) | 0402 | 1 | €0.01 | C25741 | Basic |
| R3,R4 | 5.1kΩ | CC1/CC2 resistors | 0402 | 2 | €0.01 | C25905 | Basic |
| **Total PSU** | | | | | **~€2.30** | | |

### 6.2 Componentes Sensing (ADC)

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC |
|-----|------------|---------------|---------|-----|-------|------|
| R_SENSE1 | 100kΩ 1% | Divisor tensão | 0402 | 1 | €0.01 | C25741 |
| R_SENSE2 | 22kΩ 1% | Divisor tensão | 0402 | 1 | €0.01 | C25768 |
| C_FILTER | 100nF 16V | Filtro ADC | 0402 | 1 | €0.01 | C307331 |
| **Total Sensing** | | | | | **€0.03** | |

### 6.3 Proteções e Conectores

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC | Stock |
|-----|------------|---------------|---------|-----|-------|------|-------|
| J1 | USB-C Receptacle | GCT USB4105-GF-A 16-pin | SMD | 1 | €0.40 | C3020560 | Extended |
| F1 | PTC Fuse | ASMD2920-300 3A/30V | 2920 | 1 | €0.05 | C2982291 | Extended |
| D3 | H7VN10B | TVS USB VBUS | DFN1006-2L | 1 | €0.05 | C20615788 | Extended |
| D4 | SMBJ5.0A | TVS 5V output | SMB | 1 | €0.08 | C113620 | Basic |
| J3 | Barrier Terminal | KF301-5.0-2P 10A | THT | 1 | €0.10 | C474881 | Extended |
| J6 | Pin Header 1×3 | J_MODE selector | 2.54mm | 1 | €0.02 | C2337 | Basic |
| **Total Proteções** | | | | | **€0.70** | | |

### 6.4 Regulador 3.3V e Periféricos

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC |
|-----|------------|---------------|---------|-----|-------|------|
| U_REG | AP2112K-3.3 | LDO 3.3V 600mA | SOT-23-5 | 1 | €0.15 | C51118 |
| C_LDO_IN | 10µF 10V | MLCC X5R | 0603 | 1 | €0.02 | C19702 |
| C_LDO_OUT | 10µF 10V | MLCC X5R | 0603 | 1 | €0.02 | C19702 |
| LED1 | LED Verde | Power 5V | 0805 | 1 | €0.02 | C2297 |
| LED2 | LED Verde | Power 3.3V | 0805 | 1 | €0.02 | C2297 |
| R_LED1 | 1kΩ | LED 5V | 0402 | 1 | €0.01 | C11702 |
| R_LED2 | 1kΩ | LED 3.3V | 0402 | 1 | €0.01 | C11702 |
| **Total 3.3V** | | | | | **€0.25** | |

### 6.5 Resumo Total

| Secção | Custo |
|--------|-------|
| PSU (U10 IP2721 + Q3 MOSFET + U12 TPS56838 + passivos) | €2.30 |
| Sensing (divisor ADC) | €0.03 |
| Proteções e Conectores (J1, F1, D3, D4, J3, J6) | €0.70 |
| Regulador 3.3V | €0.25 |
| **TOTAL PSU v3** | **~€3.30** |

> **Nota:**
> - U10 (IP2721) + Q3 (MOSFET): Power-path controlado, soft-start
> - U12 (TPS56838): Buck de 8A (vs 3-5A anterior)
> - Melhor compatibilidade de fontes (fallback automático)
> - Gestão automática de potência
> - Componentes Extended: U10, L1, J1, F1, D3, J3

---

## 7. Layout PCB

### 7.1 Considerações Críticas

```
Layout Buck Converter - Boas Práticas (TPS56838):
═══════════════════════════════════════════════════════════════

  1. LOOP DE ALTA CORRENTE (minimizar área)
  ─────────────────────────────────────────

     VIN ──┬──[CIN]──┬──[TPS56838 VIN/SW]──[L1]──┬──[COUT]──┬── VOUT
           │         │                         │          │
          GND       GND                       GND        GND
           │         │                         │          │
           └─────────┴─────────────────────────┴──────────┘
                         ▲
                         │
                   MANTER ESTE LOOP
                   O MAIS CURTO POSSÍVEL!

     Loops críticos (mínima área):
     • CIN → VIN → PGND → CIN (loop de entrada)
     • CBOOT → BOOT → SW → CBOOT (bootstrap)
     • L1 → COUT → GND → PGND (loop de saída)

  2. PLACEMENT
  ────────────

     ┌─────────────────────────────────────────────────────────┐
     │                                                         │
     │   [USB-C]                                               │
     │      │                                                  │
     │      ▼                                                  │
     │   [IP2721]  ◄── Perto do conector USB-C                │
     │      │                                                  │
     │      │  VOUT (20V)                                      │
     │      ▼                                                  │
     │   [CIN + C_HF] ── Imediatamente ao lado do TPS56838   │
     │      │                                                  │
     │   [TPS56838] ◄── Centro da área de potência            │
     │      │                                                  │
     │   [L1] ──────── Adjacente ao SW pin (≤2mm)             │
     │      │                                                  │
     │   [COUT x3-4] ─ Distribuídos após L1                   │
     │      │                                                  │
     │      │  5V                                              │
     │      ▼                                                  │
     │   [C_BULK] ──── Perto dos terminais de saída           │
     │      │                                                  │
     │   [TERMINALS]                                           │
     │                                                         │
     └─────────────────────────────────────────────────────────┘

  3. PLANO DE GROUND
  ──────────────────

  • Usar plano de ground contínuo na layer inferior
  • Vias múltiplas nos pads PGND do TPS56838
  • AGND (pin 5) ligar a PGND num único ponto
  • Separar ground analógico (ADC) do ground de potência
    com ferrite bead ou ligação em ponto único

  4. THERMAL
  ──────────

  • Thermal pad (PGND) ligado ao ground com vias térmicas
  • Mínimo 4-6 vias (0.3mm) no thermal pad com espaçamento 1mm
  • Copper pour na top layer e bottom layer para dissipação
  • NÃO usar thermal relief nas vias do pad (conexão completa)

═══════════════════════════════════════════════════════════════
```

### 7.2 Lições Aprendidas - Ruído e Coil Whine (PCB v3.0)

```
LIÇÕES PCB v3.0 - PROBLEMAS IDENTIFICADOS E CORREÇÕES:
═══════════════════════════════════════════════════════════════

  ⚠️ PROBLEMA 1: COIL WHINE (RUÍDO AUDÍVEL)
  ───────────────────────────────────────────
  Causa: SY8368 entrava em pulse-skipping (PFM) a light load.
  Frequência de switching caia para range audível (<20kHz).
  Manifesta-se como zumbido repetitivo, intensifica com LED blink.

  Solução: TPS56838 com FCCM nativo (frequência constante
  independentemente da carga). Elimina modos PFM/pulse-skip.

  ⚠️ PROBLEMA 2: GROUND POUR + VIAS SOB O INDUTOR L1
  ────────────────────────────────────────────────────
  Causa: Copper pour GND com vias sob o indutor L1 gera:
  • Correntes de eddy no cobre (perdas, aquecimento)
  • Acoplamento magnético entre indutor e plano de ground
  • Contribuição para vibração e ruído

  Solução: ZONE KEEPOUT sob o indutor L1
  • Sem cobre (top NEM bottom layer) sob a área do indutor
  • Sem vias na zona de keepout
  • Mínimo: área do footprint do indutor + 1mm margem
  • Aplicar em AMBAS as layers (F.Cu e B.Cu)

  ⚠️ PROBLEMA 3: ÁREA DE COBRE LX/SW DEMASIADO GRANDE
  ────────────────────────────────────────────────────
  Causa: Copper pour ou traces excessivos no nó SW (LX)
  funcionam como antena EMI (alta dV/dt no switch node).

  Solução: MINIMIZAR cobre no nó SW
  • Trace curto e direto do pin SW ao pad do indutor
  • NÃO incluir SW em copper flooding/pour
  • SW NÃO deve ser usado para dissipação térmica
  • Usar VIN, VOUT e GND pours para thermal (sinais DC quietos)

  ⚠️ PROBLEMA 4: C_FF (FEEDFORWARD) AUSENTE
  ──────────────────────────────────────────
  Causa: Condensador feedforward 22pF não incluído na v3.0.
  Resulta em resposta lenta a transientes e possível instabilidade.

  Solução: SEMPRE incluir C_FF = 22pF em paralelo com R_FB1
  • Zero a ~329kHz (1/(2π×22kΩ×22pF))
  • Colocar fisicamente adjacente a R_FB1
  • LCSC: C1555 (Basic, 0402)

  ⚠️ PROBLEMA 5: CONDENSADOR EN LONGE DO ESP32
  ─────────────────────────────────────────────
  Causa: C6 (100nF) no pin EN do ESP32 colocado perto do
  botão RESET, longe do pin EN. Auto-reset instável.

  Solução: C6 deve estar a ≤3mm do pin EN do ESP32.
  R5 (10k pull-up) pode ficar perto do botão.
  Considerar 1-10µF para EN (100nF pode ser insuficiente).

═══════════════════════════════════════════════════════════════
```

### 7.2 Footprint Indutor

```
Opções de Footprint para L1:
═══════════════════════════════════════════════════════════════

  Opção A: CKST0603 (6.6×6.6×3mm) - Compacto
  ──────────────────────────────────────────

       ┌───────────────┐
       │               │
       │   ┌───────┐   │
       │   │       │   │   6.6mm
       │   │  L1   │   │
       │   │       │   │
       │   └───────┘   │
       │               │
       └───────────────┘
           6.6mm

  Opção B: SRP1265 (12.5×12.5×6.5mm) - Alta corrente
  ─────────────────────────────────────────────────

       ┌───────────────────────┐
       │                       │
       │   ┌───────────────┐   │
       │   │               │   │   12.5mm
       │   │      L1       │   │
       │   │               │   │
       │   └───────────────┘   │
       │                       │
       └───────────────────────┘
              12.5mm

  💡 Recomendação: Desenhar footprint para o maior (SRP1265)
     que também aceita o menor (CKST0603 com ajuste de pads)

═══════════════════════════════════════════════════════════════
```

---

## 8. Testes e Validação

### 8.1 Checklist de Testes

```
Testes de Validação PSU v3:
═══════════════════════════════════════════════════════════════

  □ CONTINUIDADE (antes de ligar)
    ├─ VIN não curto com GND
    ├─ VOUT não curto com GND
    └─ Todos os componentes no lugar

  □ POWER-ON (com fonte PD 15V)
    ├─ IP2721 negocia 15V (medir VOUT)
    ├─ TPS56838 arranca (LED PWR acende)
    ├─ VOUT = 5.0V ±2% (medir com multímetro)
    └─ Sem oscilação audível (coil whine)

  □ EFICIÊNCIA
    ├─ Medir VIN, IIN
    ├─ Medir VOUT, IOUT
    ├─ Calcular: η = (VOUT × IOUT) / (VIN × IIN)
    └─ Deve ser >90% @ 3A, >88% @ 6A

  □ LOAD REGULATION
    ├─ VOUT @ 0A carga
    ├─ VOUT @ 3A carga
    ├─ VOUT @ 6A carga
    └─ Variação <2% (5.0V → 4.9V aceitável)

  □ TRANSIENT RESPONSE
    ├─ Aplicar carga de 0A→4A instantâneo
    ├─ Medir undershoot (deve ser <300mV)
    ├─ Remover carga 4A→0A
    └─ Medir overshoot (deve ser <300mV)

  □ FALLBACK 9V
    ├─ Usar fonte PD que só tem 9V
    ├─ IP2721 negocia 9V
    ├─ TPS56838 produz 5V
    └─ Firmware detecta e capa brilho a 80%

  □ FALLBACK 5V
    ├─ Usar carregador básico 5V/3A
    ├─ IP2721 fica em 5V (via resistências 5.1kΩ)
    ├─ Buck em bypass (ou tensão mínima de saída)
    └─ Firmware detecta e capa brilho a 40%

  □ THERMAL
    ├─ Operação contínua 30 min @ 5A
    ├─ Temperatura TPS56838 <85°C
    ├─ Temperatura L1 <100°C
    └─ Sem degradação de performance

═══════════════════════════════════════════════════════════════
```

### 8.2 Equipamento Necessário

| Equipamento | Uso | Alternativa DIY |
|-------------|-----|-----------------|
| Multímetro digital | Medir tensões | - |
| Fonte PD 45W+ | Fornecer entrada | Carregador laptop USB-C |
| Carga electrónica | Teste de corrente | Resistores de potência |
| Osciloscópio | Ver ripple/transients | - (opcional) |
| Termómetro IR | Medir temperaturas | Termopar + multímetro |

---

## 9. Comparação v2 → v3

```
Evolução da PSU:
═══════════════════════════════════════════════════════════════

  ┌────────────────────────┬─────────────────┬─────────────────┐
  │ Característica         │ v2              │ v3              │
  ├────────────────────────┼─────────────────┼─────────────────┤
  │ PD Controller          │ IP2721 / CYPD   │ IP2721+AO3400A  │
  │ Configuração           │ Complexa        │ Simples (1 res) │
  │                        │                 │                 │
  │ Buck Converter         │ MP1584 (3A)     │ TPS56838 (8A)   │
  │                        │ TPS54531 (5A)   │ FCCM, D-CAP3   │
  │                        │                 │                 │
  │ Corrente saída         │ 3-5A            │ 8A              │
  │                        │                 │                 │
  │ Tensão preferida       │ 12V ou 20V      │ 15V             │
  │                        │                 │                 │
  │ Fallback               │ Manual          │ Automático      │
  │                        │                 │                 │
  │ Gestão brilho          │ Manual/fixo     │ Automático ADC  │
  │                        │                 │                 │
  │ Custo                  │ ~€3.25          │ ~€4.25          │
  │                        │                 │                 │
  │ Compatibilidade        │ ~70% fontes     │ ~90% fontes     │
  │                        │                 │                 │
  │ Margem picos HUB75     │ Limitada        │ Excelente       │
  └────────────────────────┴─────────────────┴─────────────────┘

═══════════════════════════════════════════════════════════════
```

---

## 10. Referências

### Datasheets
- [IP2721 Datasheet](https://datasheet.lcsc.com/lcsc/2006111335_INJOINIC-IP2721_C603176.pdf) - INJOINIC
- [TPS56838 Datasheet (SLVSGM3B)](https://www.ti.com/lit/gpn/TPS56837) - Texas Instruments
- [TPS5683x Datasheet PDF](https://www.mouser.com/datasheet/2/405/1/tps56838-3395403.pdf) - Mouser mirror
- [Indutor CKST Series](https://www.coilcraft.com) - Coilcraft

### Application Notes
- [IP2721 USB-C PD Application](https://aichiplink.com/blog/IP2721-Datasheet-Specifications-Schematic-Features-Alternative_389)
- [High Current Buck Design](https://www.ti.com/lit/an/slva477b/slva477b.pdf) - TI
- [AN-1229 SIMPLE SWITCHER PCB Layout](https://www.ti.com/lit/pdf/snva054) - TI
- [QFN Thermal Pad Layout](https://www.ti.com/lit/an/sloa122/sloa122.pdf) - TI

### Projectos de Referência
- [Hackaday TS100 USB-C](https://cdn.hackaday.io/files/1721877366848608/V1_2%20Schematic_TS100%20-%20USB%20C%20IP2721.pdf)

### Notas de Substituição (SY8368 → TPS56838)
- SY8368AQQC (Silergy, C207642) descontinuado neste design por coil whine em PFM
- TPS56838 (TI, C37533416) escolhido: FCCM nativo, D-CAP3, 28V max, sem COMP externo
- Package mudou de QFN-20 para VQFN-HR 10-pin (mesmo 3×3mm)

### Investigação: Alternativa Silergy Drop-In com Forced PWM (Fev 2026)

**Objectivo**: Verificar se existe um chip Silergy pin-compatible com o SY8368AQQC (QFN3x3-12)
que suporte forced PWM (FCCM) para eliminar coil whine a light load.

**Análise do SY8368AQQC (QFN3x3-12, pinout confirmado no KiCad)**:
- Pinos 4, 5, 6: IN (VIN) — 3 pinos de entrada, todos obrigatórios (bond wires paralelos)
- Pinos 1, 3: GND
- Pin 2: LX (switch node)
- Pin 7: BS (bootstrap)
- Pin 8: VCC (LDO interno 3.3V)
- Pin 9: FB (feedback)
- Pin 10: ILMT (current limit: 8A/12A/16A)
- Pin 11: PG (power good)
- Pin 12: EN (enable)
- **Não tem pin MODE** — usa arquitectura "Instant PWM" (COT) que entra automaticamente
  em pulse-skipping/PFM a light load. Não há forma de forçar CCM contínuo.

**Candidatos Silergy investigados**:

| Chip | Package | VIN | IOUT | MODE/FCCM | Drop-in? | LCSC |
|------|---------|-----|------|-----------|----------|------|
| SY8368QNC | QFN3x3-10 | 4-28V | 8A | Não | Não (10 pinos, sem ILMT/PG) | C125897 |
| SY21228AQQC | QFN3x3-12 | 4-28V | 8A | A confirmar | Provável (mesmo package) | Não encontrado |
| SY21228LQQC | QFN3x3-12 | 4.5-28V | 8A | A confirmar | Provável (mesmo package) | Não encontrado |
| SY8388ARHC | QFN2.5x2.5-16 | 4-24V | 8A | **Sim (MODE pin)** | Não (package diferente) | C5110279 |
| SY21243ARHC | QFN2.5x2.5-16 | 4-24V | 8A | **Sim (MODE pin)** | Não (package diferente) | — |
| SY8386RHC | QFN2.5x2.5-16 | 4-28V | 6A | Não claro | Não (package diferente, 6A) | — |

**Conclusão**: Não existe alternativa Silergy drop-in confirmada com FCCM no mesmo QFN3x3-12.
- Os chips Silergy com MODE pin (SY8388A, SY21243A) usam packages QFN2.5x2.5-16 — incompatíveis.
- O SY21228 (A/L) é o sucessor do SY8368 no mesmo QFN3x3-12, mas: (a) não está no LCSC/JLCPCB,
  (b) o datasheet não foi acessível para confirmar se tem FCCM.
- A decisão de migrar para **TPS56838 (TI)** mantém-se como a correcta: FCCM nativo, disponível
  no JLCPCB (C37533416), e elimina o problema de coil whine sem depender de pin MODE.

**Nota sobre os 3 pinos VIN do SY8368 (layout legacy)**:
- Os pinos IN (4, 5, 6) são bond wires paralelos ao mesmo die pad interno.
- Todos os 3 DEVEM ser ligados ao VBUS — não é válido ligar apenas 1.
- Razões: distribuição de corrente nos bond wires, impedância parasita, e dissipação térmica.
- Para 2-layer board com GND pour: usar ilha de copper pour local no top layer que una os
  3 pads, com trace larga única desde Q1:Source. Vias de 20V desnecessárias se routing no top.

---

*Documento criado: Janeiro 2025*
*Versão: 3.2 - USB-C PD 30W com TPS56838 FCCM Buck 8A*
*Baseado em: POWER_SUPPLY_v2.md*
