# Power Supply - Circadian Clock

Este documento descreve a arquitetura de alimentação unificada para o Circadian Clock, utilizando uma única fonte 5V para alimentar o painel LED P10 e o ESP32.

---

## 1. Requisitos de Energia

### 1.1 Consumo por Componente

| Componente | Tensão | Corrente Típica | Corrente Máxima | Notas |
|------------|--------|-----------------|-----------------|-------|
| Painel P10 32x16 RGB | 5V | 1-2A | 4A | Depende do brilho e padrão |
| ESP32-WROOM-32E | 3.3V | 80-150mA | 500mA | WiFi ativo aumenta consumo |
| DS3231 RTC | 3.3V | 1mA | 3mA | Backup: bateria CR2032 |
| 74AHCT245 (x2) | 5V | 10mA | 20mA | Level shifters |
| LEDs indicadores (x3) | 3.3V | 30mA | 60mA | Power, WiFi, Error |
| **TOTAL** | 5V | **~2.5A** | **~5A** | |

### 1.2 Especificações da Fonte

| Parâmetro | Mínimo | Recomendado | Máximo |
|-----------|--------|-------------|--------|
| Tensão de saída | 4.75V | 5.0V | 5.25V |
| Corrente | 4A | 5A | 10A |
| Ripple | - | <50mV | 100mV |
| Regulação | - | ±2% | ±5% |

> **Recomendação:** Fonte 5V/5A com ficha DC 5.5x2.1mm, certificação CE/UL.

### 1.3 Pior Caso: Painel P10 a 100%

```
Consumo máximo do painel (todos os LEDs brancos a 100%):
═══════════════════════════════════════════════════════════════

  32 colunas × 16 linhas × 3 cores (RGB) = 1536 LEDs

  Cada LED: ~20mA (máximo)
  Duty cycle scan: 1/16 (apenas 1 linha ativa)

  Corrente instantânea: 32 × 3 × 20mA = 1.92A (por linha)
  Corrente média: 1.92A × 1/16 + overhead = ~150mA (mínimo)

  Na prática com PWM e cores mistas:
  - Brilho 25%: ~0.5A
  - Brilho 50%: ~1.0A
  - Brilho 75%: ~1.5A
  - Brilho 100%: ~2.0A

  Picos transitórios: até 4A

═══════════════════════════════════════════════════════════════
```

---

## 2. Arquitetura de Alimentação

### 2.1 Diagrama de Blocos

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        POWER SUPPLY ARCHITECTURE                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ENTRADA                PROTEÇÕES                  DISTRIBUIÇÃO            │
│   ═══════                ═════════                  ════════════            │
│                                                                             │
│                    ┌──────────────────────────────────────────┐            │
│  ┌──────────┐      │  ┌─────┐   ┌─────┐   ┌─────┐   ┌─────┐  │            │
│  │ FONTE    │      │  │ D1  │   │ F1  │   │ D2  │   │ C1  │  │            │
│  │ 5V/5A    │──────┼──┤Schot├───┤ PTC ├───┤ TVS ├───┤Cap  ├──┼─┬─► +5V_SAFE
│  │ DC 5.5mm │      │  │SS54 │   │2.5A │   │5.0A │   │100µF│  │ │          │
│  └──────────┘      │  └─────┘   └─────┘   └─────┘   └─────┘  │ │          │
│       │            │                                          │ │          │
│       │            │  Inversão  Curto     Sobre    Filtragem │ │          │
│       │            │  Polarid.  Circuito  Tensão              │ │          │
│       │            └──────────────────────────────────────────┘ │          │
│       │                                                          │          │
│       └──────────────────────────────────────────────────────────┴─► GND   │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   +5V_SAFE                                                                  │
│      │                                                                      │
│      ├────────────────────────────────────────────────► PAINEL P10 (5V)    │
│      │                                                   ~2-4A              │
│      │                                                                      │
│      ├───────────────────────────────────────────────► 74AHCT245 x2 (5V)   │
│      │                                                   ~20mA              │
│      │                                                                      │
│      │     ┌───────────────────────┐                                       │
│      └────►│    AMS1117-3.3V       │──────────────────► ESP32 (3.3V)       │
│            │    LDO Regulator      │                     ~150mA             │
│            │    (Max 1A output)    │                                        │
│            └───────────────────────┘                                        │
│                      │                                                      │
│                      └────────────────────────────────► DS3231 RTC (3.3V)  │
│                                                          ~1mA               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Fluxo de Proteção

```
Sequência de Proteção (Entrada → Carga):
═══════════════════════════════════════════════════════════════

 ENTRADA    │    PROTEÇÃO 1      │    PROTEÇÃO 2      │    PROTEÇÃO 3
 5V DC      │    Inversão        │    Curto-Circuito  │    Sobretensão
            │    Polaridade      │                    │
            │                    │                    │
    ───────►│◄─── D1 SS54 ──────►│◄─── F1 PTC ───────►│◄─── D2 TVS ──────►
            │                    │                    │
  Se        │  Díodo bloqueia    │  Fusível abre     │  TVS clampa a
  invertido │  corrente reversa  │  em >5A           │  ~9V e limita
            │                    │                    │
            │    PROTEÇÃO 4      │    PROTEÇÃO 5      │    SAÍDA
            │    Filtragem       │    ESD             │
            │                    │                    │
    ───────►│◄─── C1 100µF ─────►│◄─── SRV05 ────────►│─── +5V_SAFE ──────►
            │     C2 100nF       │    (I2C/USB)       │
            │                    │                    │
  Absorve   │  Filtra ruído      │  Protege           │  Tensão limpa
  picos     │  alta frequência   │  interfaces        │  e protegida

═══════════════════════════════════════════════════════════════
```

---

## 3. Circuito de Proteção Detalhado

### 3.1 Esquema Completo

```
                 ENTRADA                           PROTEÇÕES                              SAÍDA
═════════════════════════════════════════════════════════════════════════════════════════════════════

                              D1              F1              D2
    DC Jack 5.5x2.1mm      SS54/SS56      MF-MSMF250      SMBJ5.0A
         (+) ─────────────────┤>├─────────┤ PTC ├───────────┬─┤>├──┬──────────────── +5V_SAFE
                             │            │     │           │      │
                         Schottky      Fuse 2.5A          TVS     │
                         5A 40V       (trips @5A)         5.0V    │
                         Vf=0.3V                                  │
                                                                  │
                                                          C1     ═╧═    C2
                                                        100µF   GND   100nF
                                                         16V          X7R
                                                          │            │
                                                          │            │
         (-) ─────────────────────────────────────────────┴────────────┴──────────────── GND

═════════════════════════════════════════════════════════════════════════════════════════════════════

NOTAS:
- D1 (SS54): Proteção inversão polaridade, queda ~0.3V → 5V - 0.3V = 4.7V (aceitável)
- F1 (MF-MSMF250): Hold 2.5A, Trip 5.0A, auto-reseta após arrefecimento
- D2 (SMBJ5.0A): Standoff 5V, clampa a 9.2V, 600W pico
- C1: Absorve transientes, estabiliza tensão
- C2: Filtra ruído de alta frequência do switching

═════════════════════════════════════════════════════════════════════════════════════════════════════
```

### 3.2 Regulador 3.3V para ESP32

```
                   AMS1117-3.3 (U3)
                   ═════════════════

                     ┌─────────────┐
    +5V_SAFE ──┬────►│ VIN    VOUT │───┬────────────► +3.3V
               │     │             │   │
              ═╧═    │     GND     │  ═╧═
         C3  22µF    └──────┬──────┘  22µF  C4
             10V            │          10V
               │            │           │
    GND ───────┴────────────┴───────────┴────────────► GND


Especificações AMS1117-3.3:
- Tensão entrada: 4.5V - 12V
- Tensão saída: 3.3V ±2%
- Corrente máxima: 1A
- Dropout: 1.1V @ 1A
- Package: SOT-223

Margem de tensão:
- Entrada mínima: 3.3V + 1.1V = 4.4V
- Após D1 Schottky: 5V - 0.3V = 4.7V ✓
- Headroom: 4.7V - 4.4V = 0.3V (OK)
```

---

## 4. Componentes e BOM

### 4.1 Lista de Componentes de Proteção

| Ref | Componente | Especificação | Package | Qty | Preço | Fornecedor |
|-----|------------|---------------|---------|-----|-------|------------|
| D1 | SS54 ou SS56 | Schottky 5A 40V | SMC | 1 | €0.15 | LCSC, AliExpress |
| F1 | MF-MSMF250 | PTC Fuse 2.5A hold | 1206 | 1 | €0.20 | LCSC |
| D2 | SMBJ5.0A | TVS 5V 600W | SMB | 1 | €0.30 | LCSC |
| U3 | AMS1117-3.3 | LDO 3.3V 1A | SOT-223 | 1 | €0.10 | LCSC |
| C1 | 100µF 16V | Electrolítico | 8x10mm | 1 | €0.10 | LCSC |
| C2 | 100nF X7R | Cerâmico 16V | 0402 | 1 | €0.02 | LCSC |
| C3, C4 | 22µF X5R | Cerâmico 10V | 0805 | 2 | €0.05 | LCSC |
| J1 | DC Jack | 5.5x2.1mm | Through-hole | 1 | €0.20 | AliExpress |

**Total componentes proteção: ~€1.20**

### 4.2 Fonte de Alimentação Recomendada

| Opção | Especificações | Preço | Pesquisa AliExpress |
|-------|----------------|-------|---------------------|
| **Recomendada** | 5V 5A 25W, DC 5.5x2.1mm | ~€5-8 | `"5V 5A power adapter DC 5.5mm"` |
| Alternativa | 5V 4A 20W, DC 5.5x2.1mm | ~€4-6 | `"5V 4A power supply DC jack"` |
| Premium | 5V 6A 30W, Mean Well | ~€10-15 | `"Mean Well 5V 6A"` |

> **IMPORTANTE:** Escolher fonte com certificação CE/UL para segurança.

### 4.3 Pesquisas AliExpress - Componentes

| Componente | Pesquisa | Preço (pack) |
|------------|----------|--------------|
| Díodos SS54 | `"SS54 schottky diode SMC"` | ~€2 (50pcs) |
| Fusíveis PTC | `"PPTC fuse 2.5A 1206"` | ~€2 (20pcs) |
| TVS SMBJ5.0A | `"SMBJ5.0A TVS diode"` | ~€2 (20pcs) |
| AMS1117-3.3 | `"AMS1117-3.3 SOT-223"` | ~€1 (10pcs) |
| Capacitores 100µF | `"100uF 16V electrolytic capacitor"` | ~€1 (20pcs) |
| Capacitores 22µF | `"22uF ceramic capacitor 0805"` | ~€1 (50pcs) |
| DC Jack | `"DC power jack 5.5x2.1 PCB"` | ~€1 (10pcs) |

---

## 5. Alternativa: Alimentação via USB-C

Para desenvolvimento e testes, pode usar-se a porta USB-C do ESP32 com um adaptador.

### 5.1 Diagrama

```
                                         ┌─────────────┐
                                         │    ESP32    │
    Fonte 5V ───┬──[Proteções]──────────►│    Dev      │──► HUB75 ──► Painel
         │      │                        │   Module    │       │
         │      │                        │             │       │
         │      └──[Splitter DC]─────────┤ USB-C      │       │
         │                               └─────────────┘       │
         │                                                     │
         └─────────────────────────────────────────────────────┘
                            5V direto ao painel
```

### 5.2 Cabo Adaptador DC→USB-C

| Pesquisa AliExpress | Preço |
|---------------------|-------|
| `"DC 5.5x2.1 to USB C cable"` | ~€2 |
| `"DC barrel jack USB C adapter"` | ~€1-2 |

> **Nota:** Esta opção é mais simples mas adiciona pontos de falha. Recomendado apenas para prototipagem.

---

## 6. Diagrama de Ligações Final

```
═══════════════════════════════════════════════════════════════════════════════════════
                              CIRCADIAN CLOCK - POWER DISTRIBUTION
═══════════════════════════════════════════════════════════════════════════════════════

                    POWER ENTRY                          MAIN BOARD
                 ════════════════                    ═══════════════════

                                    PROTECTION CIRCUIT
                                 ┌──────────────────────────────────────────────┐
   ┌────────────┐                │                                              │
   │            │                │   D1        F1        D2                     │
   │  FONTE     │    DC JACK     │  SS54    PTC 2.5A   TVS 5V                   │
   │  5V / 5A   ├────[J1]────────┼───┤>├─────┤▒▒├──────┬─┤>├──┬─────────────────┼─────►
   │            │                │                      │     │                 │  +5V_SAFE
   │  25W       │                │                     ═╧═   ═╧═                │
   │            │                │               C1 100µF    C2 100nF           │
   └────────────┘                │                      │     │                 │
         ║                       │                      │     │                 │
         ║ GND ──────────────────┼──────────────────────┴─────┴─────────────────┼─────►
         ║                       │                                              │   GND
         ║                       └──────────────────────────────────────────────┘
         ║
         ║
═══════════════════════════════════════════════════════════════════════════════════════
                                    POWER DISTRIBUTION
═══════════════════════════════════════════════════════════════════════════════════════

              +5V_SAFE                                GND
                 │                                     │
                 │                                     │
    ┌────────────┼─────────────────────────────────────┼────────────────────┐
    │            │                                     │                    │
    │            ├───────────────────────────┐         │                    │
    │            │                           │         │                    │
    │       ┌────┴────┐                 ┌────┴────┐    │                    │
    │       │  U3     │                 │ 74AHCT  │    │                    │
    │       │AMS1117  │                 │  245    │────┼────────────────────┤
    │       │  3.3V   │                 │  x2     │    │                    │
    │       └────┬────┘                 └────┬────┘    │                    │
    │            │                           │         │                    │
    │            │ +3.3V                     │ 5V      │                    │
    │            │                           │ logic   │                    │
    │       ┌────┴────────────────┐          │         │                    │
    │       │                     │          │         │                    │
    │  ┌────┴────┐          ┌─────┴───┐  ┌───┴────┐    │                    │
    │  │  ESP32  │◄────────►│  DS3231 │  │ HUB75  │    │                    │
    │  │         │   I2C    │   RTC   │  │ Header │    │                    │
    │  │  ~150mA │          │   ~1mA  │  │        │    │                    │
    │  └─────────┘          └─────────┘  └────┬───┘    │                    │
    │                                         │        │                    │
    │                                         │        │                    │
    └─────────────────────────────────────────┼────────┘                    │
                                              │                             │
                                              │                             │
═══════════════════════════════════════════════════════════════════════════════════════
                                        PAINEL P10
═══════════════════════════════════════════════════════════════════════════════════════

                                    ┌─────────────────────────────────────┐
                                    │                                     │
    +5V_SAFE ───────────────────────┤  VCC                    P10 PANEL   │
                                    │                                     │
    GND ────────────────────────────┤  GND                    32x16 RGB   │
                                    │                                     │
    HUB75 Cable ────────────────────┤  DATA (R1,G1,B1,R2,G2,B2,          │
       (16 pins)                    │        A,B,C,D,CLK,LAT,OE)         │
                                    │                                     │
                                    │  Consumo: 1-4A (dependendo brilho) │
                                    │                                     │
                                    └─────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════════════════════
```

---

## 7. Cuidados de Segurança

### 7.1 Checklist Antes de Ligar

- [ ] Verificar polaridade da fonte (centro = positivo)
- [ ] Medir tensão da fonte com multímetro (4.75V - 5.25V)
- [ ] Inspecionar soldaduras (sem curtos-circuitos)
- [ ] Confirmar orientação de díodos (marca = cátodo)
- [ ] Testar circuito de proteção sem carga primeiro

### 7.2 Teste do Circuito de Proteção

```
Procedimento de Teste:
═══════════════════════════════════════════════════════════════

1. SEM CARGA - Ligar fonte ao circuito
   → Medir: +5V_SAFE deve ser ~4.7V (após queda do Schottky)

2. TESTE INVERSÃO - Inverter polaridade brevemente
   → Resultado: Nenhuma tensão na saída (D1 bloqueia)

3. TESTE CURTO - Com fonte a 1A max, curto-circuitar saída
   → Resultado: F1 aquece e corta em <1s
   → Esperar 30s, fusível reseta automaticamente

4. TESTE SOBRETENSÃO - Se possível, aplicar 7V momentâneo
   → Resultado: D2 clampa, saída não excede 6V

5. COM CARGA - Ligar ESP32 + Painel gradualmente
   → Verificar: tensões estáveis, sem aquecimento anormal

═══════════════════════════════════════════════════════════════
```

### 7.3 Riscos e Mitigações

| Risco | Consequência | Proteção | Componente |
|-------|--------------|----------|------------|
| Polaridade invertida | Curto-circuito, componentes queimados | Díodo Schottky bloqueia | D1 SS54 |
| Curto-circuito | Sobreaquecimento, fogo | Fusível PTC corta | F1 MF-MSMF250 |
| Sobretensão >6V | Danos ao ESP32 e painel | TVS clampa tensão | D2 SMBJ5.0A |
| Picos transitórios | Comportamento errático | Capacitores absorvem | C1, C2 |
| ESD (descarga estática) | Danos a interfaces | TVS array protege | SRV05-4 |
| Ruído da fonte | Flickering nos LEDs | Filtragem | C1 100µF + C2 100nF |

---

## 8. Sequência de Arranque

```
Timeline de Arranque:
═══════════════════════════════════════════════════════════════

  t=0ms     Liga fonte 5V
    │
    ├──────── C1 carrega (soft-start ~10ms)
    │
  t=10ms    +5V_SAFE estabiliza a 4.7V
    │
    ├──────── AMS1117 arranca, 3.3V disponível
    │
  t=50ms    ESP32 inicia boot
    │         │
    │         ├── Pico de corrente WiFi ~500mA
    │         │
    │         └── Painel P10 recebe dados HUB75
    │
  t=200ms   Painel mostra primeira frame
    │
  t=2000ms  Sistema estável, WiFi conectado
    │
    ▼
  OPERAÇÃO NORMAL

═══════════════════════════════════════════════════════════════

NOTAS:
- Fonte 5A suporta picos transitórios sem problemas
- Não é necessário sequenciamento especial
- Painel e ESP32 podem arrancar simultaneamente
```

---

## 9. FAQ - Perguntas Frequentes

### Porque usar 5V/5A se o consumo típico é ~2.5A?

Margem de segurança para:
- Picos transitórios do painel (até 4A)
- Pico WiFi do ESP32 (~500mA)
- Degradação da fonte ao longo do tempo
- Operação em ambientes quentes

### O díodo Schottky não desperdiça energia?

Perda = Vf × I = 0.3V × 2A = 0.6W

É aceitável para a proteção que oferece. Alternativa com P-MOSFET (Si2301) tem perda <0.1W mas é mais complexo.

### Posso usar fonte de telemóvel 5V/2A?

**Não recomendado.** Insuficiente para picos do painel. Mínimo 5V/4A.

### E se a fonte for 5.5V ou 6V?

O TVS (D2) protege até ~6V. Para fontes >6V, usar buck converter entre a fonte e o circuito de proteção.

---

## 10. Referências

- [ESP32 Power Design Guidelines](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/power-supply.html)
- [P10 LED Panel Specifications](https://www.aliexpress.com/item/32x16-P10-RGB-LED-Panel.html)
- [AMS1117 Datasheet](https://www.advanced-monolithic.com/pdf/ds1117.pdf)
- [SS54 Schottky Datasheet](https://www.vishay.com/docs/88751/ss54.pdf)
- [SMBJ5.0A TVS Datasheet](https://www.littelfuse.com/~/media/electronics/datasheets/tvs_diodes/smbj.pdf)

---

*Documento criado: Dezembro 2024*
*Última atualização: Dezembro 2024*
*Versão: 2.0 - Consolidado com PCB_DESIGN_GUIDE.md*
