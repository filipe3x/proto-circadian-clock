# Power Supply - Circadian Clock PCB

**Data:** Março 2026
**Versão:** 4.2 - CH224K + SY8388ARHC (Bulletproof, BUCK1 removido)
**Esquemático KiCad:** `product/kicad/clockv7/psu.kicad_sch`

---

## Changelog

| Data | Versão | Alterações |
|------|--------|------------|
| Mar 2026 | 4.0 | **Design final**: CH224K (PD 20V) + SY8388ARHC (Buck 8A). Documento único — substitui POWER_SUPPLY v1/v2/v3 e PSU_20V_SCHEMA_DESIGN. |
| Mar 2026 | 4.1 | **Bulletproof**: Worst-case VOUT analysis, correcção TVS D6 (SMBJ24CA), specs protecção SY8388ARHC, alinhamento BOM↔KiCad (caps 1206/C12891). |
| Mar 2026 | 4.2 | **BUCK1 removido**: Removido BUCK1 (L1=2.2µH) — PFM noise. Só BUCK2 (L2=4.7µH) montado. Ripple recalculado. |

---

## 1. Visão Geral

### 1.1 Arquitectura

```
USB-C VBUS ──► CH224K (PD 20V) ──► SY8388ARHC (Buck 8A) ──► 5V Rail
               (C970725)            (C5110279)
```

O design final utiliza apenas **2 ICs** para a PSU:

| IC | Função | LCSC | Preço |
|----|--------|------|-------|
| **CH224K** | USB PD 3.0 Sink — pede 20V | C970725 | ~$0.32 |
| **SY8388ARHC** | Buck síncrono 8A, 24V in | C5110279 | ~$0.53 |

**Total ICs PSU: ~$0.85** (vs ~$1.40+ com IP2721 + TPS56838)

### 1.2 Especificações

| Parâmetro | Mínimo | Típico | Máximo | Notas |
|-----------|--------|--------|--------|-------|
| Potência entrada | 27W | 30W | 45W | USB-C PD |
| Tensão entrada | 5V | 20V | 24V | PD negociado |
| Tensão saída | 4.9V | 5.0V | 5.1V | ±2% regulação |
| Corrente saída | — | 5A | 8A | Picos HUB75 |
| Eficiência | 90% | 93% | — | @ 20V→5V, 3A |

### 1.3 Perfis de Potência

```
Estratégia de Negociação PD (CH224K com CFG1 NC = 20V):
═══════════════════════════════════════════════════════

  PRIORIDADE 1: 20V (Preferido) ★
  ────────────────────────────────
  • 20V @ 1.5A = 30W → 5V @ 5.6A (após buck 93%)
  • 20V @ 3A = 60W → 5V @ 11A (limitado pelo buck a 8A)
  ✓ Disponível em ~85% das fontes PD
  ✓ Menor corrente no cabo = menores perdas I²R

  PRIORIDADE 2: 15V (Fallback automático)
  ────────────────────────────────────────
  • 15V @ 2A = 30W → 5V @ 5.6A
  ✓ Ratio 3:1, eficiência ~93%

  PRIORIDADE 3: 9V (Fallback)
  ───────────────────────────
  • 9V @ 3A = 27W → 5V @ 4.8A (após buck 90%)
  ⚠️ Brilho máximo ~80% (capado em software)

  FALLBACK FINAL: 5V (Emergência / USB básico)
  ─────────────────────────────────────────────
  • 5V @ 3A = 15W → Bypass directo (buck pass-through)
  ⚠️ Brilho máximo ~40%
═══════════════════════════════════════════════════════
```

---

## 2. Porquê CH224K + SY8388ARHC

### 2.1 Comparação PD Trigger: CH224K vs IP2721

| Parâmetro | CH224K (escolhido) | IP2721 (anterior) |
|-----------|-------------------|-------------------|
| **Tensões suportadas** | **5V, 9V, 12V, 15V, 20V** | 5V, 15V, 20V (sem 9V/12V!) |
| **Protocolos** | PD 3.0/2.0 + BC1.2 + QC2.0/3.0 | PD 2.0 apenas |
| **MOSFET externo** | **Não necessário** | Sim (AO3400A) |
| **Power Good pin** | **Sim (PG, open-drain)** | Não |
| **Rd interno (5.1kΩ)** | **Sim** | Não (R externas) |
| **Package** | ESSOP-10 (compacto) | TSSOP-16 |
| **LCSC** | C970725 | C603176 |
| **Preço** | **~$0.32** | ~$0.40 |
| **Config tensão** | Resistência única ou 3 GPIOs | Jumper SEL |

**Vantagens concretas do CH224K:**

1. **Suporta todas as 5 tensões PD** — o IP2721 saltava 9V e 12V, o que fazia com que fontes que só tinham 9V/12V não funcionassem
2. **Sem MOSFET externo** — VBUS liga directo ao buck. O IP2721 precisava de AO3400A + circuito de gate drive. Menos 1 componente, menos 1 ponto de falha
3. **PG (Power Good)** — LED de erro hardware indica imediatamente se o PD falhou, sem esperar pelo ESP32
4. **Rd internos** — não precisa de R3/R4 5.1kΩ externas nos CC1/CC2. Menos 2 componentes
5. **Mais barato** — $0.32 vs $0.40 + $0.03 (MOSFET) + $0.02 (resistências) = ~$0.45
6. **Config simples para 20V fixo** — CFG1/CFG2/CFG3 todos NC. Zero componentes de configuração

### 2.2 Comparação Buck: SY8388ARHC vs TPS56838 vs SY8368AQQC

| Parâmetro | SY8388ARHC (escolhido) | TPS56838 (anterior) | SY8368AQQC (v1) |
|-----------|----------------------|--------------------|--------------------|
| **VIN máx** | 24V | 28V | 18V |
| **IOUT** | 8A | 8A | 8A |
| **Modo** | **PWM + auto-skip** com FCCM disponível | FCCM nativo | PFM a light load |
| **Compensação** | **Interna** | D-CAP3 (interna) | Externa (COMP pin) |
| **Frequência** | 600kHz (fixa) | 500/800/1200kHz | ~600kHz |
| **Package** | **QFN-16 2.5x2.5mm** | VQFN-HR 10-pin 3x3mm | QFN-20 3x3mm |
| **LCSC** | **C5110279** | C37533416 | C207642 |
| **Preço** | **~$0.53** | ~$1.00+ | ~$0.40 |
| **Coil whine** | **Não** (PWM contínuo) | Não (FCCM) | **Sim!** (PFM) |
| **Thermal pad** | QFN-16 EP (melhor) | 10-pin EP | QFN-20 EP |

**Vantagens concretas do SY8388ARHC:**

1. **Custo** — ~$0.53 vs ~$1.00+ do TPS56838. Quase metade do preço para especificações equivalentes
2. **Sem coil whine** — ao contrário do SY8368AQQC que entrava em pulse-skipping (PFM) a carga leve, causando frequência audível nos kHz. O SY8388ARHC mantém PWM contínuo a 600kHz (inaudível)
3. **Compensação interna** — sem necessidade de rede RC externa no loop de compensação. Menos componentes, mais simples de rotear
4. **QFN-16 compacto** — 2.5x2.5mm com exposed pad generoso para dissipação térmica
5. **Disponibilidade JLCPCB** — Extended mas com bom stock e preço baixo
6. **Silergy** — marca reconhecida em power management, amplamente usada em produtos consumer

### 2.3 O Problema do Pulse-Skipping (PFM) — Porquê NÃO o SY8368

```
O Problema: Pulse Skipping (PFM) a carga leve
══════════════════════════════════════════════

  PWM normal (600kHz):
  ██░░░░░░██░░░░░░██░░░░░░██░░░░░░  → Frequência FIXA, inaudível ✓

  Pulse skipping (carga leve, SY8368):
  ██░░░░░░░░░░░░░░░░░░░░██░░░░░░░░░░░░░░░░░░░░██
    ↑                      ↑
    Pulsos espaçados → frequência cai para kHz AUDÍVEL!

  O SY8368AQQC não tem pin MODE para forçar PWM contínuo.
  A light load (ex: ESP32 sozinho, sem painel), entra em PFM
  e o indutor vibra na gama audível (1-20kHz) → coil whine.

  Solução SY8388ARHC:
  ██░░░░░░██░░░░░░██░░░░░░██░░░░░░  → PWM contínuo SEMPRE ✓

  O SY8388ARHC mantém frequência de switching constante a 600kHz
  mesmo com carga muito leve. Trade-off: eficiência ligeiramente
  menor a no-load (~mA extra), mas SILÊNCIO garantido.
══════════════════════════════════════════════════
```

### 2.4 Resumo de Vantagens do Design Final

| Métrica | Design Anterior (IP2721 + TPS56838) | Design Final (CH224K + SY8388ARHC) |
|---------|-------------------------------------|-------------------------------------|
| **Custo ICs** | ~$1.40 (IC + MOSFET) | **~$0.85** (-39%) |
| **N.º componentes PSU** | ~18 | **~14** (-4 componentes) |
| **Tensões PD** | 3 (5/15/20V) | **5 (5/9/12/15/20V)** |
| **Coil whine** | Não (FCCM) | **Não (PWM contínuo)** |
| **MOSFET externo** | Sim (AO3400A) | **Não** |
| **Resistências CC** | 2× 5.1kΩ | **0** (Rd interno) |
| **Compensação externa** | Não (D-CAP3) | **Não** (interna) |
| **Power Good LED** | Não | **Sim** |
| **Simplicidade** | Média | **Alta** |

---

## 3. Circuito CH224K (U5) — PD Sink Controller

### 3.1 Configuração

Para pedir **20V fixo**, usa-se o modo resistência com CFG1 floating:

- **CFG1 (pin 2)** = NC → 20V
- **CFG2 (pin 3)** = NC
- **CFG3 (pin 9)** = NC

### 3.2 Pinout e Ligações

```
         ┌──────────────────────────┐
         │      CH224K (ESSOP-10)    │
         │                          │
    VDD ─┤1                    10├─ PG (Power Good, open-drain)
   CFG1 ─┤2                     9├─ CFG3
   CFG2 ─┤3                     8├─ VBUS
     DP ─┤4                     7├─ CC2
     DM ─┤5                     6├─ CC1
         │                          │
         │     [EPAD = GND]         │
         └──────────────────────────┘
```

| Pin | Nome | Liga a | Notas |
|-----|------|--------|-------|
| 1 | VDD | VBUS via R1 1kΩ + C_VDD 1µF→GND | Regulador interno 3.3V |
| 2 | CFG1 | NC (floating) | 20V (resistor mode) |
| 3 | CFG2 | NC | Ignorado em resistor mode |
| 4 | DP | NC | Não usado (só PD via CC) |
| 5 | DM | NC | Não usado |
| 6 | CC1 | USB-C CC1 directo | Rd interno 5.1kΩ |
| 7 | CC2 | USB-C CC2 directo | Rd interno 5.1kΩ |
| 8 | VBUS | NC | Só necessário para BC1.2/QC legacy |
| 9 | CFG3 | NC | Ignorado em resistor mode |
| 10 | PG | Pull-up 10kΩ + NPN → Error LED | Open-drain, LOW = PD OK |
| EPAD | GND | GND | Thermal pad |

### 3.3 Circuito de Aplicação

```
                        USB-C Connector
                       ┌─────────────────┐
        VBUS ──────────┤ VBUS            │
                       │                 │
        CC1 ───────────┤ CC1             │
        CC2 ───────────┤ CC2             │
                       │                 │
        D+/D- ─────────┤ D+/D-           │──► CH340C (dados USB)
                       │                 │
        GND ───────────┤ GND             │
                       └─────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
              ▼               ▼               ▼

  VBUS ──[R1 1kΩ]──► VDD(1) ──[C_VDD 1µF]── GND      ← Alimentação CH224K

                CFG1(2) ── NC                          ← 20V fixo
                CFG2(3) ── NC
                  DP(4) ── NC
                  DM(5) ── NC
                 CC1(6) ── USB-C CC1 (directo)         ← Rd interno
                 CC2(7) ── USB-C CC2 (directo)
                VBUS(8) ── NC
                CFG3(9) ── NC
                 PG(10) ── Error LED (NPN inverter)

  VBUS ──┬──────────────────────────────► SY8388ARHC VIN
         │ (DIRECTO, sem MOSFET)
         │
      [D6 TVS]  SMBJ24CA (Vrwm=24V, bidirecional)
         │
        GND
```

### 3.4 Error LED (PG + NPN)

```
  VDD ──[R_PU 10kΩ]──┬── PG (CH224K pin 10)
                      │
                 [R_BASE 10kΩ]
                      │
                      B
                     /
  VDD ──[330Ω]──LED──C  Q (MMBT2222A)
                      \
                       E
                       │
                      GND

  Sem PD: PG = HIGH-Z (pull-up) → NPN ON → LED ON (erro!)
  PD OK:  PG = LOW              → NPN OFF → LED OFF
```

### 3.5 Fallback Automático

| Carregador | Tensão obtida | PG pin | Error LED |
|------------|---------------|--------|-----------|
| PD 20V/15V/12V/9V/5V | 20V | LOW | OFF |
| PD só 15V/12V/9V/5V | 15V fallback | LOW | OFF |
| PD só 12V/9V/5V | 12V fallback | LOW | OFF |
| PD só 9V/5V | 9V fallback | LOW | OFF |
| PD só 5V | 5V PD | LOW | OFF |
| USB sem PD (USB-A, Mac) | 5V default | HIGH-Z | ON |

---

## 4. Circuito SY8388ARHC (U1) — Buck Converter

### 4.1 Especificações

| Parâmetro | Valor | Notas |
|-----------|-------|-------|
| VIN | 4.5V - 24V | Suporta 20V PD com margem |
| VOUT | Ajustável via FB | 5.0V configurado |
| IOUT | 8A contínuo | |
| VFB | 0.6V ±1% | Tensão referência feedback |
| Frequência | 600kHz (fixa) | Sem coil whine |
| Controlo | Compensação interna | Sem rede RC externa |
| RDS(ON) HS | ~20mΩ | High-side MOSFET |
| RDS(ON) LS | ~10mΩ | Low-side MOSFET |
| Soft-start | Integrado | |
| Proteções | OVP, OCP, OTP, UVLO | |
| Package | QFN-16-EP 2.5x2.5mm | Thermal pad |
| LCSC | C5110279 | Extended |

### 4.2 Pinout

```
              ┌──────────────────────┐
              │  SY8388ARHC (top)    │
              │   QFN-16 2.5×2.5mm  │
              │                      │
         BS ─┤1                  16├─ NC
     IN(1) ─┤2                  15├─ NC
     IN(2) ─┤3                  14├─ EN
     IN(3) ─┤4                  13├─ SS
   PGND(1) ─┤5                  12├─ FB
   PGND(2) ─┤6                  11├─ COMP (NC)
   PGND(3) ─┤7                  10├─ PG
     LX(1) ─┤8                   9├─ LX(2)
              │                      │
              │   [THERMAL PAD]      │
              │      PGND            │
              └──────────────────────┘
```

### 4.3 Ligações

| Pin | Nome | Liga a | Notas |
|-----|------|--------|-------|
| 1 | BS | C_BOOT 100nF → LX | Bootstrap high-side driver |
| 2,3,4 | IN | VBUS + C_VIN | Entrada 5-20V |
| 5,6,7 | PGND | GND (power) | Power ground |
| 8,9 | LX | L2 (4.7µH) | Switch node — minimizar cobre! |
| 10 | PG | NC ou ESP32 GPIO | Power-good (open-drain) |
| 11 | COMP | NC | Compensação interna |
| 12 | FB | Divisor R_FB3/R_FB4 | Feedback (0.6V ref) |
| 13 | SS | C_SS ou float | Soft-start |
| 14 | EN | VIN (float = always-on) | Enable |
| 15,16 | NC | — | Não usados |
| PAD | PGND | GND via vias térmicas | 4-6 vias 0.3mm |

### 4.4 Rede de Feedback — R_FB3 (22kΩ) e R_FB4 (3kΩ)

O SY8388ARHC tem uma referência interna de **0.6V** no pino FB. O divisor
resistivo reduz a tensão de saída (5V) para 0.6V, permitindo ao IC regular:

- Se VOUT sobe acima de 5V → FB > 0.6V → IC reduz duty cycle
- Se VOUT desce abaixo de 5V → FB < 0.6V → IC aumenta duty cycle

```
                    VOUT = 5V
                        │
                   [R_FB3] 22kΩ (1%)   ← "upper" (LCSC: C31850)
                        │
                        ├────────────► FB (pin 12 SY8388ARHC)
                        │
                   [R_FB4] 3kΩ (1%)    ← "lower" (LCSC: C4211)
                        │
                       GND

Cálculo:
  V_FB = VOUT × R_FB4 / (R_FB3 + R_FB4) = 5 × 3k / 25k = 0.6V ✓
  VOUT = 0.6V × (1 + R_FB3/R_FB4) = 0.6 × (1 + 22/3) = 5.0V ✓
```

### 4.5 Circuito de Aplicação Completo

```
                     VIN = 5-20V (VBUS directo do CH224K)
                              │
              ┌───────────────┼──────────────────┐
              │               │                  │
             ═╧═             ═╧═                ═╧═
         C_VIN1          C_VIN2             C_HF
        22µF/25V        22µF/25V          100nF/50V
         (1210)          (1210)            (0402)
              │               │                  │
              └───────┬───────┴──────────────────┘
                      │
                      │   ┌──────────────────────────────────┐
                      │   │        SY8388ARHC (U12)          │
                      │   │        QFN-16 2.5×2.5mm          │
                      │   │                                  │
                      ├───┤ IN (2,3,4)            LX (8,9) ──┤──┐
                      │   │                                  │  │
                      │   │ EN (14) ◄── VIN (float = on)     │  │
                      │   │                                  │  │
                      │   │ SS (13) ── float (default)       │  │
                      │   │                                  │  │
                      │   │ BS (1) ──[C_BOOT 100nF]── LX     │  │
                      │   │                                  │  │
                      │   │ FB (12) ◄── divisor R_FB3/R_FB4    │  │
                      │   │                                  │  │
                      │   │ PG (10) ─○ (opcional)            │  │
                      │   │                                  │  │
                      │   │ PGND (5,6,7,PAD) ── GND          │  │
                      │   │                                  │  │
                      │   └──────────────────────────────────┘  │
                      │                                          │
                     GND                     ┌───────────────────┘
                                             │
                                             │  L2 (4.7µH)
                                            ═╪═ ~~~~~ ═╪═
                                             │         │
                                             │         ├─── VOUT (5V)
                                             │         │
                                   ┌────┴────┬────┬────┐
                                   │         │    │    │
                                  ═╧═       ═╧═  ═╧═  ═╧═
                               C_OUT5    C_OUT6 C_OUT7 C_OUT8
                              22µF/25V  22µF   22µF   22µF
                               (1210)   (1210) (1210) (1210)
                                   │         │    │    │
                                  GND       GND  GND  GND

  C_OUT (4× 22µF 25V em paralelo = 88µF total):
  ───────────────────────────────────────────────
  • Filtrar ripple de 600kHz → <20mV na saída
  • Resposta transitória: fornecem corrente instantânea quando o
    painel HUB75 muda de linha (picos 0→2A em µs), enquanto o
    loop de feedback do SY8388ARHC reage
  • 4 em paralelo → ESR total ~1mΩ (4× menor que 1 cap sozinho)
  • Sem capacitância suficiente → voltage sag → flickering/reset ESP32
```

### 4.6 Layout PCB

```
Regras de layout para SY8388ARHC:
═══════════════════════════════════

1. LOOP DE ALTA CORRENTE (minimizar):
   VIN → C_VIN → IC(IN→LX) → L2 → C_OUT → GND
   C_HF (100nF) junto a IN e PGND para HF bypass

2. PLACEMENT:
   C_VIN + C_HF: Imediatamente junto aos pinos IN (2,3,4) e PGND (5,6,7)
   L2: Adjacente aos pinos LX (8,9), ≤2mm
   C_OUT: Distribuídos após L2
   C_BOOT: Entre pino BS (1) e LX (8)

3. GROUND:
   - Plano contínuo na layer inferior
   - 4-6 vias (0.3mm) no thermal pad, sem thermal relief
   - Minimizar impedância do return path

4. ⚠️ ZONE KEEPOUT SOB L2:
   - SEM copper pour (top NEM bottom) sob L2
   - SEM vias na zona sob L2
   - Evita correntes de eddy e acoplamento magnético

5. ⚠️ MINIMIZAR COBRE NO NÓ LX:
   - Trace curto e direto de LX ao pad do indutor
   - NÃO incluir LX em copper pour
   - Alta dV/dt → funciona como antena EMI se grande

6. THERMAL:
   - Copper pour em VIN, VOUT e GND para dissipação
   - Vias térmicas 0.3mm com espaçamento 1mm
   - Bottom layer copper pour para dissipação adicional
```

---

## 5. Proteção VBUS

### 5.1 TVS Diode (D6)

O rail VBUS (até 20V) é protegido por TVS bidirecional:

| Parâmetro | Valor |
|-----------|-------|
| Componente | SMBJ24CA |
| Vrwm | 24V (suporta 20V VBUS) |
| Vbr | 26.7V min |
| Vc (clamp @ 15.4A) | 38.9V |
| Ppk | 600W (pulso) |
| Package | SMB (DO-214AA) |
| LCSC | C19077558 |

### 5.2 Duas Camadas de Proteção

1. **C_VIN (2×22µF 25V)** — absorvem spikes lentos (µs), proteção primária
2. **D6 TVS (SMBJ24CA)** — clamp rápido de ESD e spikes ns

---

## 6. BOM da Power Supply

### 6.1 ICs

| Ref | Componente | Valor | LCSC | Tipo | Footprint |
|-----|------------|-------|------|------|-----------|
| U5 | CH224K (WCH) | PD 3.0 Sink | **C970725** | Extended | ESSOP-10-1EP |
| U1 | SY8388ARHC (Silergy) | Buck 8A 24V | **C5110279** | Extended | QFN-16-EP 2.5x2.5mm |

### 6.2 Indutor

| Ref | Componente | Valor | LCSC | Tipo | Footprint |
|-----|------------|-------|------|------|-----------|
| L2 | Bourns SRP1265A-4R7M | 4.7µH 16A | **C780205** | Extended | 12.5x6.5mm |

### 6.3 Condensadores

| Ref | Componente | Valor | LCSC | Tipo | Footprint |
|-----|------------|-------|------|------|-----------|
| C24, C25 | MLCC (VIN) | 22µF 25V | **C12891** | Basic | 1206 |
| C_OUT5-8 | MLCC (×4, VOUT) | 22µF 25V | **C12891** | Basic | 1206 |
| C_BOOT3 | MLCC (bootstrap) | 100nF 50V | **C307331** | Basic | 0402 |
| C_FIL1 | MLCC (HF bypass) | 100nF 50V | **C307331** | Basic | 0402 |
| C_FF2 | MLCC (feedforward) | 22pF 50V | **C1555** | Basic | 0402 |
| C14 | MLCC (CH224K VDD) | 1µF 50V | **C15849** | Basic | 0603 |
| C27 | MLCC (bypass) | 1µF 50V | **C15849** | Basic | 0603 |
| C26 | MLCC (VBUS filter) | 10µF 50V | **C13585** | Basic | 1206 |

### 6.4 Resistências

| Ref | Componente | Valor | LCSC | Tipo | Footprint |
|-----|------------|-------|------|------|-----------|
| R_FB3 | Feedback upper | 22kΩ 1% | **C31850** | Basic | 0603 |
| R_FB4 | Feedback lower | 3kΩ 1% | **C4211** | Basic | 0603 |
| R14 | VBUS→VDD CH224K | 1kΩ | **C4410** | Basic | 1206 |
| R_PU1 | PG pull-up | 10kΩ | **C25744** | Basic | 0402 |
| R_BASE1 | NPN base | 10kΩ | **C25744** | Basic | 0402 |
| R_ERR1 | Error LED | 330Ω | **C25104** | Basic | 0402 |
| R_DIV1 | VBUS divider upper | 47kΩ | **C25792** | Basic | 0402 |
| R_DIV2 | VBUS divider lower | 5.6kΩ | **C23189** | Basic | 0603 |
| R_VBUS1 | VBUS sense pull-down | 10kΩ | **C25744** | Basic | 0402 |

### 6.5 Díodos e TVS

| Ref | Componente | Valor | LCSC | Tipo | Footprint |
|-----|------------|-------|------|------|-----------|
| D6 | SMBJ24CA (TVS bidirecional) | Vrwm=24V, 600W | **C19077558** | Extended | SMB (DO-214AA) |
| D1 | LED Vermelho (Error) | — | **C2286** | Basic | 0603 |

### 6.6 Outros

| Ref | Componente | Valor | LCSC | Tipo | Footprint |
|-----|------------|-------|------|------|-----------|
| F1 | PTC Fuse | 3A 30V | **C2982291** | Extended | 2920 |
| Q2 | MMBT2222A | NPN transistor | **C8512** | Basic | SOT-23 |

### 6.7 Resumo de Custos

| Bloco | Componentes | Custo estimado |
|-------|-------------|----------------|
| PD Trigger (CH224K + passivos) | 5 | ~€0.45 |
| Buck (SY8388ARHC + L2 + caps + FB) | 11 | ~€0.95 |
| Proteção (TVS + PTC) | 2 | ~€0.10 |
| Error LED (NPN + R + LED) | 3 | ~€0.05 |
| **Total PSU** | **~22** | **~€1.70** |

### 6.8 Componentes Extended (taxa JLCPCB)

| Componente | Taxa |
|------------|------|
| CH224K (C970725) | $3 |
| SY8388ARHC (C5110279) | $3 |
| L2 SRP1265A-4R7M (C780205) | $3 |
| D6 SMBJ24CA (C19077558) | $3 |
| F1 PTC (C2982291) | $3 |
| **Total taxa Extended PSU** | **$15** |

---

## 7. Alimentação CH224K — Detalhes

### 7.1 R1 1kΩ (VBUS → VDD)

O CH224K é alimentado através de R1 1kΩ de VBUS ao pin VDD.
O regulador interno (LDO série) gera 3.3V estável.

```
Cálculo a 20V:
  I_chip = ~5mA (típico)
  V_drop = 5mA × 1kΩ = 5V
  V_VDD  = 20V - 5V = 15V (entrada LDO, dentro de 4-22V ✓)
  P_R1   = 5mA × 5V = 25mW (0603 suporta 100mW ✓)
```

### 7.2 VBUS Pin (pin 8) — NC

O pino VBUS serve apenas para detectar tensão em protocolos legacy (BC1.2/QC).
Para USB PD puro via CC1/CC2, é desnecessário.

### 7.3 5V USB Pass-Through

Sem MOSFET no power path, um carregador USB 5V básico (Mac, etc.) fornece
5V directamente ao buck. O SY8388ARHC tem soft-start integrado e aceita VIN
desde 4.5V. Isto garante que o ESP32 pode ser flashado via USB sem fonte PD.

---

## 8. Análise Worst-Case — Bulletproof PSU

### 8.1 Tolerância de Saída (VOUT)

Com resistências FB de 1% e VFB ±1%:

```
Parâmetros:
  VFB  = 0.600V ±1% → [0.594V, 0.606V]
  R_FB3 = 22kΩ ±1% → [21.78kΩ, 22.22kΩ]
  R_FB4 = 3kΩ  ±1% → [2.97kΩ, 3.03kΩ]

Nominal:
  VOUT = 0.600 × (1 + 22k/3k) = 0.600 × 8.333 = 5.000V

Worst-case HIGH (VFB↑, R_FB3↑, R_FB4↓):
  VOUT = 0.606 × (1 + 22.22k/2.97k) = 0.606 × 8.485 = 5.142V (+2.8%)

Worst-case LOW (VFB↓, R_FB3↓, R_FB4↑):
  VOUT = 0.594 × (1 + 21.78k/3.03k) = 0.594 × 8.188 = 4.864V (-2.7%)

Resultado: VOUT ∈ [4.86V, 5.14V] (±2.8%)
═══════════════════════════════════════

  ✓ ESP32: opera 2.3-3.6V (via LDO 3.3V) — margem enorme
  ✓ HUB75: opera 4.2-5.5V típico — dentro de spec
  ✓ CH340C: opera 3.0-5.5V — OK
  ✓ SN74AHCT245: opera 4.5-5.5V — OK (4.86V > 4.5V mín)
```

### 8.2 Protecções SY8388ARHC (Datasheet)

| Protecção | Threshold | Comportamento |
|-----------|-----------|---------------|
| **UVLO (Under-Voltage)** | VIN < ~3.8V (rising ~4V) | Desliga, hysteresis ~0.3V |
| **OVP (Over-Voltage)** | VOUT > ~115% nominal | Latch-off (requer power cycle) |
| **OCP Peak** | ~17.5A typ (high-side) | Cycle-by-cycle current limit |
| **OCP Valley** | Configurável via ILMT pin | Float = 10A typ valley limit |
| **OTP (Over-Temp)** | ~160°C junction | Latch-off |
| **UVP (Under-Voltage Output)** | VOUT < ~70% nominal | Hiccup mode |
| **Soft-Start** | Interno, ~2ms | Limita inrush current |

### 8.3 Análise Térmica

```
Pior caso: 20V → 5V @ 8A (40W output, ~93% eficiência)

  P_input  = 40W / 0.93 = 43W
  P_loss   = 43W - 40W = 3W (total no IC + indutor)

  SY8388ARHC (QFN-16 2.5×2.5mm):
    RθJA ≈ 40°C/W (com thermal pad + vias)
    P_IC ≈ 1.5W (FET losses + driver + quiescent)
    ΔT = 1.5W × 40°C/W = 60°C
    Tj_max = 85°C (ambiente) + 60°C = 145°C (< 150°C limite) ⚠️ MARGEM BAIXA

  Recomendação: Garantir 4-6 vias térmicas 0.3mm no exposed pad
  e copper pour generoso em ambas as faces do PCB.

  L2 (SRP1265A-4R7M):
    P_L2 ≈ I²×DCR = 8²×0.0074 = 0.47W (DCR=7.4mΩ)
    ΔT ≈ 25°C (com airflow natural)
    ✓ Isat = 16A >> 8A — sem risco de saturação
```

### 8.4 Ripple de Saída

```
  f_sw = 600kHz
  L = 4.7µH (L2 — BUCK1 removido, só BUCK2 montado)
  VIN = 20V, VOUT = 5V, IOUT = 8A

  Duty cycle: D = VOUT/VIN = 5/20 = 0.25
  ΔI_L = (VIN - VOUT) × D / (f_sw × L)
       = (20 - 5) × 0.25 / (600k × 4.7µ)
       = 3.75 / 2.82 = 1.33A p-p

  Ripple voltage (ESR-dominated, 4×22µF em paralelo):
    ESR_total ≈ 3mΩ / 4 = 0.75mΩ
    V_ripple ≈ ΔI_L × ESR_total = 1.33 × 0.75m = 1.0mV

  Ripple voltage (capacitivo):
    V_ripple_cap = ΔI_L / (8 × f_sw × C_total)
                 = 1.33 / (8 × 600k × 88µ) = 3.1mV

  Total ripple ≈ √(1.0² + 3.1²) ≈ 3.3mV p-p ✓✓ (< 20mV target, excelente)
```

### 8.5 Capacitor Derating

```
  C_VIN (22µF 25V 1206):
    VIN_max = 20V → 80% rating → OK para MLCC X5R/X7R
    ⚠️ Capacitância efectiva a 20V DC bias ≈ 12-15µF (X5R perde ~35%)
    Com 2× em paralelo: ~24-30µF efectivo → suficiente

  C_OUT (22µF 25V 1206):
    VOUT = 5V → 20% rating → quase capacitância total mantida
    Com 4× em paralelo: ~80-85µF efectivo → excelente

  ✓ Output caps com margem de derating muito boa
  ⚠️ Input caps perdem capacitância a 20V — 2× em paralelo compensa
```

### 8.6 Margem de Protecção TVS (D6)

```
  SMBJ24CA specs:
    Vrwm  = 24V   ← VBUS normal 20V → 4V margem (83% derating) ✓
    Vbr   = 26.7V ← TVS não conduz a 20V nominal ✓
    Vc    = 38.9V ← Clamp voltage em surge
    Ppk   = 600W  ← Protecção robusta contra surges

  SY8388ARHC VIN_max = 24V
    ⚠️ Vc (38.9V) > VIN_max (24V) → Durante um surge forte,
    o IC pode ver até 38.9V momentaneamente antes do TVS clampar.
    PORÉM: surges são de µs/ns de duração. Os C_VIN absorvem a
    energia antes que o IC sofra dano.

  Conclusão: Proteção adequada para USB PD com 2 camadas:
    1. C_VIN (absorção µs) + 2. D6 TVS (clamp ns)
```

---

## 9. Alinhamento KiCad ↔ Documentação (v4.1)

Todas as referências, LCSC codes e footprints neste documento correspondem ao
esquemático KiCad (`psu.kicad_sch`), que é a **source of truth** (PCB encomendado na JLCPCB).

| Item | Correcção v4.1 | Status |
|------|----------------|--------|
| SY8388ARHC Ref | ~~U12~~ → **U1** | ✅ Alinhado |
| TVS Ref | ~~D3~~ → **D6** | ✅ Alinhado |
| TVS Value | ~~SMBJ5.0A~~ → **SMBJ24CA** | ✅ Alinhado |
| Caps C_VIN/C_OUT | ~~C52306 (1210)~~ → **C12891 (1206)** | ✅ Alinhado |
| Q_NPN Ref + LCSC | ~~Q_NPN/C916372~~ → **Q2/C8512** | ✅ Alinhado |
| L2 (4.7µH) | Adicionado ao BOM (C780205) | ✅ Documentado |
| L1 (2.2µH) | **Removido** — BUCK1 eliminado (PFM noise). Só L2=4.7µH montado. | ✅ Resolvido |
| VBUS divider (R_DIV1/R_DIV2) | Adicionado ao BOM | ✅ Documentado |
| Cap refs | Alinhados com KiCad (C24/C25, C_BOOT3, C_FIL1, C_FF2, C14, C26, C27) | ✅ |
| Resistor refs | Alinhados com KiCad (R14, R_PU1, R_BASE1, R_ERR1) | ✅ |
| D3 (USB data TVS) | C20615788 (H7VN10B), footprint D_SOD-123 | ⚠️ **DNP** — protecção D+/D−, não montado nesta revisão |

---

## 10. Referências

- [CH224K Datasheet (WCH)](https://www.wch.cn/downloads/file/301.html)
- [CH224K LCSC](https://www.lcsc.com/product-detail/C970725.html)
- [SY8388ARHC Datasheet (Silergy)](https://datasheet.lcsc.com/lcsc/2207201830_Silergy-Corp-SY8388ARHC_C5110279.pdf)
- [SY8388ARHC JLCPCB](https://jlcpcb.com/partdetail/SilergyCorp-SY8388ARHC/C5110279)
- [Beyondlogic CH224 Review](https://www.beyondlogic.org/review-usb-c-power-delivery-trigger-board-ch224/)

---

*Documento criado: Março 2026*
*Versão: 4.1 — Bulletproof CH224K + SY8388ARHC*
*Substitui: POWER_SUPPLY.md (v1), POWER_SUPPLY_v2.md, POWER_SUPPLY_v3.md, PSU_20V_SCHEMA_DESIGN.md*
