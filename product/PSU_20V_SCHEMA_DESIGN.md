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
USB-C VBUS ──► F1 (PTC) ──► IP2721 + AO3400A ──► Buck ──► 5V
                              (PD Trigger)      (futuro)
```

### 1.3 Fallback 5V

Jumper J_BYPASS permite bypass do IP2721 para operação básica a 5V:
- **Aberto**: Modo normal, IP2721 negocia 20V/15V/9V
- **Fechado**: Bypass, 5V direto via resistências 5.1kΩ nos CC

---

## 2. Bloco 1: Entrada USB-C + PD Trigger

### 2.1 Esquema

```
                                         J_BYPASS
                                   ┌───────○───────┐
                                   │               │
USB-C VBUS ───┬────────────────────┴──► AO3400A D  │
              │                              │     │
              │                              S ────┴──┬──► Saída 20V
              │                              │        │
              ├──► IP2721 VBUS (pin 16)      G        │
              │                              │        │
             ═╧═ C_IN 10µF 50V               │       ═╧═ C1 1µF 50V
              │                              │        │
             GND               IP2721 VBUSG (pin 4)  ═╧═ C2 10µF 50V
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
| 2 | S (Source) | Saída 20V + IP2721 VIN + C1 + C2 |
| 3 | D (Drain) | USB-C VBUS + J_BYPASS |

### 2.4 Posição dos Condensadores

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
| J_BYPASS | Pin Header | 1×2P 2.54mm | C36717 | **Basic** | `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical` |

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

### 4.3 Fallback 5V

**Decisão**: Jumper + Resistências 5.1kΩ sempre montadas

**Razão**:
- IP2721 é componente crítico, probabilidade de erro na primeira vez
- Fallback permite testar a PCB mesmo com IP2721 mal soldado
- Resistências 5.1kΩ não interferem com operação normal do IP2721

### 4.4 Posição C1 vs C2

**Decisão**: Dois condensadores na saída (C1 + C2)

| Condensador | Posição física | Função |
|-------------|----------------|--------|
| C1 (1µF) | Perto IP2721 | Bypass rápido alta frequência |
| C2 (10µF) | Perto Buck | Reserva energia, picos |

**Razão**: Condensadores pequenos respondem mais rápido, grandes armazenam mais. Juntos cobrem todas as situações.

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
