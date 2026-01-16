# PSU 20V Schema Design - Decisões de Design

Este documento regista as decisões de design tomadas para a PSU de 20V com proteções mínimas.

---

## 1. Visão Geral

### 1.1 Objectivo

PSU USB-C PD de 20V com:
- Proteções mínimas
- Fallback para 5V (jumper)
- Componentes Basic stock quando possível

### 1.2 Arquitectura

```
USB-C VBUS ──► F1 (PTC) ──► IP2721 + AO3400A ──► J_MODE ──► Buck ──► 5V
                              (PD Trigger)       (1×3)     (SY8368)
```

### 1.3 Modos de Operação (J_MODE)

Jumper J_MODE (1×3 pinos) permite selecionar entre modo normal e bypass completo:

```
         J_MODE [○ ○ ○]
                 1 2 3
                 │ │ │
F1 saída ────────┘ │ │  ← Bypass direto (5V USB)
                   │ │
5V cargas ◄────────┘ │  ← Saída comum
                     │
Buck VOUT ───────────┘  ← Saída regulada
```

| Posição | Jumper | Modo | Caminho | Saída |
|---------|--------|------|---------|-------|
| **2-3** | `[○ ■ ■]` | Normal PD | Q1 → Buck → cargas | 5V regulado |
| **1-2** | `[■ ■ ○]` | Bypass 5V | F1 → cargas (direto) | 5V limpos |

**Porque funciona sem conflito:**
- Em modo **Normal (2-3)**: Q1 abre, Buck recebe 20V, VOUT=5V vai às cargas. F1 (20V) no pino 1 fica isolado.
- Em modo **Bypass (1-2)**: Q1 fechado, Buck sem entrada, VOUT=0V. F1 (5V) vai direto às cargas.

### 1.4 Comportamento com Diferentes Fontes

| Fonte | J_MODE | IP2721 | Q1 | Buck | Resultado |
|-------|--------|--------|----|----- |-----------|
| PD 20V | 2-3 | Negocia 20V | Abre | 20V→5V | ✓ 5V regulado |
| PD 15V | 2-3 | Fallback 15V | Abre | 15V→5V | ✓ 5V regulado |
| PD 9V | 2-3 | Fallback 9V | Abre | 9V→5V | ✓ 5V regulado |
| USB 5V básico | 2-3 | Timeout | **Fechado** | — | ❌ Sem saída |
| USB 5V básico | **1-2** | Ignorado | Fechado | Sem entrada | ✓ **5V limpos** |

---

## 2. Bloco 1: Entrada USB-C + PD Trigger

### 2.1 Esquema

```
                                                    J_MODE
                                                ┌──[○ ○ ○]──┐
                                                │   1 2 3   │
                                                │   │ │ │   │
USB-C VBUS ───┬──[F1]───────────────────────────┴───┘ │ │   │
              │    │                                  │ │   │
              │    └────────► AO3400A D               │ │   │
              │                    │                  │ │   │
              │                    S ──► Buck VIN ────┘ │   │
              │                    │                    │   │
              │                    G               Buck VOUT┘
              │                    │                    │
              ├──► IP2721 VBUS     │                    ▼
              │    (pin 16)        │               5V Cargas
              │                    │                    │
             ═╧═ C_IN         IP2721:VBUSG             ═╧═ C2
             10µF 50V              │                 10µF 50V
              │              IP2721:VIN                 │
             GND                   │                   GND
                                  ═╧═ C1
                                  1µF 50V
                                   │
                                  GND

USB-C CC1 ────┬──► IP2721 pin 12 (CC1)
              │
             [R] 5.1kΩ (R_CC1)
              │
             GND

USB-C CC2 ────┬──► IP2721 pin 13 (CC2)
              │
             [R] 5.1kΩ (R_CC2)
              │
             GND

IP2721 pin 7 (SEL) ──[100kΩ]──► VIN (para selecionar 20V max)
```

### 2.2 Ligações IP2721

| Pino | Nome | Liga a | Notas |
|------|------|--------|-------|
| 1 | VIN | Source AO3400A + C1 | Alimentação IC (após MOSFET) |
| 4 | VBUSG | Gate AO3400A | Controla o MOSFET |
| 5,6,14,15 | GND | GND | Todos ao ground |
| 7 | SEL | 100kΩ → VIN | Seleciona 20V máximo |
| 12 | CC1 | USB-C CC1 | Comunicação PD |
| 13 | CC2 | USB-C CC2 | Comunicação PD |
| 16 | VBUS | USB-C VBUS + C_IN | Entrada (antes MOSFET) |
| 2,3,8-11 | N/C | Não ligar | Pinos não usados |

### 2.3 Ligações AO3400A (SOT-23)

| Pino | Nome | Liga a |
|------|------|--------|
| 1 | G (Gate) | IP2721 VBUSG (pin 4) |
| 2 | S (Source) | IP2721 VIN (pin 1) + C1 + Buck VIN |
| 3 | D (Drain) | F1 saída + IP2721 VBUS (pin 16) + C_IN |

### 2.4 Ligações J_MODE (1×3)

| Pino J_MODE | Liga a | Função |
|-------------|--------|--------|
| 1 | F1 saída | Entrada bypass (5V USB direto) |
| 2 | 5V cargas | Saída comum |
| 3 | Buck VOUT | Saída regulada do Buck |

**Posições do jumper:**
- **Pinos 2-3 ligados** `[○ ■ ■]`: Modo normal - Buck VOUT → cargas (5V regulado)
- **Pinos 1-2 ligados** `[■ ■ ○]`: Modo bypass - F1 → cargas (5V USB direto)

**Segurança**: Em modo bypass (1-2), se VBUS for >5V (ex: PD ativo), as cargas recebem essa tensão! Usar bypass **apenas** com carregadores USB 5V básicos.

### 2.5 Posição dos Condensadores

| Ref | Valor | Posição | Função |
|-----|-------|---------|--------|
| C_IN | 10µF 50V | VBUS → GND (entrada, antes MOSFET) | Filtro entrada |
| C1 | 1µF 50V | VIN → GND (perto IP2721) | Bypass IC |
| C2 | 10µF 50V | Saída → GND (perto Buck) | Estabilidade saída |

---

## 2B. Bloco 2: Buck Converter SY8368AQQC

### 2B.1 Especificações SY8368AQQC

| Parâmetro | Valor | Notas |
|-----------|-------|-------|
| VIN | 4V - 28V | ✓ Suporta 20V PD |
| VOUT | 0.6V - VIN | Ajustável via feedback |
| IOUT | 8A contínuo | 16A pico |
| VFB | 0.6V | Tensão referência feedback |
| Frequência | 800kHz | Pseudo-constante em CCM |
| RDS(ON) | 20mΩ / 10mΩ | High-side / Low-side |
| Package | QFN-20 3×3mm | Thermal pad central |
| LCSC | C207642 | **Basic** |

### 2B.2 Esquema de Aplicação

```
                            VIN (9-20V do Q1:Source)
                                    │
                    ┌───────────────┼───────────────┐
                    │               │               │
                   ═╧═             ═╧═              │
               C_VIN1          C_VIN2              │
              22µF 25V        22µF 25V             │
                │               │                  │
                └───────┬───────┘                  │
                        │                          │
                        │    ┌─────────────────────┤
                        │    │                     │
                        │    │  ┌─────────────────────────────────┐
                        │    │  │         SY8368AQQC (U2)         │
                        │    │  │              QFN-20              │
                        │    │  │                                  │
                        ├────┼──┤ VIN (19,20)          SW (2,3,4) ├──┐
                        │    │  │                                  │  │
                        │    ├──┤ EN (18)                          │  │
                        │    │  │                                  │  │
                        │    │  │ SS (17) ────[C_SS]──── GND      │  │
                        │    │  │            10nF                  │  │
                        │    │  │                                  │  │
                        │    │  │ MODE (11) ────────── GND (CCM)  │  │
                        │    │  │                                  │  │
                        │    │  │ BOOT (1) ────[C_BOOT]───┐       │  │
                        │    │  │             100nF       │       │  │
                        │    │  │                         │       │  │
                        │    │  │ FB (14) ◄───┬───────────│───────│──│──┐
                        │    │  │             │           │       │  │  │
                        │    │  │ COMP (15) ──┤           │       │  │  │
                        │    │  │             │           │       │  │  │
                        │    │  │ PGND (5-9) ─┴─ GND     │       │  │  │
                        │    │  │                         │       │  │  │
                        │    │  │ SGND (12,13) ── GND    │       │  │  │
                        │    │  │                         │       │  │  │
                        │    │  └─────────────────────────│───────┘  │  │
                        │    │                            │          │  │
                        │    └────────────────────────────┘          │  │
                        │                                            │  │
                       GND                      ┌─────────────────────┘  │
                                                │                        │
                                                │  L1 (2.2µH)            │
                                               ═╪═ ~~~~~ ═╪═             │
                                                │         │              │
                                                │         ├──────────────┤
                                                │         │              │
                                                │    ┌────┴────┐         │
                                                │    │         │         │
                                               ═╧═  ═╧═       ═╧═       [R_FB1]
                                            C_OUT1 C_OUT2   C_OUT3      22kΩ
                                           22µF×4  (1206)              │
                                                │         │              │
                                                │         │         VOUT (5V)
                                                │         │              │
                                               GND       GND        [R_FB2]
                                                                       3kΩ
                                                                         │
                                                                        GND
```

### 2B.3 Cálculo Resistências Feedback

**Fórmula**: `VOUT = VFB × (1 + R_FB1/R_FB2)`

Para VOUT = 5V com VFB = 0.6V:
```
5V = 0.6V × (1 + R_FB1/R_FB2)
5/0.6 = 1 + R_FB1/R_FB2
8.33 = 1 + R_FB1/R_FB2
R_FB1/R_FB2 = 7.33
```

**Valores escolhidos**:
- R_FB1 = 22kΩ (C25765, Basic)
- R_FB2 = 3kΩ (C25890, Basic)

**Verificação**: `0.6 × (1 + 22/3) = 0.6 × 8.33 = 5.0V ✓`

### 2B.4 Seleção do Indutor

**Requisitos**:
- Indutância: 2.2µH (recomendado datasheet)
- Corrente saturação: >10A (para 8A + margem)
- DCR: <15mΩ (para eficiência)

**Opções**:
| Ref | Modelo | Isat | DCR | LCSC | Stock |
|-----|--------|------|-----|------|-------|
| **L1** | **Bourns SRP1265A-2R2M** | **22A** | **4.2mΩ** | **C2831487** | Extended |
| L1 (alt) | CXP0630-2R2M-AG | 10A | ~10mΩ | C7187315 | Extended |

**Escolha**: Bourns SRP1265A-2R2M (C2831487) - 22A, DCR baixo, marca reconhecida.

### 2B.5 Condensadores

**Entrada (VIN)**:
| Ref | Valor | Qty | Tensão | LCSC | Stock | Footprint |
|-----|-------|-----|--------|------|-------|-----------|
| C_VIN | 22µF | 2 | 25V | C52306 | **Basic** | 1210 |

**Saída (VOUT)**:
| Ref | Valor | Qty | Tensão | LCSC | Stock | Footprint |
|-----|-------|-----|--------|------|-------|-----------|
| C_OUT | 22µF | 4 | 10V | C12891 | **Basic** | 1206 |

**Auxiliares**:
| Ref | Valor | Função | LCSC | Stock | Footprint |
|-----|-------|--------|------|-------|-----------|
| C_BOOT | 100nF 25V | Bootstrap | C307331 | **Basic** | 0402 |
| C_SS | 10nF 25V | Soft-start | C15195 | **Basic** | 0402 |

### 2B.6 Pinout SY8368AQQC (QFN-20)

```
              ┌──────────────────────┐
              │   SY8368AQQC (top)   │
              │                      │
       BOOT ─┤1                  20├─ VIN
         SW ─┤2                  19├─ VIN
         SW ─┤3                  18├─ EN
         SW ─┤4                  17├─ SS
       PGND ─┤5                  16├─ PGOOD
       PGND ─┤6                  15├─ COMP
       PGND ─┤7                  14├─ FB
       PGND ─┤8                  13├─ SGND
       PGND ─┤9                  12├─ SGND
         NC ─┤10                 11├─ MODE
              │                      │
              │    [THERMAL PAD]     │
              │        GND           │
              └──────────────────────┘
```

### 2B.7 Ligações SY8368AQQC

| Pino | Nome | Liga a | Notas |
|------|------|--------|-------|
| 1 | BOOT | C_BOOT → SW | Bootstrap para high-side driver |
| 2,3,4 | SW | L1 + C_BOOT | Nó de comutação |
| 5-9 | PGND | GND (power) | Ground de potência, múltiplas vias |
| 10 | NC | Não ligar | Não conectado |
| 11 | MODE | GND | CCM forçado (melhor para cargas constantes) |
| 12,13 | SGND | GND (signal) | Ground de sinal |
| 14 | FB | Divisor R_FB1/R_FB2 | Feedback para regulação |
| 15 | COMP | Rede compensação | Ou NC se estável |
| 16 | PGOOD | NC ou LED | Indicador power-good (opcional) |
| 17 | SS | C_SS → GND | Soft-start (10nF ≈ 6ms) |
| 18 | EN | VIN | Always-on (ou controlo externo) |
| 19,20 | VIN | Q1:Source + C_VIN | Entrada 9-20V |
| PAD | GND | GND via múltiplas vias | Dissipação térmica |

### 2B.8 Layout e Térmico

```
Regras de layout para SY8368:

1. LOOP DE ALTA CORRENTE (minimizar):
   VIN ─► C_VIN ─► U2(VIN/SW) ─► L1 ─► C_OUT ─► GND

2. PLACEMENT:
   C_VIN: Imediatamente junto aos pinos VIN (19,20)
   L1: Adjacente aos pinos SW (2,3,4)
   C_OUT: Distribuídos após L1
   C_BOOT: Entre pinos BOOT(1) e SW(2)

3. GROUND:
   - Plano contínuo na layer inferior
   - Mínimo 16 vias (4×4) no thermal pad
   - PGND e SGND ligados no mesmo ponto

4. THERMAL (crítico para 6A):
   - Copper pour 30×30mm mínimo na top layer
   - Vias térmicas 0.3mm no thermal pad
   - Heatsink adesivo 10×10mm recomendado
```

### 2B.9 Análise Térmica (6A)

**Dissipação @ 6A**:
```
P_OUT = 5V × 6A = 30W
η ≈ 91%
P_DISS = 30W / 0.91 - 30W = 3W
```

**Temperaturas estimadas**:
| Configuração | θJA | ΔT | Tj (25°C amb) |
|--------------|-----|-----|---------------|
| Layout básico | 45°C/W | 135°C | 160°C ❌ |
| Layout otimizado (16 vias) | 35°C/W | 105°C | 130°C ⚠️ |
| **Com heatsink 10×10mm** | **25°C/W** | **75°C** | **100°C ✓** |

**Recomendação**: Para 6A contínuo, usar heatsink adesivo.

---

## 3. Bill of Materials (BOM)

### 3.1 Bloco 1 - Componentes Principais

| Ref | Descrição | Valor | LCSC | Stock | Footprint KiCad |
|-----|-----------|-------|------|-------|-----------------|
| U1 | PD Trigger | IP2721 | C603176 | Extended | `Package_SO:TSSOP-16_4.4x5mm_P0.65mm` |
| Q1 | N-MOSFET | AO3400A 30V | C20917 | **Basic** | `Package_TO_SOT_SMD:SOT-23` |
| F1 | PTC Fuse | 3A 30V | C2982291 | Extended | `Fuse:Fuse_2920_7451Metric` |
| D1 | TVS Diode | SMBJ5.0A | C19077558 | Extended (Promo) | `Diode_SMD:D_SMB` |

### 3.2 Condensadores

| Ref | Descrição | Valor | LCSC | Stock | Footprint KiCad |
|-----|-----------|-------|------|-------|-----------------|
| C_IN | MLCC | 10µF 50V | C13585 | **Basic** | `Capacitor_SMD:C_1206_3216Metric` |
| C1 | MLCC | 1µF 50V | C15849 | **Basic** | `Capacitor_SMD:C_0603_1608Metric` |
| C2 | MLCC | 10µF 50V | C13585 | **Basic** | `Capacitor_SMD:C_1206_3216Metric` |

### 3.3 Resistências

| Ref | Descrição | Valor | LCSC | Stock | Footprint KiCad |
|-----|-----------|-------|------|-------|-----------------|
| R_SEL | Config 20V | 100kΩ | C25741 | **Basic** | `Resistor_SMD:R_0402_1005Metric` |
| R_CC1 | Fallback 5V | 5.1kΩ | C25905 | **Basic** | `Resistor_SMD:R_0402_1005Metric` |
| R_CC2 | Fallback 5V | 5.1kΩ | C25905 | **Basic** | `Resistor_SMD:R_0402_1005Metric` |

### 3.4 Conectores

| Ref | Descrição | Valor | LCSC | Stock | Footprint KiCad |
|-----|-----------|-------|------|-------|-----------------|
| J1 | USB-C Receptacle | 16-pin | (existente) | - | (existente no projeto) |
| J_MODE | Pin Header | 1×3P 2.54mm | C2337 | **Basic** | `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical` |

### 3.5 Bloco 2 - Buck Converter

| Ref | Descrição | Valor | LCSC | Stock | Footprint KiCad |
|-----|-----------|-------|------|-------|-----------------|
| U2 | Buck Converter | SY8368AQQC | C207642 | **Basic** | (easyeda2kicad - 12 pinos) |
| L1 | Indutor Bourns | 2.2µH 22A | C2831487 | Extended | (easyeda2kicad) |
| R_FB1 | Feedback Upper | 22kΩ 1% | **C31850** | **Basic** | `Resistor_SMD:R_0603_1608Metric` |
| R_FB2 | Feedback Lower | 3kΩ 1% | **C4211** | **Basic** | `Resistor_SMD:R_0603_1608Metric` |

**Cálculo Feedback**: VOUT = 0.6V × (1 + 22kΩ/3kΩ) = **5.00V** ✓

### 3.6 Bloco 2 - Condensadores

| Ref | Descrição | Valor | LCSC | Stock | Footprint KiCad |
|-----|-----------|-------|------|-------|-----------------|
| C_VIN1 | Input Cap | 22µF 25V | C52306 | **Basic** | `Capacitor_SMD:C_1210_3225Metric` |
| C_VIN2 | Input Cap | 22µF 25V | C52306 | **Basic** | `Capacitor_SMD:C_1210_3225Metric` |
| C_OUT1-4 | Output Cap | 22µF 10V ×4 | C12891 | **Basic** | `Capacitor_SMD:C_1206_3216Metric` |
| C_BOOT | Bootstrap | 100nF 25V | C307331 | **Basic** | `Capacitor_SMD:C_0402_1005Metric` |
| C_VCC | LDO Bypass | 1µF 10V | C15849 | **Basic** | `Capacitor_SMD:C_0603_1608Metric` |

### 3.7 Bloco 2 - Pinout IC (12-pin variant)

| Pino | Nome | Liga a | Notas |
|------|------|--------|-------|
| 1,3 | GND | GND | Ground |
| 2 | LX | L1 + C_BOOT | Switch node |
| 4,5,6 | IN | C_VIN + 20V_OUT | Entrada VIN |
| 7 | BS | C_BOOT → LX | Bootstrap |
| 8 | VCC | C_VCC → GND | LDO interno 3.3V |
| 9 | FB | Divisor R_FB | Feedback |
| 10 | ILMT | NC (flutuante) | 12A current limit |
| 11 | PG | NC | Power Good (opcional) |
| 12 | EN | VIN | Enable (always-on) |

### 3.8 Bloco 2 - Térmico

| Ref | Descrição | Valor | LCSC | Stock | Notas |
|-----|-----------|-------|------|-------|-------|
| HS1 | Heatsink adesivo | 10×10×5mm | C5184686 | **Basic** | Recomendado para 6A |

---

## 4. Decisões de Design

### 4.1 PTC Fuse (F1)

**Decisão**: C2982291 (ASMD2920-300-30V)

| Considerado | Specs | Problema |
|-------------|-------|----------|
| C883162 (16V) | 3A/16V | Tensão insuficiente para 20V |
| C960026 (2A) | 2A/30V | Corrente insuficiente para 9V fallback |
| **C2982291** | **3A/30V** | ✅ Escolhido |

**Razão**: Suporta fallback 9V (3.3A) com margem apertada mas aceitável, especialmente com brilho capado em software.

### 4.2 Condensadores de Entrada/Saída

**Decisão**: 50V em vez de 25V

| Tensão | Margem sobre 20V | Decisão |
|--------|------------------|---------|
| 25V | 25% | ⚠️ Apertado |
| **50V** | 150% | ✅ Escolhido |

**Razão**: Maior margem de segurança sem custo adicional significativo.

### 4.3 Fallback 5V com J_MODE

**Decisão**: Jumper 1×3 (J_MODE) com bypass direto + Resistências 5.1kΩ sempre montadas

**Configuração elegante do J_MODE**:
```
Pino 1: F1 saída ──────┐
                       │
Pino 2: 5V cargas ◄────┤ (saída comum)
                       │
Pino 3: Buck VOUT ─────┘
```

**Porque funciona**:
- Em bypass (1-2): F1 (5V) → cargas. Buck sem entrada (Q1 fechado), VOUT=0V → sem conflito
- Em normal (2-3): Buck VOUT (5V) → cargas. F1 (20V) no pino 1 isolado → sem conflito

**Sequência com carregador 5V básico (sem PD)**:
1. IP2721 tenta negociar PD → timeout (carregador não responde)
2. IP2721 **não abre** o MOSFET Q1 → Buck sem entrada
3. Com J_MODE em **1-2**: 5V passa direto do F1 às cargas (**5V limpos!**)

**Resistências 5.1kΩ**:
- Sempre montadas nos CC1/CC2
- Sinalizam ao carregador USB básico: "sou sink, dá-me 5V"
- Não interferem com operação normal do IP2721

### 4.4 Posição C1 vs C2

**Decisão**: Dois condensadores na saída (C1 + C2)

| Condensador | Posição física | Função |
|-------------|----------------|--------|
| C1 (1µF) | Perto IP2721 | Bypass rápido alta frequência |
| C2 (10µF) | Perto Buck | Reserva energia, picos |

**Razão**: Condensadores pequenos respondem mais rápido, grandes armazenam mais. Juntos cobrem todas as situações.

### 4.5 Mecânica Detalhada do J_MODE

**Esquema de ligações do J_MODE:**

```
                              J_MODE
                          ┌──[○ ○ ○]──┐
                          │   1 2 3   │
                          │   │ │ │   │
F1 saída ─────────────────┴───┘ │ │   │  ← Pino 1: VBUS após fuse (bypass)
                                │ │   │
5V cargas ◄─────────────────────┘ │   │  ← Pino 2: Saída comum
                                  │   │
Buck VOUT ────────────────────────┘   │  ← Pino 3: Saída regulada
                                      │
               Q1:Source ──► Buck VIN─┘  (ligação fixa, não passa pelo J_MODE)
```

**Modo Normal (Jumper 2-3): PD com Buck**

```
[○ ■ ■]  Pinos 2-3 ligados
 1 2-3

USB-C PD ──► F1 ──► Q1:D ──► Q1:S ──► Buck VIN ──► Buck VOUT ──► J_MODE:3 ──► J_MODE:2 ──► 5V cargas
  20V              (abre)                              5V
                    │
                F1 saída (20V) no pino 1 fica ISOLADO das cargas ✓
```

**Modo Bypass (Jumper 1-2): 5V Direto**

```
[■ ■ ○]  Pinos 1-2 ligados
 1-2 3

USB-C 5V ──► F1 ──► J_MODE:1 ──► J_MODE:2 ──► 5V cargas (5V LIMPOS!)
                                    │
                   Buck VOUT (0V) no pino 3 fica ISOLADO ✓
                   (Q1 fechado → Buck sem entrada)
```

**Vantagem desta configuração**: 5V limpos em bypass, sem dropout do Buck!

### 4.6 Proteção contra Uso Incorreto do Jumper

**⚠️ Risco**: Em modo bypass (1-2) com fonte PD (20V), as cargas receberiam 20V!

**Solução**: TVS de 5.1V na saída (D1)

```
J_MODE:2 (saída) ──┬──► C2 ──► GND
                   │
                   ├──► 5V cargas
                   │
                  ─┴─ D1 (TVS SMBJ5.0A)
                   │
                  GND
```

**Comportamento do TVS:**
| Situação | Tensão Saída | TVS | Resultado |
|----------|--------------|-----|-----------|
| Normal (5V) | 5V | Não conduz | OK |
| Bypass 5V | 5V | Não conduz | OK |
| Bypass 20V (erro!) | >5.5V | **Conduz** | Limita tensão, F1 dispara |

**Componente:**
| Ref | Valor | LCSC | Footprint |
|-----|-------|------|-----------|
| D1 | SMBJ5.0A | C19077558 | `Diode_SMD:D_SMB` |

**Specs SMBJ5.0A:**
- Vbr (breakdown): 6.4V
- Vc (clamping @ 1A): 9.2V
- Ppk: 600W (pulso)

Com bypass 20V: TVS limita a ~9V enquanto F1 (3A) dispara, protegendo as cargas.

---

## 5. Resumo de Custos

### 5.1 Bloco 1 (Entrada USB-C PD)

| Tipo | Qty | Preço unit. | Total |
|------|-----|-------------|-------|
| IP2721 | 1 | ~€0.40 | €0.40 |
| AO3400A | 1 | ~€0.03 | €0.03 |
| PTC Fuse | 1 | ~€0.05 | €0.05 |
| TVS SMBJ5.0A | 1 | ~€0.05 | €0.05 |
| MLCC 10µF (×2) | 2 | ~€0.03 | €0.06 |
| MLCC 1µF | 1 | ~€0.01 | €0.01 |
| Resistências (×3) | 3 | ~€0.01 | €0.03 |
| Pin Header | 1 | ~€0.02 | €0.02 |
| **Total Bloco 1** | | | **~€0.65** |

### 5.2 Bloco 2 (Buck Converter)

| Tipo | Qty | Preço unit. | Total |
|------|-----|-------------|-------|
| SY8368AQQC | 1 | ~€0.55 | €0.55 |
| Indutor 2.2µH | 1 | ~€0.25 | €0.25 |
| MLCC 22µF 25V (C_VIN) | 2 | ~€0.08 | €0.16 |
| MLCC 22µF 10V (C_OUT) | 4 | ~€0.03 | €0.12 |
| MLCC 100nF (C_BOOT) | 1 | ~€0.01 | €0.01 |
| MLCC 10nF (C_SS) | 1 | ~€0.01 | €0.01 |
| Resistências FB (×2) | 2 | ~€0.01 | €0.02 |
| Heatsink 10×10mm | 1 | ~€0.08 | €0.08 |
| **Total Bloco 2** | | | **~€1.20** |

### 5.3 Total PSU

| Bloco | Custo |
|-------|-------|
| Bloco 1 (USB-C PD) | €0.65 |
| Bloco 2 (Buck + Heatsink) | €1.20 |
| **Total PSU** | **~€1.85** |

---

## 6. Próximos Passos

- [x] ~~Bloco 2: Buck Converter SY8368AQQC~~ → Secção 2B
- [x] ~~Bloco 3: Condensadores entrada/saída do Buck~~ → Secção 2B.5
- [x] ~~Bloco 4: Resistências feedback (divisor para 5V)~~ → Secção 2B.3
- [x] ~~Bloco 5: Proteção adicional~~ → TVS D1 (secção 4.6)
- [ ] Atualizar guia KiCad com Bloco 2
- [ ] Criar símbolo SY8368AQQC no KiCad

---

## 7. Referências

- [IP2721 Datasheet](https://datasheet.lcsc.com/lcsc/2006111335_INJOINIC-IP2721_C603176.pdf)
- [SY8368AQQC Datasheet](https://www.lcsc.com/datasheet/lcsc_datasheet_2302161530_Silergy-Corp-SY8368AQQC_C207642.pdf)
- [POWER_SUPPLY_v3.md](./POWER_SUPPLY_v3.md) - Documentação anterior
- [Hackaday TS100 USB-C Project](https://cdn.hackaday.io/files/1721877366848608/V1_2%20Schematic_TS100%20-%20USB%20C%20IP2721.pdf)

---

*Documento criado: Janeiro 2025*
*Versão: 2.0 - Bloco 1 + Bloco 2 (Buck Converter)*
