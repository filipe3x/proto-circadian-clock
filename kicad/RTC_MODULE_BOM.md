# RTC Module - Bill of Materials

**Módulo:** DS3231SN RTC com Interface I2C
**Tensão:** 3.3V
**Data:** Janeiro 2026
**Para:** Circadian Clock PCB

---

## Componentes do Módulo RTC

| Ref | Componente | Valor | Package | LCSC | Preço (USD) | Notas |
|-----|------------|-------|---------|------|-------------|-------|
| **U1** | DS3231SN | RTC I2C | SOIC-16W | **C722469** | ~$2.37 | RTC com TCXO integrado, ±2ppm |
| **R1** | Resistência | 4.7kΩ | 0402 | **C25900** | ~$0.0004 | Pull-up I2C SDA |
| **R2** | Resistência | 4.7kΩ | 0402 | **C25900** | ~$0.0004 | Pull-up I2C SCL |
| **C1** | Condensador | 100nF | 0402 | **C307331** | ~$0.002 | Bypass VCC |
| **BT1** | Suporte Bateria | CR2032 | SMD | **C70377** | ~$0.10 | Backup battery holder |

---

## Resumo de Códigos LCSC

```
DS3231SN (RTC)............ C722469
4.7kΩ 0402 (pull-up)...... C25900
100nF 0402 (bypass)....... C307331
CR2032 Holder (SMD)....... C70377
```

---

## Pinout DS3231SN (SOIC-16)

```
        ┌─────────────────┐
  32KHz │ 1            16 │ SCL ──────► ESP32 GPIO22
    VCC │ 2            15 │ SDA ◄─────► ESP32 GPIO21
INT/SQW │ 3 (NC)       14 │ VBAT ◄──── CR2032 (+)
   ~RST │ 4 (NC)       13 │ GND  ← Pull-up interno 50kΩ
     NC │ 5            12 │ NC
     NC │ 6            11 │ NC
     NC │ 7            10 │ NC
     NC │ 8             9 │ NC
        └─────────────────┘
```

### Ligações Principais

| Pino DS3231 | Função | Ligação |
|-------------|--------|---------|
| 2 (VCC) | Alimentação | +3.3V |
| 13 (GND) | Massa | GND |
| 14 (VBAT) | Bateria backup | CR2032 (+) |
| 15 (SDA) | I2C Data | ESP32 GPIO21 + 4.7kΩ pull-up |
| 16 (SCL) | I2C Clock | ESP32 GPIO22 + 4.7kΩ pull-up |
| 3 (INT/SQW) | Interrupção (opcional) | NC (não conectado) |
| 4 (~RST) | Reset | NC (pull-up interno 50kΩ) |

---

## Esquema de Ligações

```
                    +3.3V
                      │
              ┌───────┼───────┐
              │       │       │
           ┌──┴──┐ ┌──┴──┐    │
           │4.7kΩ│ │4.7kΩ│    │
           │ R1  │ │ R2  │   ─┴─
           └──┬──┘ └──┬──┘   ─┬─ C1 100nF
              │       │       │
              │       │       │
ESP32 GPIO21 ─┴─ SDA  │       ├──── VCC (pin 2)
                      │       │        │
ESP32 GPIO22 ─────── SCL      │   ┌────┴────┐
                              │   │         │
                              │   │ DS3231  │
                              │   │   SN    │
                              │   │         │
                              │   └────┬────┘
                              │        │
                              └────────┴──── GND (pin 13)


  CR2032 (+) ───────────────────────── VBAT (pin 14)
  CR2032 (-) ───────────────────────── GND
```

---

## Ligações I2C por Placa

### ESP32 Dev Module (Standard)
| Sinal | GPIO |
|-------|------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Matrix Portal S3
| Sinal | GPIO |
|-------|------|
| SDA | GPIO 16 |
| SCL | GPIO 17 |

---

## Especificações Técnicas DS3231SN

| Parâmetro | Valor |
|-----------|-------|
| Tensão VCC | 2.3V - 5.5V |
| Tensão VBAT | 2.3V - 5.5V |
| Corrente VCC (típica) | 200µA @ 3.3V |
| Corrente VBAT (típica) | 0.84µA @ 3.0V |
| Precisão | ±2ppm (0°C a +40°C) |
| Interface | I2C (400kHz max) |
| Endereço I2C | 0x68 (fixo) |
| Cristal | Integrado (32.768kHz TCXO) |
| Sensor Temperatura | Integrado (±3°C) |
| Package | SOIC-16W (7.5 x 10.3mm) |

---

## Notas de Montagem JLCPCB

### Tipo de Assemblagem
- **Standard PCBA** requerido (não Economic)
- DS3231SN requer manipulação precisa

### Custo Estimado (5 unidades)
| Item | Custo |
|------|-------|
| DS3231SN x5 | ~$11.85 |
| Resistências 4.7kΩ x10 | ~$0.01 |
| Condensadores 100nF x5 | ~$0.01 |
| CR2032 Holders x5 | ~$0.50 |
| **Total componentes** | **~$12.40** |

### Footprints KiCad

| Componente | Footprint |
|------------|-----------|
| DS3231SN | `Package_SO:SOIC-16W_7.5x10.3mm_P1.27mm` |
| 4.7kΩ | `Resistor_SMD:R_0402_1005Metric` |
| 100nF | `Capacitor_SMD:C_0402_1005Metric` |
| CR2032 | `Battery:BatteryHolder_Keystone_3034_1x20mm` ou SMD |

---

## Alternativas de Componentes

### DS3231 Alternativas
| Part Number | LCSC | Notas |
|-------------|------|-------|
| DS3231SN# | **C722469** | ✅ Recomendado |
| DS3231SN#T&R | **C9866** | Tape & Reel |
| DS3231M | - | Versão MEMS (menos preciso) |

### CR2032 Holder Alternativas
| Part Number | LCSC | Tipo |
|-------------|------|------|
| CR2032-BS-6-1 (Q&J) | **C70377** | ✅ SMD, recomendado |
| BS-2032-11ZJDZ | C19184100 | SMD alternativo |
| CR2032 3V (Q&J) | C70373 | SMD clip |

---

## Verificação Final

- [ ] DS3231SN: LCSC **C722469** ✓
- [ ] Resistências 4.7kΩ 0402: LCSC **C25900** ✓
- [ ] Condensador 100nF 0402: LCSC **C307331** ✓
- [ ] CR2032 Holder: LCSC **C70377** ✓
- [ ] Todos os footprints verificados
- [ ] I2C pull-ups calculados para 400kHz

---

## Notas Importantes

1. **~RST (pin 4)**: Tem pull-up **interno** de 50kΩ - **NÃO** precisa de pull-up externo
2. **INT/SQW (pin 3)**: Só usar se precisar de alarmes - deixar NC para uso normal
3. **Cristal**: O DS3231SN tem TCXO **integrado** - não adicionar cristal externo
4. **I2C Pull-ups**: 4.7kΩ são adequados para 400kHz, podem ser partilhados com outros dispositivos I2C

---

*Documento gerado: Janeiro 2026*
*Atualizado: Janeiro 2026 - Correção pino RST (pull-up interno)*
