# BOM Verificado - Circadian Clock PCB

**Data:** Janeiro 2026
**Versão:** 3.0 - Componentes Otimizados
**Fonte:** JLCPCB BOM Detection + Verificação Manual
**Total:** 24 componentes

---

## Histórico de Alterações

| Versão | Data | Alterações |
|--------|------|------------|
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
| **ESP32-WROOM-32E** | MCU principal, sem alternativa |
| **CH340C** | USB-Serial, AMS1117 é Basic mas CH340C é necessário |
| **DS3231SN** | RTC de precisão com TCXO integrado |
| **USB4105-GF-A** | Conector USB-C específico para o footprint |

### Componentes Extended Substituíveis

| Atual (Extended) | Alternativa | Economia | Nota |
|------------------|-------------|----------|------|
| DS3231SN (Extended) | **PCF8563T (Preferred) + Cristal (Basic)** | **~$5** | Ambos sem taxa! |
| ME6211C33M5G-N (SOT-23-5) | AMS1117-3.3 (SOT-223) | $3 | Requer mudança de footprint |
| TMB12A05 (THT) | - | - | Sem alternativa Basic THT |

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

### D3: TVS SMF9.0CA → C20615788 ⚠️ VERIFICAR

| Parâmetro | SMF9.0CA (Original) | C20615788 (Novo) | Status |
|-----------|---------------------|------------------|--------|
| **LCSC** | C123799 | C20615788 | ⚠️ Part não encontrado |
| **Package** | SMB | SOD-123 / DFN1006? | Conflito |

**AVISO:** O código LCSC C20615788 não foi encontrado na base de dados. Alternativas recomendadas:
- **C123799** - SMF9.0CA MDD (SOD-123FL) - Extended
- **C266723** - SMF9.0CA FMS (SOD-123FL) - Verificar tipo

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

### U2, U7: Level Shifter ⚠️ ATENÇÃO

| Parâmetro | Especificado | C10910 Real | Status |
|-----------|--------------|-------------|--------|
| **Componente** | SN74LVC245APW | **SN74AHCT245PWR** | ⚠️ Diferente! |
| **Família** | LVC (1.65-3.6V) | **AHCT (4.5-5.5V)** | Verificar |
| **Package** | TSSOP-20 | TSSOP-20 | ✅ |

**AVISO:** C10910 = SN74**AHCT**245PWR (família AHCT), NÃO SN74**LVC**245APW (família LVC).
- **LVC**: Opera 1.65V-3.6V, ideal para 3.3V
- **AHCT**: Opera 4.5V-5.5V com inputs TTL-compatíveis

Para ESP32 (3.3V) → 5V level shifting, ambos funcionam, mas verificar requisitos de tensão.

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
| **DS3231 → PCF8563 + Cristal** | **~$5** | **Alta** | PCF8563 Preferred + Cristal Basic = $0 taxas! |
| ME6211 → AMS1117 | $3 | Média | Mudar footprint SOT-23-5 → SOT-223 |
| UMH3N → discreto | $3 | Baixa | Complexifica o design |
| TVS → Basic | $3 | Média | Verificar specs de proteção |

**Poupança máxima realista:** ~$8-11 por lote

**Melhor oportunidade:** O cristal 32.768kHz (**C32346**) é **Basic** e o PCF8563T é **Preferred Extended** - ambos sem taxa de carregamento! Poupança de ~$5/lote.

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

## Resumo de Códigos LCSC - v3.0 OTIMIZADO

```
# Microcontrolador
U3  = C701342   (ESP32-WROOM-32E-N8)     [Extended]

# ICs
U4  = C84681    (CH340C)                  [Extended]
U5  = C1980462  (D3V3XA4B10LP-7)         [Extended]
U6  = C6186     (AMS1117-3.3) SOT-223    [Basic] ← ALTERADO v3.0

# Level Shifters
U2,U7 = C10910  (SN74AHCT245PWR)         [Verificar] ← NOVO v3.0

# Transistores
Q1  = C62892    (UMH3N)                   [Extended]
Q2  = C916372   (MMBT2222A)              [Basic]

# Díodos
D1  = C17179590 (MMBD4148SE) SOT-23      [Basic] ← ALTERADO v3.0
D2  = C2286     (KT-0603R)               [Basic]
D3  = C123799   (SMF9.0CA) SOD-123FL     [Extended] ← VERIFICAR C20615788

# Condensadores - TODOS BASIC!
C3  = C12891    (22µF 1206)              [Extended] ← CORRIGIDO!
C4,C6,C8 = C307331 (100nF 0402)          [Basic]
C5  = C19702    (10µF 0603)              [Basic]

# Resistências - TODAS BASIC!
R1  = C25104    (330Ω)                   [Basic]
R2  = C106232   (100Ω)                   [Basic]
R3,R4 = C25905  (5.1kΩ)                  [Basic]
R5  = C25744    (10kΩ)                   [Basic]
R6  = C4410     (1kΩ 1206)               [Basic] ← ALTERADO v3.0

# Outros
F1  = C106264   (PTC 500mA 1206)         [Basic]
J1  = C3020560  (USB4105-GF-A)           [Extended]
J2  = C68234    (2x8 Header)             [Basic]
J3  = C474881   (KF301-5.0-2P)           [Extended]
BZ1 = C96093    (TMB12A05)               [Extended]
SW1,SW2 = C720477 (TS-1088)              [Extended]

# RTC Module - OTIMIZADO v3.0
U1  = C7440     (PCF8563T) SOIC-8        [Preferred] ← ALTERADO v3.0
Y1  = C32346    (32.768kHz Crystal)      [Basic] ← NOVO v3.0
R7,R8 = C25900  (4.7kΩ I2C pull-up)      [Basic]
C9  = C307331   (100nF RTC bypass)       [Basic]
BT1 = C70377    (CR2032 Holder)          [Extended]
```

---

## Conclusão: Estratégia de Custos

Para a **segunda fase de desenvolvimento**, recomendo:

### Análise: DS3231 vs PCF8563

| Opção | Custo Componente | Taxa Extended | Total/lote | Complexidade |
|-------|------------------|---------------|------------|--------------|
| **DS3231SN** | $2.37 | $3 | **$5.37** | Simples (TCXO integrado) |
| **PCF8563T + Cristal** | $0.45 | **$0** | **$0.45** | +1 componente, footprint SOIC-8 |

**RECOMENDAÇÃO:** Substituir por **PCF8563T + Cristal C32346**:
- Poupança: **~$5/lote** ($2 componente + $3 taxa)
- Cristal C32346 é **Basic** (sem taxa!)
- PCF8563T é **Preferred Extended** (sem taxa!)
- Para 50 lotes: **$250 de poupança!**
- Único trabalho: mudar footprint SOIC-16W → SOIC-8

### Componentes Extended (após otimização)
ESP32, CH340C, USB-C, ESD, LDO, UMH3N, TVS, Terminal, Buzzer, Botões, Battery Holder

### Resumo de Custos

| Cenário | Taxa Extended | Nota |
|---------|---------------|------|
| BOM atual (DS3231) | $33 | 11 Extended |
| **Com PCF8563 + Cristal** | **$30** | **10 Extended** (-1 RTC) |

**Custo total estimado por lote de 5 PCBs:**
- Componentes: ~$13-17 (com PCF8563)
- PCB: ~$5-10
- Assembly: ~$15-20
- Taxa Extended: ~$30
- **Total: ~$65-75** (ou ~$13-15 por placa)

---

*Documento atualizado: Janeiro 2026*
*Análise de custos Basic/Extended incluída*
