# Competitive PCB Analysis - HUB75 Interface Controllers

## Sumário Executivo

Este documento analisa os principais projetos de PCB open-source para interface com painéis LED HUB75, comparando abordagens de design e identificando lacunas para a solução Proto Circadian Clock.

---

## 1. ESP32-Trinity (Brian Lough / WitnessMeNow)

**Repositório:** [github.com/witnessmenow/ESP32-Trinity](https://github.com/witnessmenow/ESP32-Trinity)

### 1.1 Visão Geral

O ESP32-Trinity é uma placa open-source certificada (OSHW) projetada especificamente para controlar painéis HUB75. É atualmente o projeto mais popular e bem documentado neste espaço.

### 1.2 Características Técnicas

| Aspecto | Implementação |
|---------|---------------|
| **MCU** | ESP32-WROOM (soldado na placa) |
| **Level Shifters** | **NÃO INCLUÍDOS** - ligação direta 3.3V |
| **Alimentação** | Barrel jack 5.5x2.1mm + USB-C |
| **Proteção Inversão** | Limitada (poly fuse interno USB) |
| **Proteção ESD** | Não especificada |
| **Proteção Sobretensão** | Não especificada |
| **Conector HUB75** | Header 2x8 standard |
| **Painéis Testados** | 64x32 P3, 64x64 P3 |
| **Software** | ESP32-HUB75-MatrixPanel-I2S-DMA |

### 1.3 Design de Alimentação

```
USB-C ──[Poly Fuse interno]──┬── 5V para ESP32
                             │
Barrel Jack 5V ──────────────┴── 5V para Painel HUB75
```

**Recomendação:** Fonte 5V 8A "laptop style" para cobrir cenários de iluminação máxima.

### 1.4 Ficheiros de Design (V1.2)

- BOM CSV (Bill of Materials)
- Schematic PDF/PNG
- Gerber files
- Interactive BOM (HTML)
- EasyEDA project backup
- Pick-and-Place files (PNP)

### 1.5 Pontos Fortes

- Open Source Hardware certificado
- Comunidade ativa (Discord, GitHub)
- Documentação extensa com exemplos
- Vem totalmente montado e testado
- Preço acessível (~$15-20 via Makerfabs)
- Compatível com biblioteca ESP32-HUB75-MatrixPanel-DMA

### 1.6 Limitações Identificadas

| Limitação | Impacto | Risco |
|-----------|---------|-------|
| **Sem level shifters** | Sinal 3.3V pode ser marginal para alguns painéis | Médio |
| **Proteções básicas** | Vulnerável a picos, inversão de polaridade | Alto |
| **Sem ESD protection** | Descargas podem danificar GPIO | Médio |
| **Sem RTC integrado** | Requer módulo externo para funcionamento offline | Baixo |
| **Sem sensor de luz** | Não ajusta brilho automaticamente | Baixo |

### 1.7 Gap para Proto Circadian Clock

```diff
- Level shifters 3.3V → 5V (74AHCT245)
- Proteção inversão polaridade (Schottky/P-MOSFET)
- Proteção curto-circuito (PTC fuse)
- Proteção ESD (SRV05-4, PESD5V0S1BA)
- Proteção sobretensão (TVS SMBJ5.0A)
- RTC DS3231 integrado com bateria CR2032
- Sensor de luz ambiente (opcional)
```

---

## 2. Adafruit MatrixPortal S3

**Repositório:** [github.com/adafruit/Adafruit-MatrixPortal-S3-PCB](https://github.com/adafruit/Adafruit-MatrixPortal-S3-PCB)

### 2.1 Visão Geral

Solução comercial de alta qualidade da Adafruit, usando o ESP32-S3 mais recente. Representa o "estado da arte" em termos de design profissional para interfaces HUB75.

### 2.2 Características Técnicas

| Aspecto | Implementação |
|---------|---------------|
| **MCU** | ESP32-S3 (8MB Flash, 2MB SRAM) |
| **Level Shifters** | **2x 74AHCT125** (3.3V → 5V) |
| **Alimentação** | USB-C + terminais parafuso M3 |
| **Acelerómetro** | LIS3DH integrado |
| **Sensor Luz** | Previsto no schematic (ALS-PT19), não incluído |
| **LED RGB** | NeoPixel integrado para status |
| **Botões** | 2 botões dedicados |
| **GPIO Breakout** | 6 GPIO + Reset + Boot + TX debug |
| **Analog Inputs** | 4 entradas analógicas (PWM/SPI/I2S) |

### 2.3 Level Shifter Design

```
ESP32-S3 (3.3V) ──► 74AHCT125 ──► HUB75 (5V)
                    (2 ICs)
```

**Nota Importante:** O MatrixPortal S3 usa **74AHCT125** (quad buffer) em vez de **74AHCT245** (octal transceiver). Ambos funcionam para level shifting unidirecional.

| IC | Canais | Vantagem |
|----|--------|----------|
| 74AHCT125 | 4 | Menor, independentes |
| 74AHCT245 | 8 | Menos ICs necessários |

### 2.4 Conectores HUB75

- Socket 2x10 que encaixa em portas HUB75 2x8
- Conector IDC 2x8 adicional para cabo ribbon
- Compatível com painéis 16x32 até 64x64
- **Address E line jumper** para painéis 64x64 (default: GPIO8)

### 2.5 Design de Alimentação

```
USB-C ──────────────┬── 5V + Dados (programação)
                    │
                    ├── LED verde (indicador 3.3V)
                    │
                    └── LED verde (indicador 5V)

Terminais M3 ───────┬── 5V entrada separada (painéis grandes)
                    │
                    └── GND
```

### 2.6 Ficheiros de Design

- Adafruit MatrixPortal S3.brd (EagleCAD layout)
- Adafruit MatrixPortal S3.sch (schematic)
- PrettyPins PDF/SVG (pinout diagrams)

### 2.7 Pontos Fortes

- **ESP32-S3 dual-core** (WiFi + display em paralelo)
- **Level shifters integrados** (compatibilidade garantida)
- **Acelerómetro LIS3DH** (orientação, tap detection)
- USB nativo (HID keyboard/mouse/MIDI)
- Qualidade comercial Adafruit
- Suporte CircuitPython + Arduino
- STEMMA QT (I2C) + JST analog

### 2.8 Limitações Identificadas

| Limitação | Impacto | Notas |
|-----------|---------|-------|
| **Preço elevado** | ~$25 USD só a placa | Sem painel |
| **Sem RTC** | Depende de NTP | Precisa internet |
| **Sem proteções robustas** | Apenas poly fuses | Sem TVS/ESD explícito |
| **Sensor luz removido** | ALS-PT19 no schematic mas não montado | |
| **Sem DAC** | Requer I2S para áudio | |

### 2.9 Gap para Proto Circadian Clock

```diff
+ Level shifters já incluídos (74AHCT125)
+ Acelerómetro pode ser útil (wake on movement)
+ ESP32-S3 mais potente
- RTC DS3231 com bateria backup
- Proteção inversão polaridade
- Proteção ESD robusta
- Proteção sobretensão (TVS)
- Sensor de luz ambiente funcional
```

---

## 3. ESP32-HUB75-Driver (Rorosaurus)

**Repositório:** [github.com/rorosaurus/esp32-hub75-driver](https://github.com/rorosaurus/esp32-hub75-driver)

### 3.1 Visão Geral

Design minimalista e de baixo custo. Filosofia "faça você mesmo" com PCB simples que apenas conecta os pinos necessários.

### 3.2 Características Técnicas

| Aspecto | Implementação |
|---------|---------------|
| **MCU** | ESP32-DEVKIT-V1 (módulo separado) |
| **Level Shifters** | **NÃO INCLUÍDOS** |
| **Latch IC Externo** | **NÃO INCLUÍDO** |
| **Alimentação** | Screw terminals opcionais (5V) |
| **Proteção** | Nenhuma |
| **Componentes** | ~46 pontos de solda |
| **Design Tool** | EasyEDA |

### 3.3 Filosofia de Design

```
MINIMALISMO EXTREMO:
━━━━━━━━━━━━━━━━━━━━

ESP32-DEVKIT-V1
      │
      └──[16 fios diretos]──► HUB75 Panel

Sem level shifters: "most panels work at 3.3V tolerant"
Sem latch IC: "uses more pins and memory"
```

### 3.4 Capacitores Opcionais

| Componente | Valor | Função |
|------------|-------|--------|
| Cerâmico SMD | 10µF (1206) | Auto-bootloader |
| Eletrolítico THT | 1000µF | Power smoothing 5V |

### 3.5 Pontos Fortes

- Custo mínimo (~$2-3 PCB)
- Gerbers disponíveis para fabrico direto
- Suporta daisy-chain de múltiplos painéis (testado com 5!)
- Compatível com 1/8, 1/16, 1/32 scan panels
- Design simples de entender e modificar

### 3.6 Limitações Identificadas

| Limitação | Impacto | Risco |
|-----------|---------|-------|
| **Sem level shifters** | Só painéis 3.3V tolerantes | Alto |
| **Sem latch IC** | Usa mais memória ESP32 | Médio |
| **Sem proteções** | Qualquer problema danifica tudo | Crítico |
| **Resolução limitada** | Máximo 64x32 recomendado | Médio |
| **WiFi comprometido** | 128x64 consome muita RAM DMA | Alto |

### 3.7 Gap para Proto Circadian Clock

```diff
- Level shifters 3.3V → 5V
- Todas as proteções (polaridade, ESD, sobretensão, curto)
- RTC integrado
- USB-C para programação
- Regulador 3.3V dedicado
- Status LEDs
- Botões de controlo
```

---

## 4. HUB75_Driver_PCB (ModischFabrications)

**Repositório:** [github.com/ModischFabrications/HUB75_Driver_PCB](https://github.com/ModischFabrications/HUB75_Driver_PCB)

### 4.1 Visão Geral

Projeto KiCad alemão focado em simplicidade com algumas proteções básicas. Licenciado sob CERN-OHL-S-2.0 (open hardware).

### 4.2 Características Técnicas

| Aspecto | Implementação |
|---------|---------------|
| **MCU** | ESP32-CH9102X (30-pin devboard) |
| **Level Shifters** | **NÃO INCLUÍDOS** |
| **Alimentação** | 5V 10W mínimo |
| **Proteção Curto** | **Polyfuse** |
| **Proteção Inversão** | **Díodo D1** |
| **Touch Inputs** | 2-3 entradas capacitivas |
| **Design Tool** | KiCad |

### 4.3 Proteções Implementadas

```
5V IN ──[D1 Díodo]──[Polyfuse]──► 5V SAFE

Se inversão: Díodo bloqueia → Polyfuse dispara
Se curto: Polyfuse dispara → Auto-reset quando arrefece
```

### 4.4 Consumo Medido

| Estado | Corrente |
|--------|----------|
| Idle | ~0.06A |
| Típico | ~0.7A |
| Máximo | ~2A |

### 4.5 Pontos Fortes

- **KiCad nativo** (fácil de modificar)
- GitHub Actions para exports automáticos (KiBot)
- Proteção básica contra curto-circuito
- Proteção contra inversão de polaridade
- Touch inputs para interação

### 4.6 Limitações Identificadas

| Limitação | Impacto | Notas |
|-----------|---------|-------|
| **Sem level shifters** | Compatibilidade limitada | |
| **Sem ESD** | Vulnerável a descargas | |
| **Sem TVS** | Vulnerável a sobretensão | |
| **USB + Matrix power** | NUNCA ligar simultaneamente | Risco de dano |
| **30-pin devboard** | IOs limitados | |

### 4.7 Gap para Proto Circadian Clock

```diff
+ Proteção curto-circuito (polyfuse)
+ Proteção inversão polaridade (díodo)
+ Design KiCad (podemos reutilizar)
- Level shifters 74AHCT245
- Proteção ESD (SRV05-4)
- Proteção sobretensão (TVS)
- RTC DS3231 integrado
- USB-C com proteção
```

---

## 5. SmartMatrix Shield (Pixelmatix)

**Repositório:** [github.com/pixelmatix/SmartMatrix](https://github.com/pixelmatix/SmartMatrix)

### 5.1 Visão Geral

Originalmente para Teensy, com porta ESP32. O projeto mais maduro em termos de software, com shields open-source disponíveis.

### 5.2 Características Técnicas

| Aspecto | Implementação |
|---------|---------------|
| **MCU Principal** | Teensy 3.x/4.x (melhor) |
| **MCU Alternativo** | ESP32 (com limitações) |
| **Level Shifters** | **74AHCT245** desde V3 |
| **Biblioteca** | SmartMatrix (muito otimizada) |
| **Design Files** | /extras/hardware/ |

### 5.3 Level Shifter Approach

```
Desde SmartMatrix Shield V3:
━━━━━━━━━━━━━━━━━━━━━━━━━━━

Teensy/ESP32 (3.3V) ──► 74AHCT245 ──► HUB75 (5V)
                         buffers

"Some panels won't work with 3.3V levels"
"Lowering ESP32_I2S_CLOCK_SPEED from 20MHz to 10-15MHz helps"
```

### 5.4 ESP32 Específico

- Ficheiro: `MatrixHardware_ESP32_V0.h`
- Clock rate recomendado: 10-15MHz (vs 20MHz default)
- Sem level shifter: imagem pode ser instável
- Melhor com shields de terceiros (Jason Coon, Evil Genius Labs)

### 5.5 Pontos Fortes

- Biblioteca extremamente otimizada
- Hardware open-source com schematics Eagle
- Comunidade estabelecida
- Level shifters incluídos (shields oficiais)
- Suporte Teensy (muito mais potente)

### 5.6 Limitações para ESP32

| Limitação | Impacto |
|-----------|---------|
| **Focado em Teensy** | ESP32 é "segunda classe" |
| **Clock instável** | Precisa baixar de 20MHz para 10-15MHz |
| **Shields ESP32** | Menos opções que Teensy |
| **Sem WiFi otimizado** | Conflito DMA vs WiFi stack |

---

## 6. Evil Genius Labs Shields

**Website:** [evilgeniuslabs.org](https://www.evilgeniuslabs.org/level-shifter-featherwing)

### 6.1 Level Shifter FeatherWing

| Aspecto | Implementação |
|---------|---------------|
| **Form Factor** | Adafruit Feather |
| **Level Shifter** | 74HCT245 |
| **Outputs** | 8 GPIO level-shifted |
| **Target** | Addressable RGB LEDs |

### 6.2 Wemos D32 Shield

| Aspecto | Implementação |
|---------|---------------|
| **Form Factor** | Wemos D32 |
| **Level Shifter** | 74HCT245 |
| **Outputs** | 8 GPIO (0, 2, 4, 12, 13, 15, 16, 17) |
| **Target** | General ESP32 projects |

### 6.3 Relevância para Projeto

- Demonstra implementação 74HCT245 em produção
- Pode servir de referência para routing
- Confirma que approach é viável e comercializável

---

## 7. Outras Soluções Identificadas

### 7.1 Tindie HUB75 Shield (Robot Power)

- Shield básico para WeMos Mini D1
- Sem level shifters
- PCB only (kit de solda)
- Preço: ~$5-10

### 7.2 ElectroDragon RGB Matrix Interface

- Suporta ESP32-DEVKITC ou ESP32-PICO
- Expõe pinos E4 e E8 para painéis 64x64
- Produção chinesa económica

### 7.3 OSHWLab ESP32 Mini P3P4

- Design EasyEDA comunitário
- Para ESP32 D1 MINI
- Compatível com biblioteca I2S-DMA

---

## 8. Tabela Comparativa Global

| Característica | ESP32-Trinity | MatrixPortal S3 | Rorosaurus | ModischFab | SmartMatrix | **Proto Circadian** |
|----------------|---------------|-----------------|------------|------------|-------------|---------------------|
| **MCU** | ESP32 | ESP32-S3 | ESP32 DevKit | ESP32 30-pin | Teensy/ESP32 | ESP32-WROOM-32E |
| **Level Shifters** | Não | 74AHCT125 x2 | Não | Não | 74AHCT245 | **74AHCT245 x2** |
| **Prot. Polaridade** | Limitada | Limitada | Não | Díodo | ? | **Schottky SS54** |
| **Prot. Curto** | Poly USB | Poly USB | Não | Polyfuse | ? | **PTC MF-MSMF250** |
| **Prot. ESD** | Não | Não | Não | Não | ? | **SRV05-4 + PESD** |
| **Prot. Sobretensão** | Não | Não | Não | Não | ? | **TVS SMBJ5.0A** |
| **RTC Integrado** | Não | Não | Não | Não | Não | **DS3231 + CR2032** |
| **Sensor Luz** | Não | Previsto* | Não | Não | Não | **Opcional** |
| **Acelerómetro** | Não | LIS3DH | Não | Não | Não | Não |
| **USB-C** | Sim | Sim | Não | Não | Não | **Sim** |
| **Status LEDs** | Não | NeoPixel | Não | Não | Não | **RGB (3x)** |
| **Botões** | Não | 2 | Não | Touch | Não | **3 (Mode/Reset/Boot)** |
| **Open Source** | OSHW Cert | CC BY-SA | MIT | CERN-OHL | Sim | **MIT** |
| **Preço Placa** | $15-20 | $25 | $2-3 | ~$5 | $25+ | **~$12-15** |
| **Design Tool** | EasyEDA | Eagle | EasyEDA | KiCad | Eagle | **KiCad** |

\* ALS-PT19 no schematic mas não montado na produção

---

## 9. Análise de Lacunas (Gap Analysis)

### 9.1 O Que Nenhum Concorrente Oferece

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    FEATURES ÚNICAS DO PROTO CIRCADIAN                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. RTC DS3231 INTEGRADO                                               │
│     → Funcionamento offline sem internet                                │
│     → Backup com bateria CR2032                                         │
│     → Precisão ±2ppm                                                    │
│                                                                         │
│  2. PROTEÇÕES ROBUSTAS (5 NÍVEIS)                                       │
│     → Inversão polaridade (Schottky)                                    │
│     → Curto-circuito (PTC resettable)                                   │
│     → ESD (TVS arrays)                                                  │
│     → Sobretensão (TVS clamping)                                        │
│     → Filtering (capacitores)                                           │
│                                                                         │
│  3. LEVEL SHIFTERS + PROTEÇÕES                                          │
│     → Compatibilidade garantida com qualquer painel                     │
│     → Proteção GPIO contra backfeed                                     │
│                                                                         │
│  4. FOCO EM APLICAÇÃO CIRCADIANA                                        │
│     → Solar algorithm NOAA integrado                                    │
│     → Red therapy mode                                                  │
│     → Mesh sync ESP-NOW                                                 │
│                                                                         │
│  5. USB-C COM PROTEÇÃO COMPLETA                                         │
│     → ESD protection                                                    │
│     → Programação + alimentação                                         │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 9.2 O Que Podemos Aprender de Cada Projeto

| Projeto | Lição a Incorporar |
|---------|-------------------|
| **ESP32-Trinity** | Documentação extensiva, comunidade Discord, exemplos prontos |
| **MatrixPortal S3** | Level shifter 74AHCT125/245, layout profissional, ESP32-S3 |
| **Rorosaurus** | Simplicidade, baixo custo, Gerbers prontos |
| **ModischFab** | KiCad workflow, proteções básicas, GitHub Actions (KiBot) |
| **SmartMatrix** | Software otimizado, clock speed tuning |
| **Evil Genius** | Comercialização de level shifter shields |

### 9.3 Decisões de Design Validadas

Com base na análise competitiva, as seguintes decisões do Proto Circadian Clock estão **validadas**:

| Decisão | Validação |
|---------|-----------|
| **74AHCT245 para level shifting** | Usado no SmartMatrix Shield V3+, Evil Genius Labs |
| **ESP32-WROOM vs ESP32-S3** | Trinity usa WROOM com sucesso, S3 é overkill para nossa app |
| **KiCad como ferramenta** | ModischFab demonstra workflow GitHub Actions |
| **Proteções robustas** | NENHUM concorrente oferece - diferencial competitivo |
| **RTC integrado** | NENHUM concorrente oferece - diferencial para offline |
| **USB-C** | MatrixPortal S3 e Trinity já usam - padrão esperado |

---

## 10. Recomendações Finais

### 10.1 Prioridade Alta (Implementar)

1. **Level Shifters 74AHCT245**
   - Copiar approach Adafruit/SmartMatrix
   - 2 ICs para todos os 13 sinais HUB75

2. **Proteções Completas**
   - Diferencial competitivo único
   - Seguir design já documentado em PCB_DESIGN_GUIDE.md

3. **RTC DS3231 Integrado**
   - Crítico para funcionamento offline
   - Holder CR2032 SMD

### 10.2 Prioridade Média (Considerar)

4. **Migração para ESP32-S3**
   - Dual-core WiFi+Display
   - USB nativo
   - Mais RAM

5. **Sensor de Luz Ambiente**
   - ALS-PT19 ou similar
   - Ajuste automático de brilho

### 10.3 Prioridade Baixa (Futuro)

6. **Acelerómetro LIS3DH**
   - Wake on movement
   - Orientação automática

7. **NeoPixel Status**
   - RGB animado vs LEDs discretos
   - Mais flexível mas mais complexo

---

## 11. Recursos e Links

### Repositórios Analisados

- [ESP32-Trinity](https://github.com/witnessmenow/ESP32-Trinity)
- [Adafruit MatrixPortal S3 PCB](https://github.com/adafruit/Adafruit-MatrixPortal-S3-PCB)
- [Rorosaurus ESP32-HUB75-Driver](https://github.com/rorosaurus/esp32-hub75-driver)
- [ModischFabrications HUB75_Driver_PCB](https://github.com/ModischFabrications/HUB75_Driver_PCB)
- [SmartMatrix](https://github.com/pixelmatix/SmartMatrix)
- [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA)

### Documentação Complementar

- [Adafruit MatrixPortal S3 Learn](https://learn.adafruit.com/adafruit-matrixportal-s3)
- [ESPHome HUB75 Component](https://esphome.io/components/display/hub75/)
- [WLED MoonModules HUB75](https://mm.kno.wled.ge/2D/HUB75/)

### Compra de Hardware

- [Makerfabs ESP32-Trinity](https://www.makerfabs.com/esp32-trinity.html)
- [Adafruit MatrixPortal S3 (ID 5778)](https://www.adafruit.com/product/5778)
- [Tindie HUB75 Shields](https://www.tindie.com/search/?q=hub75+esp32)

---

*Documento criado: Dezembro 2024*
*Versão: 1.0*
*Autor: Claude (AI Assistant)*
