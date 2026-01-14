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
Q1:Source ───────┘ │ │
                   │ │
Buck VIN ──────────┘ │
                     │
Buck VOUT / 5V ──────┘
```

| Posição | Jumper | Modo | Caminho | Saída |
|---------|--------|------|---------|-------|
| **2-3** | `[○ ■ ■]` | Normal PD | Q1 → Buck | 5V regulado |
| **1-2** | `[■ ■ ○]` | Bypass MOSFET | F1 → Buck | ~4.5V (dropout) |

**Notas**:
- Em modo **Normal (2-3)**: IP2721 negocia PD, abre Q1, tensão alta vai ao Buck, saída 5V regulado
- Em modo **Bypass (1-2)**: Salta o Q1, 5V do USB vai direto ao Buck, mas Buck com VIN≈VOUT tem dropout
- Para bypass **completo** (5V limpos), ver secção 4.6 - Alternativa A com dois jumpers

### 1.4 Comportamento com Diferentes Fontes

| Fonte | J_MODE | IP2721 | Q1 | Buck | Resultado |
|-------|--------|--------|----|----- |-----------|
| PD 20V | 2-3 | Negocia 20V | Abre | 20V→5V | ✓ 5V regulado |
| PD 15V | 2-3 | Fallback 15V | Abre | 15V→5V | ✓ 5V regulado |
| PD 9V | 2-3 | Fallback 9V | Abre | 9V→5V | ✓ 5V regulado |
| USB 5V básico | 2-3 | Timeout | **Fechado** | — | ❌ Sem saída |
| USB 5V básico | **1-2** | Ignorado | Bypassed | 5V→~4.5V | ⚠️ ~4.5V (dropout) |

---

## 2. Bloco 1: Entrada USB-C + PD Trigger

### 2.1 Esquema

```
                                              J_MODE
                                          ┌──[○ ○ ○]──┐
                                          │   1 2 3   │
                                          │   │ │ │   │
USB-C VBUS ───┬──[F1]─────────► AO3400A D │   │ │ │   │
              │                     │     │   │ │ │   │
              │                     S ────┴───┘ │ │   │
              │                     │           │ │   │
              │                     G      Buck VIN   │
              ├──► IP2721 VBUS      │           │     │
              │    (pin 16)         │           │     │
              │                     │      Buck VOUT──┴──► 5V Cargas
             ═╧═ C_IN              ═╧═ C1       │
             10µF 50V              1µF 50V    ═╧═ C2
              │                     │         10µF 50V
             GND                    │           │
                                    │          GND
                          IP2721 VBUSG (pin 4)
                                    │
                          IP2721 VIN (pin 1)


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
| 2 | S (Source) | IP2721 VIN (pin 1) + C1 + **J_MODE pino 1** |
| 3 | D (Drain) | F1 saída + IP2721 VBUS (pin 16) + C_IN |

### 2.4 Ligações J_MODE (1×3)

| Pino J_MODE | Liga a | Função |
|-------------|--------|--------|
| 1 | Q1:Source (AO3400A) | Saída do MOSFET (9V/15V/20V) |
| 2 | Buck VIN (SY8368) | Entrada do conversor |
| 3 | Buck VOUT / 5V cargas | Saída final |

**Posições do jumper:**
- **Pinos 1-2 ligados**: Modo normal - energia passa pelo Buck
- **Pinos 2-3 ligados**: Modo bypass - 5V direto às cargas (salta Q1 + Buck)

### 2.5 Posição dos Condensadores

| Ref | Valor | Posição | Função |
|-----|-------|---------|--------|
| C_IN | 10µF 50V | VBUS → GND (entrada, antes MOSFET) | Filtro entrada |
| C1 | 1µF 50V | VIN → GND (perto IP2721) | Bypass IC |
| C2 | 10µF 50V | Saída → GND (perto Buck) | Estabilidade saída |

---

## 3. Bill of Materials (BOM) - Bloco 1

### 3.1 Componentes Principais

| Ref | Descrição | Valor | LCSC | Stock | Footprint KiCad |
|-----|-----------|-------|------|-------|-----------------|
| U1 | PD Trigger | IP2721 | C603176 | Extended | `Package_SO:TSSOP-16_4.4x5mm_P0.65mm` |
| Q1 | N-MOSFET | AO3400A 30V | C20917 | **Basic** | `Package_TO_SOT_SMD:SOT-23` |
| F1 | PTC Fuse | 3A 30V | C2982291 | Extended | `Fuse:Fuse_2920_7451Metric` |

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

**Decisão**: Jumper 1×3 (J_MODE) + Resistências 5.1kΩ sempre montadas

**Razão para jumper 3 pinos em vez de 2**:
- Com 5V de entrada, o Buck SY8368 **não funciona** (VIN ≈ VOUT, sem headroom)
- Jumper 2 pinos só bypassava o MOSFET, não o Buck
- Jumper 3 pinos permite bypass **completo** (MOSFET + Buck)

**Sequência com carregador 5V básico (sem PD)**:
1. IP2721 tenta negociar PD → timeout (carregador não responde)
2. IP2721 **não abre** o MOSFET Q1 → saída = 0V
3. Com J_MODE em 2-3: 5V passa direto às cargas, ignorando Q1 e Buck

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
F1 saída ─────────────────┴───┘ │ │   │  ← Pino 1: VBUS após fuse
                                │ │   │
Q1:Source ──────────────────────┤ │   │  ← Também liga ao pino 2
                                │ │   │
                           Buck VIN   │  ← Pino 2: Entrada Buck
                                │     │
                           Buck VOUT──┴──► 5V cargas  ← Pino 3: Saída
```

**Modo Normal (Jumper 2-3): PD com Buck**

```
[○ ■ ■]  Pinos 2-3 ligados
 1 2-3

USB-C PD ──► F1 ──► Q1:D ──► Q1:S ──► Buck VIN ──► Buck VOUT ──► 5V
  20V              (abre)            (via pino 2)   (via pino 3)
```

- IP2721 negocia tensão → abre Q1
- 20V/15V/9V passa pelo Q1 → entra no Buck
- Buck converte para 5V regulado

**Modo Bypass (Jumper 1-2): 5V Direto**

```
[■ ■ ○]  Pinos 1-2 ligados
 1-2 3

USB-C 5V ──► F1 ──► J_MODE:1 ──► J_MODE:2 ──► Buck VIN
                                                  │
                                            (Buck não regula,
                                             passa ~5V direto)
                                                  │
                                             Buck VOUT ──► 5V cargas
```

- Q1 fica fechado (IP2721 não abre sem PD)
- 5V entra direto pelo pino 1 → pino 2 → Buck
- Buck com VIN≈VOUT opera em ~100% duty cycle, passa 5V (com pequena queda)

### 4.6 Limitação e Alternativa

**Limitação do jumper 1×3**: Em modo bypass (1-2), os 5V ainda passam pelo Buck. Com VIN≈VOUT, o Buck opera no limite e a saída cai para ~4.5-4.7V (dropout).

**Se precisares de 5V "limpos" em bypass**, há duas alternativas:

**Alternativa A: Dois jumpers de 2 pinos**

```
J_MOSFET (1×2)           J_BUCK (1×2)
    [○ ○]                    [○ ○]
     1 2                      1 2
     │ │                      │ │
F1 ──┴─┤                      │ │
       │                      │ │
Q1:S ──┘──► Buck VIN ─────────┴─┤
                                │
                     Buck VOUT──┘──► 5V cargas
```

| J_MOSFET | J_BUCK | Modo |
|----------|--------|------|
| Aberto | Aberto | Normal PD (via Q1 e Buck) |
| Fechado | Aberto | Bypass MOSFET (Buck ainda ativo) |
| Fechado | Fechado | Bypass completo (5V direto) |

**Alternativa B: Manter 1×3 e aceitar ~4.5V em bypass**

Para a maioria dos casos (LEDs, ESP32), 4.5-4.7V é suficiente para funcionar em modo de teste/debug.

---

## 5. Resumo de Custos (Bloco 1)

| Tipo | Qty | Preço unit. | Total |
|------|-----|-------------|-------|
| IP2721 | 1 | ~€0.40 | €0.40 |
| AO3400A | 1 | ~€0.03 | €0.03 |
| PTC Fuse | 1 | ~€0.05 | €0.05 |
| MLCC 10µF (×2) | 2 | ~€0.03 | €0.06 |
| MLCC 1µF | 1 | ~€0.01 | €0.01 |
| Resistências (×3) | 3 | ~€0.01 | €0.03 |
| Pin Header | 1 | ~€0.02 | €0.02 |
| **Total Bloco 1** | | | **~€0.60** |

---

## 6. Próximos Passos

- [ ] Bloco 2: Buck Converter SY8368AQQC (20V → 5V)
- [ ] Bloco 3: Condensadores entrada/saída do Buck
- [ ] Bloco 4: Resistências feedback (divisor para 5V)
- [ ] Bloco 5: Proteção adicional (TVS opcional)

---

## 7. Referências

- [IP2721 Datasheet](https://datasheet.lcsc.com/lcsc/2006111335_INJOINIC-IP2721_C603176.pdf)
- [POWER_SUPPLY_v3.md](./POWER_SUPPLY_v3.md) - Documentação anterior
- [Hackaday TS100 USB-C Project](https://cdn.hackaday.io/files/1721877366848608/V1_2%20Schematic_TS100%20-%20USB%20C%20IP2721.pdf)

---

*Documento criado: Janeiro 2025*
*Versão: 1.0 - Bloco 1 (Entrada USB-C PD)*
