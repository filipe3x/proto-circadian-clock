# Power Supply v3 - USB-C PD 30W com Buck de Alta Corrente

Este documento descreve a arquitetura de alimentação v3 baseada em USB-C Power Delivery, otimizada para 30W máximo com buck de alta corrente (8A).

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
│   • IP2721 ou CYPD3177                 • CH224K (mais simples, económico)   │
│     (configuração complexa)              (straps por resistor)              │
│                                                                             │
│   • MP1584EN (3A) ou TPS54531 (5A)     • SY8368AQQC (8A síncrono)          │
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
│   │            ├──────┤─►│ USB-C  │─►│ CH224K │─►│SY8368A │─►│TERMINAL│ │ │
│   │  9V/15V/   │      │  │ Recept.│  │  PD    │  │ BUCK   │  │PARAFUSO│ │ │
│   │  20V       │      │  │        │  │TRIGGER │  │ 8A     │  │        │ │ │
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

  USB-C         CH224K          SY8368A          Cargas
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

### 3.1 U1 - CH224K (PD Trigger)

O CH224K é um controlador USB-C PD sink que negocia tensão fixa baseado em resistores de configuração.

```
CH224K - USB-C PD Sink Controller:
═══════════════════════════════════════════════════════════════

  Características:
  ─────────────────
  • Suporta USB PD 2.0/3.0
  • Tensões: 5V, 9V, 12V, 15V, 20V
  • Corrente máxima: 5A
  • Configuração por resistores (sem firmware)
  • Package: ESSOP-10
  • LCSC: C970725

  Pinout e Ligações:
  ──────────────────

        ┌──────────────────────────────────┐
        │         CH224K (ESSOP-10)        │
        │                                  │
   CC1 ─┤1   CC1              VBUS    10├── VBUS (entrada)
   CC2 ─┤2   CC2              VDD      9├── 3.3V (do AP2112K)
        │                                  │
   GND ─┤3   GND              PG       8├── Power Good (opcional)
        │                                  │
  CFG1 ─┤4   CFG1             CFG3     7├── CFG3
  CFG2 ─┤5   CFG2             VOUT     6├── VOUT (saída = tensão negociada)
        │                                  │
        └──────────────────────────────────┘

  Configuração de Tensão (CFG1, CFG2, CFG3):
  ──────────────────────────────────────────

  ┌─────────────┬─────────┬─────────┬─────────┬─────────────────┐
  │ Tensão      │  CFG1   │  CFG2   │  CFG3   │ Resistor Total  │
  ├─────────────┼─────────┼─────────┼─────────┼─────────────────┤
  │ 5V          │  Float  │  Float  │  Float  │ Nenhum          │
  │ 9V          │  GND    │  Float  │  Float  │ CFG1 → GND      │
  │ 12V         │  Float  │  GND    │  Float  │ CFG2 → GND      │
  │ 15V         │  Float  │  Float  │  GND    │ CFG3 → GND      │
  │ 20V ★       │  GND    │  GND    │  Float  │ CFG1,2 → GND    │
  └─────────────┴─────────┴─────────┴─────────┴─────────────────┘

  ★ Configuração recomendada para v3: 20V (CFG1,CFG2 → GND)

  Circuito de Aplicação (20V):
  ────────────────────────────

                        USB-C Connector
                       ┌─────────────────┐
        VBUS ──────────┤ VBUS            │
                       │                 │
        CC1 ───────────┤ CC1             │
        CC2 ───────────┤ CC2             │
                       │                 │
        GND ───────────┤ GND             │
                       └─────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
              ▼               ▼               ▼
         ┌─────────────────────────────────────────┐
         │                 CH224K                  │
         │                                         │
    VBUS─┤ VBUS(10)                    CC1(1) ├───► CC1
         │                                         │
   3.3V ─┤ VDD(9)                      CC2(2) ├───► CC2
         │                                         │
   N/C  ─┤ PG(8)                       GND(3) ├───► GND
         │                                         │
   N/C  ─┤ CFG3(7) ─○ Float           CFG1(4)├─[0Ω]─ GND
         │                                         │
  20V ◄──┤ VOUT(6)                    CFG2(5)├─[0Ω]─ GND
         │                                         │
         └─────────────────────────────────────────┘
                   │
                   ▼
             Para SY8368A
             (VIN = 20V)

═══════════════════════════════════════════════════════════════

  Fallback Automático:
  ────────────────────

  Se a fonte não suportar 15V, o CH224K negocia automaticamente
  a próxima tensão disponível (20V, 12V, 9V, 5V).

  O firmware do ESP32 detecta a tensão real via ADC e ajusta
  o brilho máximo em conformidade.

═══════════════════════════════════════════════════════════════
```

### 3.2 U2 - SY8368AQQC (Buck Síncrono 8A)

O SY8368AQQC é um buck converter síncrono de alta eficiência com MOSFETs integrados.

```
SY8368AQQC - Buck Converter 8A:
═══════════════════════════════════════════════════════════════

  Características:
  ─────────────────
  • Tensão entrada: 4.5V - 18V
  • Tensão saída: 0.6V - VIN
  • Corrente: 8A contínua
  • Frequência: 500kHz (ajustável)
  • Eficiência: 93% @ 15V→5V, 3A
  • MOSFETs integrados (síncrono)
  • Package: QFN-20 (3x3mm)
  • LCSC: C207642

  Pinout:
  ───────

              ┌──────────────────────┐
              │   SY8368AQQC (QFN20) │
              │                      │
       BOOT ─┤1                  20├─ VIN
        SW ──┤2                  19├─ VIN
        SW ──┤3                  18├─ EN
        SW ──┤4                  17├─ SS
       PGND ─┤5                  16├─ PGOOD
       PGND ─┤6                  15├─ COMP
       PGND ─┤7                  14├─ FB
       PGND ─┤8                  13├─ SGND
       PGND ─┤9                  12├─ SGND
       PGND ─┤10                 11├─ MODE
              │                      │
              │      (PAD = PGND)    │
              └──────────────────────┘

  Circuito de Aplicação (15V → 5V @ 8A):
  ──────────────────────────────────────

                     VIN = 15V (do CH224K)
                          │
                          │
            ┌─────────────┼─────────────────────────────────────┐
            │             │                                     │
           ═╧═           ═╧═                                    │
       C_IN1          C_IN2                                     │
      22µF/25V       22µF/25V                                   │
       (1210)         (1210)                                    │
            │             │                                     │
            └──────┬──────┘                                     │
                   │                                            │
                   │  ┌─────────────────────────────────────┐   │
                   │  │           SY8368AQQC                │   │
                   │  │                                     │   │
                   ├──┤ VIN (19,20)                         │   │
                   │  │                                     │   │
                   │  │ EN (18) ────────────────────────────┼───┤
                   │  │              (ligar a VIN via 10k)  │   │
                   │  │                                     │   │
                   │  │ SS (17) ─────[10nF]───┬─ GND        │   │
                   │  │                       │             │   │
                   │  │ MODE (11) ──────────[GND] (forced CCM) │
                   │  │                                     │   │
                   │  │ FB (14) ◄────────────┤              │   │
                   │  │                      │              │   │
                   │  │ COMP (15) ───[R_C]───┼──[C_C]─ GND  │   │
                   │  │                      │              │   │
                   │  │ PGOOD (16) ─○ (opcional ao ESP32)   │   │
                   │  │                                     │   │
                   │  │ BOOT (1) ───[100nF]──┤              │   │
                   │  │                      │              │   │
                   │  │ SW (2,3,4) ──────────┼──────────────┤   │
                   │  │                      │              │   │
                   │  │ PGND (5-10,PAD) ─────┴─ GND         │   │
                   │  │                                     │   │
                   │  └─────────────────────────────────────┘   │
                   │                      │                     │
                   │                      │ SW                  │
                   │                      │                     │
                   │              ┌───────┴───────┐             │
                   │              │               │             │
                   │             ═╪═             ═╧═            │
                   │          L1 2.2µH       C_BOOT             │
                   │           10A          100nF               │
                   │              │               │             │
                   │              │               │             │
                   │              ├───────────────┘             │
                   │              │                             │
                   │              │ VOUT = 5V                   │
                   │              │                             │
            ┌──────┴──────────────┼───────────────────────┐     │
            │                     │                       │     │
           ═╧═                   ═╧═                     ═╧═    │
       C_OUT1                C_OUT2                  C_OUT3     │
      22µF/10V              22µF/10V                22µF/10V    │
       (1206)                (1206)                  (1206)     │
            │                     │                       │     │
            └─────────────────────┴───────────────────────┘     │
                                  │                             │
                                  │                             │
                                 ═╧═                            │
                            C_BULK                              │
                          470-1000µF                            │
                          10V Elect.                            │
                          (opcional)                            │
                                  │                             │
                                  │                             │
      GND ────────────────────────┴─────────────────────────────┘


  Rede de Feedback (5.0V):
  ────────────────────────

                          VOUT = 5V
                              │
                              │
                             [R1]  16.2kΩ (1%)
                              │
                              ├────────────► FB (pino 14)
                              │
                             [R2]  3.01kΩ (1%)
                              │
                             GND

  Cálculo: VOUT = 0.6V × (1 + R1/R2) = 0.6 × (1 + 16.2/3.01) = 5.03V ✓


  Rede de Compensação:
  ────────────────────

  COMP (pino 15) ───[R_C 10kΩ]───┬───[C_C 22nF]─── GND
                                 │
                            [C_HF 100pF]
                                 │
                                GND

  (Valores típicos para 500kHz, ajustar conforme estabilidade)

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

  ┌──────────────────┬────────┬───────┬───────┬─────────────────┐
  │ Part Number      │ Value  │ Isat  │ DCR   │ LCSC            │
  ├──────────────────┼────────┼───────┼───────┼─────────────────┤
  │ SRP1265A-2R2M    │ 2.2µH  │ 13A   │ 8.4mΩ │ C132462         │
  │ SWPA6045S2R2MT   │ 2.2µH  │ 8.5A  │ 16mΩ  │ C408335         │
  │ WPN5040S2R2MT    │ 2.2µH  │ 10A   │ 12mΩ  │ C495537         │
  └──────────────────┴────────┴───────┴───────┴─────────────────┘

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

  ENTRADA (perto do SY8368A):
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


  SAÍDA (perto do SY8368A):
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
│               │          CH224K (U1)            │                           │
│               │          PD Trigger             │                           │
│               │                                 │                           │
│     VBUS ─────┤ VBUS(10)              CC1(1) ───┼─► CC1                     │
│               │                                 │                           │
│     3.3V ─────┤ VDD(9)                CC2(2) ───┼─► CC2                     │
│               │                                 │                           │
│               │ PG(8) ─○              GND(3) ───┼─► GND                     │
│               │                                 │                           │
│   Float ○────┤ CFG3(7)              CFG1(4) ───┼─[0Ω]─ GND (20V)           │
│               │                                 │                           │
│  VOUT (20V) ◄─┤ VOUT(6)              CFG2(5) ───┼─[0Ω]─ GND                 │
│               │                                 │                           │
│               └─────────────────────────────────┘                           │
│                         │                                                   │
│                         │ VOUT = 20V (ou fallback 15V/9V/5V)                │
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
│                 │    │  │       SY8368AQQC (U2)       │ │                   │
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
│         │  16.2k   3.01k   22µF/10V                                         │
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

  VOUT do CH224K (5V-20V)
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
| U1 | CH224K | USB-C PD Trigger | ESSOP-10 | 1 | €0.35 | C970725 | Extended |
| U2 | SY8368AQQC | Buck Sync 8A | QFN-20 3x3 | 1 | €0.55 | C207642 | Basic |
| L1 | CKST0603-2.2uH/M | Indutor 2.2µH 10A | 6.6x6.6x3mm | 1 | €0.25 | C3002634 | Extended |
| | *Alt: SRP1265A-2R2M* | *2.2µH 13A* | *12.5x12.5mm* | 1 | €0.30 | C132462 | Basic |
| C_BOOT | 100nF 25V | MLCC X7R | 0402 | 1 | €0.01 | C307331 | Basic |
| C_IN | CL32A226KAJNNNE | MLCC 22µF 25V | 1210 | 4 | €0.08 | C52306 | Basic |
| C_OUT | CL31A226KAHL | MLCC 22µF 10V | 1206 | 6 | €0.03 | C12891 | Basic |
| C_BULK | 470µF 16V | Electrolítico | Φ8x10mm | 1 | €0.08 | C3339 | Basic |
| C_SS | 10nF 25V | MLCC X7R | 0402 | 1 | €0.01 | C15195 | Basic |
| R_FB1 | 16.2kΩ 1% | Resistor | 0402 | 1 | €0.01 | C25762 | Basic |
| R_FB2 | 3.01kΩ 1% | Resistor | 0402 | 1 | €0.01 | C25754 | Basic |
| R_EN | 10kΩ 1% | Resistor | 0402 | 1 | €0.01 | C25744 | Basic |
| R_CFG1 | 0Ω (jumper) | CFG1→GND (20V) | 0402 | 1 | €0.01 | C17168 | Basic |
| R_CFG2 | 0Ω (jumper) | CFG2→GND (20V) | 0402 | 1 | €0.01 | C17168 | Basic |
| R_C | 10kΩ | Compensação | 0402 | 1 | €0.01 | C25744 | Basic |
| C_C | 22nF | Compensação | 0402 | 1 | €0.01 | C1604 | Basic |
| C_HF | 100pF | Compensação | 0402 | 1 | €0.01 | C1546 | Basic |
| **Total PSU** | | | | | **~€2.20** | | |

### 6.2 Componentes Sensing (ADC)

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC |
|-----|------------|---------------|---------|-----|-------|------|
| R_SENSE1 | 100kΩ 1% | Divisor tensão | 0402 | 1 | €0.01 | C25741 |
| R_SENSE2 | 22kΩ 1% | Divisor tensão | 0402 | 1 | €0.01 | C25768 |
| C_FILTER | 100nF 16V | Filtro ADC | 0402 | 1 | €0.01 | C307331 |
| **Total Sensing** | | | | | **€0.03** | |

### 6.3 Proteções (do v2, mantidas)

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC |
|-----|------------|---------------|---------|-----|-------|------|
| J1 | USB-C Receptacle | 16-pin USB 2.0 | SMD | 1 | €0.40 | C165948 |
| F1 | PTC Fuse | 3A hold, 6A trip | 1812 | 1 | €0.15 | C369159 |
| D1 | TVS SMBJ24A | 24V (VBUS) | SMB | 1 | €0.25 | C114152 |
| D2 | D3V3XA4B10LP | 4-ch ESD | UDFN2510 | 1 | €0.25 | C2827654 |
| **Total Proteções** | | | | | **€1.05** | |

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

### 6.5 Terminais de Saída

| Ref | Componente | Especificação | Package | Qty | Preço | LCSC |
|-----|------------|---------------|---------|-----|-------|------|
| J2 | Screw Terminal | 2-pin 5.0mm 10A | THT | 1 | €0.10 | C474881 |
| C_TERM | 100µF 10V | MLCC (opcional) | 1206 | 2 | €0.30 | C408880 |
| **Total Terminais** | | | | | **€0.70** | |

### 6.6 Resumo Total

| Secção | Custo |
|--------|-------|
| PSU (CH224K + SY8368A + passivos) | €2.20 |
| Sensing (divisor ADC) | €0.03 |
| Proteções (TVS, PTC, ESD) | €1.05 |
| Regulador 3.3V | €0.25 |
| Terminais e bulk caps | €0.70 |
| **TOTAL PSU v3** | **~€4.25** |

> **Nota:** Custo ligeiramente superior ao v2 (~€3.25), mas com:
> - Buck de 8A (vs 3-5A)
> - Melhor compatibilidade de fontes
> - Gestão automática de potência

---

## 7. Layout PCB

### 7.1 Considerações Críticas

```
Layout Buck Converter - Boas Práticas:
═══════════════════════════════════════════════════════════════

  1. LOOP DE ALTA CORRENTE (minimizar área)
  ─────────────────────────────────────────

     VIN ──┬──[CIN]──┬──[SY8368 VIN/SW]──[L1]──┬──[COUT]──┬── VOUT
           │         │                         │          │
          GND       GND                       GND        GND
           │         │                         │          │
           └─────────┴─────────────────────────┴──────────┘
                         ▲
                         │
                   MANTER ESTE LOOP
                   O MAIS CURTO POSSÍVEL!

  2. PLACEMENT
  ────────────

     ┌─────────────────────────────────────────────────────────┐
     │                                                         │
     │   [USB-C]                                               │
     │      │                                                  │
     │      ▼                                                  │
     │   [CH224K]  ◄── Perto do conector USB-C                │
     │      │                                                  │
     │      │  VOUT (15V)                                      │
     │      ▼                                                  │
     │   [CIN x4] ── Imediatamente ao lado do SY8368          │
     │      │                                                  │
     │   [SY8368] ◄── Centro da área de potência              │
     │      │                                                  │
     │   [L1] ──────── Adjacente ao SW pin                    │
     │      │                                                  │
     │   [COUT x6-8] ─ Distribuídos após L1                   │
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
  • Evitar cortar o plano sob o indutor
  • Vias múltiplas nos pads PGND do SY8368
  • Separar ground analógico (ADC) do ground de potência
    com ferrite bead ou ligação em ponto único

  4. THERMAL
  ──────────

  • Pad exposto do SY8368 ligado ao ground com vias térmicas
  • Mínimo 9 vias (3x3) no thermal pad
  • Copper pour na top layer para dissipação

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
    ├─ CH224K negocia 15V (medir VOUT)
    ├─ SY8368 arranca (LED PWR acende)
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
    ├─ CH224K negocia 9V
    ├─ SY8368 produz 5V
    └─ Firmware detecta e capa brilho a 80%

  □ FALLBACK 5V
    ├─ Usar carregador básico 5V/3A
    ├─ CH224K fica em 5V (sem negociação)
    ├─ Buck em bypass (ou tensão mínima de saída)
    └─ Firmware detecta e capa brilho a 40%

  □ THERMAL
    ├─ Operação contínua 30 min @ 5A
    ├─ Temperatura SY8368 <85°C
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
  │ PD Controller          │ IP2721 / CYPD   │ CH224K          │
  │ Configuração           │ Complexa        │ Simples (strap) │
  │                        │                 │                 │
  │ Buck Converter         │ MP1584 (3A)     │ SY8368 (8A)     │
  │                        │ TPS54531 (5A)   │                 │
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
- [CH224K Datasheet](https://www.wch-ic.com/products/CH224.html) - WCH
- [SY8368AQQC Datasheet](https://www.silergy.com/product/detail/SY8368) - Silergy
- [Indutor CKST Series](https://www.coilcraft.com) - Coilcraft

### Application Notes
- [USB PD Sink Design with CH224K](https://www.wch-ic.com/downloads/CH224DS1_PDF.html)
- [High Current Buck Design](https://www.ti.com/lit/an/slva477b/slva477b.pdf) - TI

### Fóruns
- [CH224K Discussion - EEVblog](https://www.eevblog.com/forum/projects/ch224k-usb-pd-trigger/)
- [SY8368 Layout Tips](https://www.silergy.com/technical-documents)

---

*Documento criado: Janeiro 2025*
*Versão: 3.0 - USB-C PD 30W com Buck 8A*
*Baseado em: POWER_SUPPLY_v2.md*
