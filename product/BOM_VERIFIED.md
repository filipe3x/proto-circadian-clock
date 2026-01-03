# BOM Verificado - Circadian Clock PCB

**Data:** Janeiro 2026
**Fonte:** JLCPCB BOM Detection
**Total:** 24 componentes

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
| **ESP32-WROOM-32E** | MCU principal, sem alternativa |
| **CH340C** | USB-Serial, AMS1117 é Basic mas CH340C é necessário |
| **DS3231SN** | RTC de precisão com TCXO integrado |
| **USB4105-GF-A** | Conector USB-C específico para o footprint |

### Componentes Extended Substituíveis

| Atual (Extended) | Alternativa | Economia | Nota |
|------------------|-------------|----------|------|
| DS3231SN (Extended) | **PCF8563T (Preferred)** | **$3** | Sem taxa! Mesmo pinout SOIC-8 |
| ME6211C33M5G-N (SOT-23-5) | AMS1117-3.3 (SOT-223) | $3 | Requer mudança de footprint |
| TMB12A05 (THT) | - | - | Sem alternativa Basic THT |

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

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| U4 | CH340C | **C84681** | Extended | - | USB-Serial necessário |
| U5 | D3V3XA4B10LP-7 (ESD) | **C1980462** | Extended | USBLC6-2SC6 (C7519) | Mesmo footprint UDFN |
| U6 | ME6211C33M5G-N (LDO) | **C82942** | Extended | AMS1117-3.3 (C6186) | Requer mudar para SOT-223 |

**Custo Extended:** +$9 (ou +$6 se substituir LDO)

**Recomendação U6:** O ME6211 tem dropout muito baixo (120mV) e quiescent current de 60µA, ideal para bateria. O AMS1117 tem dropout de 1.1V e 5mA quiescent - **manter ME6211** se eficiência for importante.

### Transistores

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| Q1 | UMH3N | **C62892** | Extended | 2N7002 + resistências discretas | Complexifica o design |
| Q2 | MMBT2222A | **C916372** | Basic | - | Já é Basic! |

**Custo Extended:** +$3

### Díodos e LEDs

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| D1 | 1N4148TR | **C84410** | Basic | - | Já é Basic |
| D2 | KT-0603R (LED) | **C2286** | Basic | - | Já é Basic |
| D3 | SMF9.0CA (TVS) | **C123799** | Extended | SMBJ5.0A (C35597) | Verificar specs |

**Custo Extended:** +$3

### Condensadores

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| C3 | CL31A226KAHNNNE (22µF 1206) | **C12891** | Basic | - | Já é Basic |
| C4,C6,C8 | CL05B104KB54PNC (100nF 0402) | **C307331** | Basic | - | Já é Basic |
| C5 | CL10A106KP8NNNC (10µF 0603) | **C19702** | Basic | - | Já é Basic |

**Custo Extended:** $0 - Todos Basic!

### Resistências

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| R1 | 330Ω 0402 | **C25104** | Basic | - | Já é Basic |
| R2 | 100Ω 0402 | **C106232** | Basic | - | Já é Basic |
| R3,R4 | 5.1kΩ 0402 | **C25905** | Basic | - | Já é Basic |
| R5 | 10kΩ 0402 | **C25744** | Basic | - | Já é Basic |
| R6 | 1kΩ 0402 | **C106235** | Basic | - | Já é Basic |

**Custo Extended:** $0 - Todos Basic!

### Fusível

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| F1 | SMD1206P050TF/15 (500mA) | **C106264** | Basic | - | Já é Basic |

**Custo Extended:** $0

### Conectores

| Ref | Componente | LCSC | Tipo | Alternativa Basic | Nota |
|-----|------------|------|------|-------------------|------|
| J1 | USB4105-GF-A | **C3020560** | Extended | TYPE-C-31-M-12 (C165948) | Verificar pinout |
| J2 | 2.54-2*8P Header | **C68234** | Basic | - | Já é Basic |
| J3 | KF301-5.0-2P | **C474881** | Extended | - | Terminal block THT |

**Custo Extended:** +$6

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

**Recomendação RTC:** O **PCF8563T (C7563)** é **Preferred Extended** - sem taxa de carregamento no Economic PCBA!

| Característica | DS3231SN | PCF8563T |
|----------------|----------|----------|
| Preço | ~$2.37 | ~$0.30 |
| Precisão | ±2ppm (~1 min/ano) | ±20ppm (~10 min/ano) |
| TCXO integrado | Sim | Não |
| Taxa JLCPCB | $3 (Extended) | **$0 (Preferred)** |
| Package | SOIC-16W | **SOIC-8** (diferente!) |

**ATENÇÃO:** O PCF8563T usa package SOIC-8, diferente do DS3231SN (SOIC-16W). Requer mudança de footprint no KiCad se substituir.

---

## Resumo de Custos Extended

| Categoria | Componentes Extended | Custo |
|-----------|---------------------|-------|
| Microcontrolador | ESP32 | $3 |
| ICs | CH340C, ESD, LDO | $9 |
| Transistores | UMH3N | $3 |
| Díodos | TVS | $3 |
| Conectores | USB-C, Terminal | $6 |
| Buzzer | TMB12A05 | $3 |
| Botões | TS-1088 | $3 |
| RTC | DS3231, Battery Holder | $6 |
| **TOTAL** | **11 tipos Extended** | **$33** |

### Potencial de Poupança

Se substituirmos componentes Extended por Basic onde viável:

| Substituição | Poupança | Viabilidade | Nota |
|--------------|----------|-------------|------|
| **DS3231 → PCF8563** | **$3 + $2** | **Alta** | Preferred Extended + componente mais barato! |
| ME6211 → AMS1117 | $3 | Média | Mudar footprint SOT-23-5 → SOT-223 |
| UMH3N → discreto | $3 | Baixa | Complexifica o design |
| TVS → Basic | $3 | Média | Verificar specs de proteção |

**Poupança máxima realista:** ~$8-11 por lote (DS3231→PCF8563 é a melhor opção!)

---

## Notas RTC
- **~RST (pin 4)**: Pull-up interno 50kΩ → deixar NC
- **INT/SQW (pin 3)**: Deixar NC (só usar para alarmes)
- **Cristal**: TCXO integrado (32.768kHz) → não adicionar externo
- **I2C**: GPIO21 (SDA), GPIO22 (SCL) no ESP32 Dev Module

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

## Resumo de Códigos LCSC

```
# Microcontrolador
U3  = C701342   (ESP32-WROOM-32E-N8)     [Extended]

# ICs
U4  = C84681    (CH340C)                  [Extended]
U5  = C1980462  (D3V3XA4B10LP-7)         [Extended]
U6  = C82942    (ME6211C33M5G-N)         [Extended]

# Transistores
Q1  = C62892    (UMH3N)                   [Extended]
Q2  = C916372   (MMBT2222A)              [Basic]

# Díodos
D1  = C84410    (1N4148TR)               [Basic]
D2  = C2286     (KT-0603R)               [Basic] ← Footprint 0603!
D3  = C123799   (SMF9.0CA)               [Extended]

# Condensadores - TODOS BASIC!
C3  = C12891    (22µF 1206)              [Basic] ← Footprint 1206!
C4,C6,C8 = C307331 (100nF 0402)          [Basic]
C5  = C19702    (10µF 0603)              [Basic]

# Resistências - TODAS BASIC!
R1  = C25104    (330Ω)                   [Basic]
R2  = C106232   (100Ω)                   [Basic]
R3,R4 = C25905  (5.1kΩ)                  [Basic]
R5  = C25744    (10kΩ)                   [Basic]
R6  = C106235   (1kΩ)                    [Basic]

# Outros
F1  = C106264   (PTC 500mA 1206)         [Basic]
J1  = C3020560  (USB4105-GF-A)           [Extended] ← CORRIGIDO!
J2  = C68234    (2x8 Header)             [Basic]
J3  = C474881   (KF301-5.0-2P)           [Extended]
BZ1 = C96093    (TMB12A05)               [Extended]
SW1,SW2 = C720477 (TS-1088)              [Extended]

# RTC Module
U2  = C722469   (DS3231SN)               [Extended]
R7,R8 = C25900  (4.7kΩ I2C pull-up)      [Basic]
C9  = C307331   (100nF RTC bypass)       [Basic]
BT1 = C70377    (CR2032 Holder)          [Extended]
```

---

## Conclusão: Estratégia de Custos

Para a **segunda fase de desenvolvimento**, recomendo:

### Ação Imediata Recomendada

**Substituir DS3231SN por PCF8563T:**
- Poupança: **$3** (taxa) + **~$2** (componente) = **$5/lote**
- PCF8563T é **Preferred Extended** = sem taxa de carregamento!
- Precisão de ±20ppm é suficiente para relógio circadiano (~10 min/ano)
- **NOTA:** Requer mudança de footprint (SOIC-16W → SOIC-8)

### Componentes Extended Inevitáveis
ESP32, CH340C, USB-C - são o core do projeto, manter.

### Resumo de Custos

| Cenário | Taxa Extended | Nota |
|---------|---------------|------|
| BOM atual (com DS3231) | $33 | 11 Extended |
| Com PCF8563 (Preferred) | **$30** | 10 Extended |

**Custo total estimado por lote de 5 PCBs:**
- Componentes: ~$13-18 (com PCF8563)
- PCB: ~$5-10
- Assembly: ~$15-20
- Taxa Extended: ~$30
- **Total: ~$65-80** (ou ~$13-16 por placa)

---

*Documento atualizado: Janeiro 2026*
*Análise de custos Basic/Extended incluída*
