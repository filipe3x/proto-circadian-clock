# BOM Verificado - Circadian Clock PCB

**Data:** Janeiro 2026
**Versão:** 4.1 - Componentes Alinhados com KiCad + PSU Bulletproof
**Fonte:** JLCPCB BOM Detection + Verificação Manual
**Total:** 24 componentes

---

## Histórico de Alterações

| Versão | Data | Alterações |
|--------|------|------------|
| 4.1 | Mar 2026 | **Alinhamento KiCad**: Remover componentes PSU obsoletos, alinhar refs/LCSC com KiCad. PSU v4.1 Bulletproof. |
| 3.0 | Jan 2026 | Substituição de Extended por Basic: D1→MMBD4148SE, R6→1206, U1→PCF8563T, U6→AMS1117, U2/U7→SN74AHCT245PWR |
| 2.0 | Jan 2026 | Análise completa Basic vs Extended |
| 1.0 | Jan 2026 | BOM inicial |

---

## Otimização de Custos: Basic vs Extended

### Porque Importa?

Na produção PCBA via JLCPCB, os componentes dividem-se em duas categorias fundamentais:

| Tipo | Taxa de Carregamento | Descrição |
|------|---------------------|-----------|
| **Basic** | **$0** | Componentes comuns já montados nas máquinas P&P |
| **Preferred Extended** | **$0** | Extended mas isentos de taxa no Economic PCBA |
| **Extended** | **$3 por tipo único** | Requerem carregamento manual de feeders |

### Impacto Financeiro

Considerando que temos **~10 componentes Extended** no BOM atual:

```
Custo adicional Extended = 10 × $3 = $30 por encomenda
```

Este custo é **fixo por lote**, independentemente da quantidade de PCBs. Numa encomenda de 5 protótipos, são **$6 extra por placa** só em taxas de carregamento!

### Estratégia de Otimização

1. **Substituir Extended por Basic** sempre que possível
2. **Aceitar Extended inevitáveis** (ESP32, RTC, USB-C) - são críticos
3. **Avaliar alternativas** com mesmo footprint quando viável
4. **Usar Preferred Extended** - isentos de taxa no Economic PCBA

### Componentes Extended Inevitáveis

Alguns componentes não têm alternativa Basic viável:

| Componente | Razão |
|------------|-------|
| **ESP32-WROOM-32E** (U3) | MCU principal, sem alternativa |
| **CH340C** (U4) | USB-Serial necessário |
| **CH224K** (U5) | PD Sink — ver secção PSU |
| **SY8388ARHC** (U1) | Buck converter — ver secção PSU |

### Substituições já Aplicadas (v4.1)

| Anterior (Extended) | Atual | Economia | Nota |
|---------------------|-------|----------|------|
| DS3231SN → **PCF8563T** (U6) | Preferred + Cristal Basic | **~$5** | ✅ Aplicado |
| ME6211 → **AMS1117-3.3** (U8) | Basic (SOT-223) | **$3** | ✅ Aplicado |
| IP2721 → **CH224K** (U5) | Extended mas mais barato | **$0.13** | ✅ Aplicado (PSU v4.1) |
| UMH3N → **MMBT2222A** (Q1) | Basic | **$3** | ✅ Aplicado |
| R3,R4 (5.1kΩ CC) | **Removidos** (CH224K Rd interno) | **-2 componentes** | ✅ Aplicado |

---

## Verificação de Substituições v3.0

### D1: 1N4148 → MMBD4148SE ✅ APROVADO

| Parâmetro | 1N4148 (Original) | MMBD4148SE (Novo) | Compatível? |
|-----------|-------------------|-------------------|-------------|
| **LCSC** | C84410 | **C17179590** | - |
| **Package** | DO-35 (THT) | **SOT-23 (SMD)** | Novo footprint |
| **Vrrm** | 100V | 100V | ✅ |
| **If(av)** | 300mA | 200mA | ✅ (suficiente) |
| **Vf** | ~0.65V | 1V max @ 10mA | ✅ |
| **trr** | 4ns | 4ns | ✅ |
| **Tipo** | Basic | Basic | ✅ |

**Nota:** MMBD4148SE é a versão SMD oficial da família 1N4148. Equivalência elétrica confirmada.

### D3: TVS para linhas D+/D− ⚠️ DNP (reservado, não montado)

| Parâmetro | C20615788 (H7VN10B) | Notas |
|-----------|---------------------|-------|
| **LCSC** | C20615788 | ✅ |
| **Fabricante** | hongjiacheng | |
| **Package** | DFN1006-2L | Compacto |
| **Vrwm** | ~7.5V | ✅ adequado para D+/D− (0–3.3V) |
| **Vbr** | 9V | |
| **Vc (clamp)** | 9.6V | |
| **Ipp** | 6A @ 8/20µs | |
| **Ppk** | 80W | |
| **ESD** | IEC 61000-4-2 | ✅ |
| **Tipo** | Extended | +$3 quando montado |

**Função:** Protecção ESD nas linhas USB D+ e D−. Um TVS por linha, em paralelo entre a linha e GND.

**Estado: DNP — não montado nesta revisão.** O footprint está reservado no PCB para instalação futura caso seja necessária protecção ESD nas linhas de dados USB.

> ⚠️ **Não confundir com D6 (SMBJ24CA):** D3 é para D+/D− (Vrwm~7.5V, linhas de sinal 3.3V). D6 é para VBUS (Vrwm=24V, rail de potência até 20V). Um **não** substitui o outro.

### R6: 1kΩ 0402 → 1kΩ 1206 ✅ APROVADO

| Parâmetro | Original | Novo | Status |
|-----------|----------|------|--------|
| **LCSC** | C106235 | **C4410** | ✅ |
| **Package** | 0402 | **1206** | Novo footprint |
| **Potência** | 63mW | 250mW | ✅ Melhor |
| **Tolerância** | ±1% | ±1% | ✅ |
| **Tipo** | Basic | **Basic** | ✅ |

### U1: DS3231SN → PCF8563T ✅ APROVADO

| Parâmetro | DS3231SN (Original) | PCF8563T (Novo) | Status |
|-----------|---------------------|-----------------|--------|
| **LCSC** | C722469 | **C7440** | ✅ |
| **Package** | SOIC-16W | **SOIC-8** | Novo footprint |
| **Interface** | I2C | I2C | ✅ |
| **Cristal** | Integrado (TCXO) | **Externo C32346** | +1 componente |
| **Precisão** | ±2ppm | ±20ppm | Aceitável |
| **Preço** | ~$2.37 | ~$0.30 + $0.15 | ✅ Muito melhor |
| **Tipo** | Extended ($3) | **Preferred ($0)** | ✅ |

**Cristal necessário:** 32.768kHz (C32346) - **Basic** ($0 taxa)

### U2, U7: Level Shifter ✅ APROVADO

| Parâmetro | KiCad (Footprint) | BOM Real | Status |
|-----------|-------------------|----------|--------|
| **Componente** | SN74LVC245APW | **SN74AHCT245PWR** | ✅ Pin-compatible |
| **LCSC** | - | **C10910** | ✅ |
| **Família** | LVC | **AHCT** | ✅ AHCT é o pretendido |
| **Package** | TSSOP-20 | TSSOP-20 | ✅ Mesmo footprint |
| **Pinout** | Standard 74x245 | Standard 74x245 | ✅ Idêntico |

**Nota:** No KiCad usa-se SN74LVC245APW apenas por conveniência (footprint disponível).
O componente real a encomendar é o **SN74AHCT245PWR (C10910)** - 100% pin-compatible.

**Porquê AHCT?**
- Ideal para **level shifting ESP32 (3.3V) → LED Matrix (5V)**
- Inputs TTL-compatible reconhecem 3.3V como HIGH
- Mesmo chip usado no **Adafruit MatrixPortal S3**
- Opera a 5V no lado da matriz LED

### U6: ME6211 → AMS1117-3.3 ✅ APROVADO

| Parâmetro | ME6211 (Original) | AMS1117-3.3 (Novo) | Status |
|-----------|-------------------|-------------------|--------|
| **LCSC** | C82942 | **C6186** | ✅ |
| **Package** | SOT-23-5 | **SOT-223** | Novo footprint |
| **Vout** | 3.3V | 3.3V | ✅ |
| **Iout** | 600mA | 1A | ✅ Melhor |
| **Dropout** | 120mV | 1.1V | ⚠️ Maior |
| **Iq** | 60µA | 5mA | ⚠️ Maior |
| **Tipo** | Extended | **Basic** | ✅ |

**Nota:** AMS1117 é o LDO standard dos ESP32 DevKit. Dropout maior mas aceitável com USB 5V input.

### D4: Battery Backup Diode ✅ NOVO

| Parâmetro | Valor | Status |
|-----------|-------|--------|
| **Componente** | MMBD4148SE | ✅ |
| **LCSC** | C17179590 | ✅ (mesmo que D1) |
| **Package** | SOT-23 | ✅ |
| **Função** | OR díodo para backup RTC | ✅ |
| **Tipo** | Basic | ✅ |

**Circuito:** VDD (3.3V) e BT1+ ligam ao VDD do PCF8563T, com D4 no caminho da bateria.
Quando USB desliga, bateria alimenta RTC através de D4 (~2.3V após drop).

---

## Análise de Classificação por Componente

### Legenda
- **Basic** = Sem taxa adicional (já nas máquinas P&P)
- **Preferred** = Extended mas **sem taxa** no Economic PCBA (ex-Basic, agora carregado manualmente)
- **Extended** = +$3 taxa de carregamento por tipo único

---

## Problemas Críticos Detetados

| Ref | Problema | Ação Necessária |
|-----|----------|-----------------|
| **C3** | Footprint 0805, componente é 1206 | Mudar footprint para `C_1206_3216Metric` |
| **D2** | Footprint 0402, componente é 0603 | Mudar footprint para `LED_0603_1608Metric` |
| **J1** | LCSC errado (Hanxia genérico) | Mudar para **C3020560** (GCT USB4105-GF-A) |

> BZ1 Resolvido: Substituído GPC12075YB-5V (C252948, fora de stock) por TMB12A05 (C96093) - mesmo footprint Ø12mm, pitch 7.6mm

---

## Lista Completa de Componentes

### Microcontrolador

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| U3 | ESP32-WROOM-32E-N8 | **C701342** | Extended | - | MCU principal, sem alternativa |

**Custo Extended:** +$3

### ICs

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| U4 | CH340C | **C84681** | Extended | USB-Serial |
| U8 | AMS1117-3.3 (LDO) | **C6186** | Basic | SOT-223, 3.3V regulador |

**Custo Extended:** +$3

> **U5** (anteriormente D3V3XA4B10LP-7 ESD) → Substituído por **CH224K** (PD Sink). Ver secção PSU.

### Transistores

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| Q1 | MMBT2222A | **C8512** | Basic | Buzzer driver (sound.kicad_sch) |
| Q4 | MMBT2222A | **C8512** | Basic | UART DTR (uart.kicad_sch) |
| Q5 | MMBT2222A | **C8512** | Basic | UART RTS (uart.kicad_sch) |

**Custo Extended:** $0 — Todos Basic!

> **Q2** (MMBT2222A, Error LED NPN) → Ver secção PSU.
> **Q1** alterado: UMH3N (C62892) → **MMBT2222A (C8512)** — simplificação do driver do buzzer.

### Díodos e LEDs

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| D2 | KT-0603R (LED Wifi Status) | **C2286** | Basic | clockv7.kicad_sch |
| D4 | MMBD4148SE (RTC backup) | **C17179590** | Basic | RTC.kicad_sch |
| D5 | MMBD4148SE (buzzer flyback) | **C17179590** | Basic | sound.kicad_sch |

**Custo Extended:** $0 — Todos Basic!

> **D1** (anteriormente 1N4148TR) → Agora **LED_ERR** (C2286). Ver secção PSU.
> **D3** (anteriormente SMF9.0CA) → Agora **H7VN10B** (C20615788, USB data TVS). Ver secção PSU.
> **D6** (SMBJ24CA, VBUS TVS 24V) → Ver secção PSU.

### Condensadores (Não-PSU)

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| C2 | 10µF 0603 | **C19702** | Basic | ESP32 bypass (clockv7) |
| C3 | 22µF 1206 | **C12891** | Basic | LDO output (clockv7) |
| C4,C6,C8,C11,C12,C13,C18,C19 | 100nF 0402 | **C307331** | Basic | Bypass caps (vários) |
| C5 | 22µF | **C59461** | Basic | Level shifter (clockv7) |

**Custo Extended:** $0 — Todos Basic!

> Condensadores PSU (C24, C25, C_OUT5-8, C_BOOT3, C_FIL1, C_FF2, C14, C26, C27) → Ver secção PSU.

### Resistências

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| R1 | 330Ω 0402 | **C25104** | Basic | LED D2 (clockv7) |
| R2 | 100Ω 0402 | **C25076** | Basic | USB (clockv7) |
| R5 | 10kΩ 0402 | **C25744** | Basic | ESP32 pull-up (clockv7) |
| R6 | 1kΩ 1206 | **C4410** | Basic | Buzzer (sound) |
| R10 | 10kΩ 0402 | **C25744** | Basic | UART DTR (uart) |
| R11 | 10kΩ 0402 | **C25744** | Basic | UART RTS (uart) |

**Custo Extended:** $0 — Todos Basic!

> **R3,R4** (5.1kΩ CC pull-downs) → **REMOVIDOS**. CH224K tem Rd internos 5.1kΩ.
> Resistências PSU (R14, R_FB3, R_FB4, R_PU1, R_BASE1, R_ERR1, R_DIV1, R_DIV2, R_VBUS1) → Ver secção PSU.

### Fusível

> **F1** (anteriormente SMD1206P050TF/15 500mA) → Substituído por **ASMD2920-300-30V 3A** (C2982291). Ver secção PSU.

### Conectores

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| J2 | 2.54-2*8P Header (GPIO +5V) | **C30734** | Basic | clockv7.kicad_sch |

**Custo Extended:** $0

> **USBC1** (TYPE-C-31-M-12, C165948) → Ver secção PSU.
> Conectores PSU adicionais (J3, J4, J7, P1, CN1) → Ver secção PSU / KiCad.

### Buzzer

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| BZ1 | TMB12A05 | **C96093** | Extended | - | Sem alternativa Basic THT |

**Custo Extended:** +$3

### Botões

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| SW1,SW2 | TS-1088-AR02016 | **C720477** | Extended | TS-1187A-B-A-B (C318884) | Verificar footprint |

**Custo Extended:** +$3

### RTC (Real-Time Clock)

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| U2 | DS3231SN | **C722469** | Extended | **PCF8563T (C7563) - Preferred!** | Sem taxa, menor precisão, muito mais barato |
| R7,R8 | 4.7kΩ 0402 | **C25900** | Basic | - | Já é Basic |
| C9 | 100nF 0402 | **C307331** | Basic | - | Já é Basic |
| BT1 | CR2032 Holder | **C70377** | Extended | - | Sem alternativa |

**Custo Extended:** +$6

**Análise RTC:** O **PCF8563T (C7563)** é **Preferred Extended** + Cristal **C32346 é Basic** = SEM TAXAS!

| Característica | DS3231SN | PCF8563T + Cristal |
|----------------|----------|-------------------|
| Componente | ~$2.37 | ~$0.30 + $0.15 = **$0.45** |
| Cristal externo | Não precisa (TCXO) | **C32346** ($0.15, **Basic!**) |
| Precisão | ±2ppm (~1 min/ano) | ±20ppm (~10 min/ano) |
| Taxa JLCPCB | $3 (Extended) | **$0** (Preferred + Basic) |
| **Total/lote** | **$5.37** | **$0.45** |
| **Poupança** | - | **~$5/lote** |

**RECOMENDAÇÃO FORTE:**
- PCF8563T (Preferred) + Cristal C32346 (Basic) = **$0 de taxas!**
- Poupança: ~$5/lote = **$250 em 50 lotes**
- Único contra: footprint diferente (SOIC-8 vs SOIC-16W)

---

## Resumo de Custos Extended (v4.1 atualizado)

### Componentes Não-PSU Extended

| Categoria | Componentes Extended | Custo |
|-----------|---------------------|-------|
| Microcontrolador | ESP32 (U3) | $3 |
| ICs | CH340C (U4) | $3 |
| Buzzer | TMB12A05 (BZ1) | $3 |
| Botões | TS-1088 (SW1,SW2) | $3 |
| RTC | Battery Holder (BT1) | $3 |
| **Subtotal Não-PSU** | **5 tipos** | **$15** |

### Componentes PSU Extended

| Categoria | Componentes Extended | Custo |
|-----------|---------------------|-------|
| PSU | CH224K, SY8388ARHC, L2, D6 TVS, F1 PTC | $15 |

### **TOTAL Extended: 10 tipos = $30**

> **Poupanças já aplicadas vs v3.0:** DS3231→PCF8563 ($5), ME6211→AMS1117 ($3), UMH3N→MMBT2222A ($3), ESD IC removido ($3) = **~$14 poupados**

---

## Notas RTC (PCF8563T)

### Pinout SOIC-8
```
        ┌────────────┐
 OSCI  1│●           │8  VDD (3.3V)
 OSCO  2│  PCF8563T  │7  CLKOUT (NC)
  INT  3│            │6  SCL
  VSS  4│            │5  SDA
        └────────────┘
```

### Ligações
- **Pin 1 (OSCI)** ↔ **Pin 2 (OSCO)**: Cristal Y1 (C32346) - sem caps externos
- **Pin 3 (INT)**: Deixar NC (só usar para alarmes)
- **Pin 7 (CLKOUT)**: Deixar NC
- **Pin 5 (SDA)**: GPIO21 ESP32 + pull-up 4.7kΩ
- **Pin 6 (SCL)**: GPIO22 ESP32 + pull-up 4.7kΩ
- **Pin 8 (VDD)**: 3.3V + bypass 100nF + D4 para backup bateria

### Circuito Backup Bateria
```
VDD (3.3V) ─────────────────┬─── Pin 8 (VDD) PCF8563T
                            │
BT1+ (CR2032) ───►|─────────┘
                  D4
             (MMBD4148SE)

BT1- ─────────────────────────── Pin 4 (VSS/GND)
```

### Funcionamento D4
| Estado | Resultado |
|--------|-----------|
| USB ON (VDD=3.3V) | D4 reverse bias → bateria não gasta |
| USB OFF (VDD=0V) | D4 forward → RTC recebe ~2.3V da bateria |

---

## Referência de Dimensões de Packages

| Package | Dimensões (mm) | Nome Métrico |
|---------|---------------|--------------|
| 0201 | 0.6 x 0.3 | 0603 Metric |
| 0402 | 1.0 x 0.5 | 1005 Metric |
| 0603 | 1.6 x 0.8 | 1608 Metric |
| 0805 | 2.0 x 1.2 | 2012 Metric |
| 1206 | 3.2 x 1.6 | 3216 Metric |
| 1210 | 3.2 x 2.5 | 3225 Metric |
| 1812 | 4.5 x 3.2 | 4532 Metric |
| SOT-23 | 2.9 x 1.3 | - |
| SOT-23-5 | 2.9 x 1.6 | - |
| SOT-363 | 2.0 x 1.25 | SC-70-6 |
| SOD-123 | 2.7 x 1.2 | - |
| SMB | 5.3 x 3.6 | DO-214AA |

---

## Ações Necessárias no KiCad

### 1. Corrigir C3 (Condensador 22µF)
```
Footprint atual: C_0805_2012Metric (ERRADO)
Footprint correto: C_1206_3216Metric
```

### 2. Corrigir D2 (LED Vermelho)
```
Footprint atual: LED_0402_1005Metric_Red (ERRADO)
Footprint correto: LED_0603_1608Metric
```

### 3. Corrigir J1 (USB-C)
```
LCSC atual: C49261569 (Hanxia genérico - não corresponde ao footprint)
LCSC correto: C3020560 (GCT USB4105-GF-A)
```

---

## Resumo de Códigos LCSC - v4.1

```
# ══════════════════════════════════════
# COMPONENTES NÃO-PSU (clockv7 + uart + RTC + sound)
# ══════════════════════════════════════

# Microcontrolador
U3  = C701342   (ESP32-WROOM-32E-N8)     [Extended]

# ICs
U4  = C84681    (CH340C)                  [Extended]
U8  = C6186     (AMS1117-3.3) SOT-223    [Basic]

# Level Shifters (KiCad: SN74LVC245APW footprint, encomendar AHCT)
U2,U7 = C10910  (SN74AHCT245PWR)         [Extended]

# Transistores
Q1  = C8512     (MMBT2222A) — buzzer     [Basic]
Q4  = C8512     (MMBT2222A) — UART DTR   [Basic]
Q5  = C8512     (MMBT2222A) — UART RTS   [Basic]

# Díodos
D2  = C2286     (KT-0603R LED Wifi)      [Basic]
D4  = C17179590 (MMBD4148SE RTC backup)  [Basic]
D5  = C17179590 (MMBD4148SE buzzer)      [Basic]

# Condensadores
C2  = C19702    (10µF 0603)              [Basic]
C3  = C12891    (22µF 1206, LDO out)     [Basic]
C4,C6,C8,C11,C12,C13,C18,C19 = C307331 (100nF 0402) [Basic]
C5  = C59461    (22µF, level shifter)    [Basic]

# Resistências
R1  = C25104    (330Ω LED)               [Basic]
R2  = C25076    (100Ω USB)               [Basic]
R5  = C25744    (10kΩ ESP32)             [Basic]
R6  = C4410     (1kΩ 1206 buzzer)        [Basic]
R10 = C25744    (10kΩ UART DTR)          [Basic]
R11 = C25744    (10kΩ UART RTS)          [Basic]

# Conectores
J2  = C30734    (2x8 Header GPIO)        [Basic]

# Buzzer
BZ1 = C252948   (TMB12A05)               [Extended]

# Botões
SW1,SW2 = C720477 (TS-1088)              [Extended]

# RTC Module
U6  = C722469   (PCF8563T) SOIC-8        [Preferred]
Y1  = C32346    (32.768kHz Crystal)      [Basic]
R7,R8 = C25900  (4.7kΩ I2C pull-up)      [Basic]
C1  = C307331   (100nF RTC bypass)       [Basic]
C9  = C12891    (22µF RTC bulk)          [Basic]
BT1 = C70377    (CR2032 Holder)          [Extended]

# ══════════════════════════════════════
# COMPONENTES PSU — Ver secção PSU v4.1
# ══════════════════════════════════════
# U5  = C970725   (CH224K PD Sink)        [Extended]
# U1  = C5110279  (SY8388ARHC Buck 8A)    [Extended]
# USBC1 = C165948 (TYPE-C-31-M-12)        [Extended]
# D6  = C19077558 (SMBJ24CA TVS 24V)      [Extended]
# D3  = C20615788 (H7VN10B D+/D- TVS)     [Extended] ← DNP, não montado
# D1  = C2286     (LED_ERR)               [Basic]
# F1  = C2982291  (PTC 3A 2920)           [Extended]
# L2  = C780205   (SRP1265A-4R7M 4.7µH)   [Extended]
# Q2  = C8512     (MMBT2222A Error LED)    [Basic]
# + Caps, resistors — ver POWER_SUPPLY.md secção 6
```

---

## Conclusão: Estado Atual (v4.1)

### Componentes Extended Finais (10 tipos = $30)

| # | Componente | Ref | Secção |
|---|-----------|-----|--------|
| 1 | ESP32-WROOM-32E | U3 | Main |
| 2 | CH340C | U4 | UART |
| 3 | SN74AHCT245PWR ×2 | U2,U7 | Main |
| 4 | TMB12A05 (Buzzer) | BZ1 | Sound |
| 5 | TS-1088 (Botões) ×2 | SW1,SW2 | Main |
| 6 | CR2032 Holder | BT1 | RTC |
| 7 | CH224K (PD Sink) | U5 | PSU |
| 8 | SY8388ARHC (Buck) | U1 | PSU |
| 9 | SMBJ24CA (TVS) | D6 | PSU |
| 10 | ASMD2920 (PTC Fuse) | F1 | PSU |

> **Nota:** L2 (indutor) é também Extended (+$3), totalizando **11 tipos = $33**. D3 (H7VN10B, TVS D+/D−) está **DNP** — não montado nesta revisão, não conta para a taxa Extended.

**Custo total estimado por lote de 5 PCBs:**
- Componentes: ~$15-20
- PCB: ~$5-10
- Assembly: ~$15-20
- Taxa Extended: ~$36 (12 tipos)
- **Total: ~$75-85** (ou ~$15-17 por placa)

---

## PSU Components (Secção PSU) — Design Final v4.1 (Bulletproof)

Componentes específicos da secção de alimentação. BOM detalhado em `POWER_SUPPLY.md`.

### PD Sink Controller - CH224K (substitui IP2721 + AO3404A MOSFET)

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| U5 | **CH224K** (WCH) - PD 3.0 Sink | **C970725** | Extended | ESSOP-10, pede 20V (CFG1 NC), PG pin, Rd interno |
| R14 | 1kΩ (VBUS→VDD) | C4410 | **Basic** | Alimentação CH224K |
| C14 | 1µF 50V (0603) | C15849 | **Basic** | Bypass VDD |

**Porquê CH224K em vez de IP2721 (C603176)?**
- IP2721 só suporta 3 tensões: 5V, 15V, 20V (sem 9V e 12V!)
- CH224K suporta **todas as 5 tensões PD**: 5V, 9V, 12V, 15V, 20V
- CH224K tem **PG (Power Good)** open-drain — LED de status hardware
- **Sem MOSFET externo** (Q3 removido) — VBUS liga directo ao buck
- **Rd internos 5.1kΩ** — não precisa R externas nos CC1/CC2
- Mais barato: ~$0.32 vs ~$0.40 (IP2721) + $0.03 (MOSFET) + $0.02 (R's)

### Buck Converter - SY8388ARHC (substitui TPS56838)

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| U1 | **SY8388ARHC** (Silergy) - Buck 8A | **C5110279** | Extended | QFN-16-EP 2.5×2.5mm, 24V in, 600kHz, compensação interna |
| L2 | Bourns SRP1265A-4R7M (4.7µH 16A) | C780205 | Extended | Shielded, Zone Keepout obrigatório! |
| C24,C25 | 22µF 25V MLCC (1206) | C12891 | **Basic** | Input caps |
| C_OUT5-8 | 22µF 25V MLCC ×4 (1206) | C12891 | **Basic** | Output caps (88µF total, baixo ESR) |
| C_BOOT3 | 100nF 50V (0402) | C307331 | **Basic** | Bootstrap |
| C_FIL1 | 100nF 50V (0402) | C307331 | **Basic** | HF bypass VIN→PGND |
| C_FF2 | 22pF 50V (0402) | C1555 | **Basic** | Feedforward (// R_FB3) |
| R_FB3 | 22kΩ 1% (0603) | C31850 | **Basic** | Feedback upper |
| R_FB4 | 3kΩ 1% (0603) | C4211 | **Basic** | Feedback lower |

**Porquê SY8388ARHC em vez de TPS56838 (C37533416)?**
- **Custo**: ~$0.53 vs ~$1.00+ (quase metade do preço!)
- **Sem coil whine**: PWM contínuo a 600kHz (vs SY8368 que fazia PFM a light load)
- **Compensação interna**: sem rede RC externa (simplifica layout)
- **QFN-16 compacto**: 2.5×2.5mm com thermal pad generoso
- 8A contínuo, 24V max — mesmas specs funcionais que TPS56838

### Proteção e Error LED

| Ref | Componente | LCSC | Tipo | Nota |
|-----|------------|------|------|------|
| D6 | SMBJ24CA (TVS bidirecional) | C19077558 | Extended | Vrwm=24V, 600W pk, SMB pkg — proteção VBUS 20V |
| F1 | PTC Fuse 3A/30V | C2982291 | Extended | Protecção overcurrent |
| C26 | 10µF 50V (1206) | C13585 | **Basic** | Filtro entrada VBUS |
| Q2 | MMBT2222A (NPN) | C8512 | **Basic** | Inverter para Error LED |
| D1 | LED Vermelho 0603 | C2286 | **Basic** | Error LED (sem PD) |
| R_PU1 | 10kΩ (PG pull-up) | C25744 | **Basic** | Pull-up PG |
| R_BASE1 | 10kΩ (NPN base) | C25744 | **Basic** | Base NPN |
| R_ERR1 | 330Ω (LED) | C25104 | **Basic** | Corrente LED |

### Resumo Custos PSU

| Bloco | Taxa Extended | Nota |
|-------|---------------|------|
| CH224K | $3 | PD Sink |
| SY8388ARHC | $3 | Buck |
| L2 (Bourns) | $3 | Indutor |
| D6 (TVS) | $3 | Proteção |
| F1 (PTC) | $3 | Fuse |
| **Total PSU Extended** | **$15** | 5 tipos Extended |

---

*Documento atualizado: Março 2026*
*Análise de custos Basic/Extended incluída*
*PSU v4.1 Bulletproof: CH224K (PD 20V) + SY8388ARHC (Buck 8A) — worst-case analysis, caps/TVS aligned with KiCad*
