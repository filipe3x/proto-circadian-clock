# Alimentação do ESP32

Este documento descreve estratégias seguras para alimentar o ESP32 a partir de uma fonte DC 5V externa, partilhada com o painel P10.

## Contexto

O painel P10 32x16 RGB requer uma fonte 5V com pelo menos 2-4A. É conveniente alimentar o ESP32 a partir da mesma fonte, eliminando a necessidade de um carregador USB separado.

## Requisitos de Energia

| Componente | Tensão | Corrente Típica | Corrente Máxima |
|------------|--------|-----------------|-----------------|
| ESP32 Dev Module | 5V (via VIN/USB) | 80-150mA | 500mA |
| Painel P10 32x16 | 5V | 1-2A | 4A |
| DS3231 RTC | 3.3V | 1mA | 3mA |
| **Total** | 5V | ~2A | ~4.5A |

> **Recomendação:** Fonte 5V 5A para margem de segurança.

---

## Estratégia 1: Cabo DC para USB-C (Mais Simples)

A forma mais fácil de alimentar o ESP32 é usar um cabo adaptador que converte a ficha DC da fonte para USB-C.

### Vantagens
- Sem soldadura
- Usa a proteção interna do ESP32
- Fácil de desconectar para programação

### Desvantagens
- Mais um cabo no projeto
- Dependente da qualidade do cabo

### O que comprar (AliExpress)

| Pesquisa | Preço estimado |
|----------|----------------|
| `"DC 5.5x2.1 to USB C cable"` | ~1-2€ |
| `"DC barrel jack to USB-C adapter"` | ~1-2€ |

### Esquema

```
Fonte 5V DC ────[Cabo DC→USB-C]──── USB-C (ESP32)
     │
     └── Painel P10 (5V + GND)
```

---

## Estratégia 2: Alimentação pelos Pinos (Mais Elegante)

Alimentar diretamente pelo pino **VIN** ou **5V** do ESP32, com circuito de proteção.

### Vantagens
- Instalação mais limpa
- Menos cabos
- Proteção personalizada

### Desvantagens
- Requer soldadura
- Perde alguma proteção do regulador USB

### Circuito de Proteção Recomendado

```
                    Fusível PTC        Díodo Schottky
Fonte 5V (+) ────────┤ 500mA ├──────────┤>├──────┬────── VIN (ESP32)
                                                 │
                                          100µF ═╪═ 100nF
                                                 │
Fonte 5V (-) ────────────────────────────────────┴────── GND (ESP32)
```

### Componentes de Proteção

#### 1. Díodo Schottky (Proteção de Polaridade)

Previne danos se os fios forem ligados ao contrário.

| Componente | Especificação | Pesquisa AliExpress |
|------------|---------------|---------------------|
| 1N5822 | 3A, 40V, Vf=0.3V | `"1N5822 schottky diode"` |
| SS34 | 3A, 40V, Vf=0.35V | `"SS34 schottky diode"` |
| SS54 | 5A, 40V, Vf=0.35V | `"SS54 schottky diode"` |

> **Nota:** Schottky tem queda de tensão baixa (~0.3V), resultando em 4.7V para o ESP32 (aceitável).

#### 2. Capacitores de Filtragem (Supressão de Ruído)

Absorvem picos de tensão e filtram ruído da fonte.

| Componente | Função | Pesquisa AliExpress |
|------------|--------|---------------------|
| 100µF 16V Eletrolítico | Filtragem geral | `"100uF 16V electrolytic capacitor"` |
| 100nF (0.1µF) Cerâmico | Ruído alta frequência | `"100nF ceramic capacitor"` ou `"104 capacitor"` |

> **Dica:** O código "104" em capacitores cerâmicos significa 100nF.

#### 3. Fusível Resettável PTC (Proteção de Curto-Circuito)

Corta a corrente em caso de curto-circuito e reseta automaticamente.

| Componente | Especificação | Pesquisa AliExpress |
|------------|---------------|---------------------|
| PPTC 500mA | Hold: 500mA, Trip: 1A | `"PPTC fuse 500mA"` |
| MF-R050 | 500mA resettable | `"MF-R050 fuse"` |

---

## Estratégia 3: Módulo Buck Converter (Mais Robusta)

Se a fonte não for perfeitamente estável ou tiver tensão ligeiramente superior a 5V.

### Quando usar
- Fonte com tensão 6-12V
- Fonte não regulada ou ruidosa
- Necessidade de isolamento adicional

### O que comprar

| Pesquisa AliExpress | Preço |
|---------------------|-------|
| `"MP1584 buck converter"` | ~1€ |
| `"LM2596 DC-DC step down"` | ~1-2€ |
| `"mini 360 buck converter"` | ~0.5€ |

### Esquema

```
Fonte 5-12V ────[Buck Converter]──── 5V ──── VIN (ESP32)
                    │
                  Ajustar para 5V
```

> **Importante:** Ajusta o potenciómetro do módulo para 5V **antes** de ligar ao ESP32!

---

## Lista de Compras AliExpress

### Kit Básico (Estratégia 1)

| Item | Quantidade | Pesquisa | Preço |
|------|------------|----------|-------|
| Cabo DC para USB-C | 1 | `"DC 5.5x2.1 to USB C"` | ~2€ |

**Total: ~2€**

### Kit Completo (Estratégia 2)

| Item | Quantidade | Pesquisa | Preço |
|------|------------|----------|-------|
| Díodos 1N5822 | 10+ | `"1N5822 schottky"` | ~1€ |
| Capacitores 100µF 16V | 10+ | `"100uF 16V electrolytic"` | ~1€ |
| Capacitores 100nF | 20+ | `"100nF ceramic capacitor"` | ~1€ |
| Fusíveis PTC 500mA | 10+ | `"PPTC 500mA fuse"` | ~1€ |
| Fichas DC fêmea | 5+ | `"DC 5.5x2.1 female terminal"` | ~1€ |
| Placa perfurada | 2+ | `"PCB prototype board 5x7"` | ~1€ |

**Total: ~6€**

---

## Cuidados de Segurança

### Antes de ligar

- [ ] Verificar polaridade com multímetro
- [ ] Confirmar tensão da fonte (deve ser 4.8V - 5.5V)
- [ ] Inspecionar soldaduras (sem curtos)
- [ ] Testar circuito de proteção sem ESP32 primeiro

### Riscos e Mitigações

| Risco | Consequência | Proteção |
|-------|--------------|----------|
| Tensão > 6V | Queima o regulador | Buck converter ou fonte regulada |
| Polaridade invertida | Curto-circuito | Díodo Schottky |
| Picos de tensão | Danos nos componentes | Capacitores de filtragem |
| Curto-circuito | Sobreaquecimento/fogo | Fusível PTC |
| Ruído elétrico | Comportamento errático | Capacitores cerâmicos |

### Teste do Circuito de Proteção

1. **Sem ESP32**, liga a fonte ao circuito
2. Mede a tensão na saída (deve ser ~4.7V devido ao díodo)
3. Provoca um curto momentâneo na saída - o fusível deve atuar
4. Aguarda 30 segundos - o fusível deve resetar
5. Se tudo OK, liga o ESP32

---

## Sequência de Arranque Recomendada

Quando a fonte é partilhada entre o painel P10 e o ESP32:

```
1. Liga a fonte 5V
2. Painel P10 arranca (pico de corrente inicial)
3. Aguarda 1-2 segundos
4. ESP32 arranca (se alimentação separada por switch)
```

> **Nota:** Na prática, ambos podem arrancar juntos se a fonte tiver capacidade suficiente (5A+). O pico inicial do painel P10 é breve.

---

## Diagrama de Ligações Completo

```
                                    ┌─────────────────────────┐
                                    │      ESP32 Dev Module   │
                                    │                         │
Fonte 5V ──┬── Fusível ── Díodo ──┬─┤ VIN              GPIO25 ├──→ R1
    (+)    │   500mA     1N5822   │ │                  GPIO26 ├──→ G1
           │                      │ │                  GPIO27 ├──→ B1
           │               100µF ═╪═│                  GPIO14 ├──→ R2
           │               100nF  │ │                  GPIO12 ├──→ G2    Painel
           │                      │ │                  GPIO13 ├──→ B2     P10
           │                      │ │                  GPIO23 ├──→ A    (HUB75)
           │                      │ │                  GPIO19 ├──→ B
           │                      │ │                   GPIO5 ├──→ C
           │                      │ │                  GPIO17 ├──→ D
           │                      │ │                   GPIO4 ├──→ LAT
           │                      │ │                  GPIO15 ├──→ OE
           │                      │ │                  GPIO16 ├──→ CLK
           │                      │ │                         │
           │            ┌─────────┼─┤ GND                 GND ├──→ GND
           │            │         │ │                         │
    (-)────┴────────────┴─────────┘ └─────────────────────────┘
           │
           └──────────────────────────────────────────────────────→ GND (P10)
           │
           └──────────────────────────────────────────────────────→ 5V (P10)
```

---

## Referências

- [ESP32 Dev Module Pinout](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [ESP32 Power Supply Guide](https://randomnerdtutorials.com/esp32-power-supply/)
- [Schottky Diode Protection](https://electronics.stackexchange.com/questions/tagged/reverse-polarity-protection)
- [PTC Fuse Selection](https://www.littelfuse.com/technical-resources/technical-centers/resettable-ptcs.aspx)
