# BOM Verificado - Circadian Clock PCB

**Data:** Janeiro 2026
**Fonte:** JLCPCB BOM Detection
**Total:** 24 componentes

---

## ⚠️ Problemas Críticos Detetados

| Ref | Problema | Ação Necessária |
|-----|----------|-----------------|
| **C3** | Footprint 0805, componente é 1206 | Mudar footprint para `C_1206_3216Metric` |
| **D2** | Footprint 0402, componente é 0603 | Mudar footprint para `LED_0603_1608Metric` |
| **J1** | LCSC errado (Hanxia genérico) | Mudar para **C3020560** (GCT USB4105-GF-A) |
| **BZ1** | Verificar se footprint THT está correto | Confirmar Plugin vs SMD |

---

## Lista Completa de Componentes

### Microcontrolador

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| U3 | ESP32-WROOM-32E-N8 | `ESP32-WROOM-32E` | 25.5 x 18.0 mm | **C701342** | ✅ |

### ICs

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| U4 | CH340C | `SOIC-16_3.9x9.9mm_P1.27mm` | 3.9 x 9.9 mm | **C84681** | ✅ |
| U5 | D3V3XA4B10LP-7 | `Diodes_UDFN-10_1.0x2.5mm_P0.5mm` | 1.0 x 2.5 mm | **C1980462** | ✅ |
| U6 | ME6211C33M5G-N | `SOT23-5` | 2.9 x 1.6 mm | **C82942** | ✅ |

### Transistores

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| Q1 | UMH3N | `SOT-363_SC-70-6` | 2.0 x 1.25 mm | **C62892** | ✅ |
| Q2 | MMBT2222A | `SOT23-3` | 2.9 x 1.3 mm | **C916372** | ✅ |

### Díodos e LEDs

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| D1 | 1N4148TR | `D_DO-35_SOD27_P7.62mm_Horizontal` | Ø1.85 x 4.25 mm (THT) | **C84410** | ✅ THT |
| D2 | KT-0603R | ~~`LED_0402_1005Metric_Red`~~ | 1.6 x 0.8 mm | **C2286** | ❌ MUDAR para `LED_0603_1608Metric` |
| D3 | SMF9.0CA (TVS) | `D_SMB` | 5.3 x 3.6 mm | **C123799** | ✅ |

### Condensadores

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| C3 | CL31A226KAHNNNE (22µF) | ~~`C_0805_2012Metric`~~ | 3.2 x 1.6 mm | **C12891** | ❌ MUDAR para `C_1206_3216Metric` |
| C4,C6,C8 | CL05B104KB54PNC (100nF) | `C_0402_1005Metric` | 1.0 x 0.5 mm | **C307331** | ✅ |
| C5 | CL10A106KP8NNNC (10µF) | `C_0603_1608Metric` | 1.6 x 0.8 mm | **C19702** | ✅ |

### Resistências

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| R1 | 330Ω | `R_0402_1005Metric` | 1.0 x 0.5 mm | **C25104** | ✅ |
| R2 | 100Ω | `R_0402_1005Metric` | 1.0 x 0.5 mm | **C106232** | ✅ |
| R3,R4 | 5.1kΩ | `R_0402_1005Metric` | 1.0 x 0.5 mm | **C25905** | ✅ |
| R5 | 10kΩ | `R_0402_1005Metric` | 1.0 x 0.5 mm | **C25744** | ✅ |
| R6 | 1kΩ | `R_0402_1005Metric` | 1.0 x 0.5 mm | **C106235** | ✅ |

### Fusível

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| F1 | SMD1206P050TF/15 (500mA) | `Fuse_1206_3216Metric` | 3.2 x 1.6 mm | **C106264** | ✅ |

### Conectores

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| J1 | ~~HX-TYPE-C~~ → **USB4105-GF-A** | `USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal` | 8.94 x 7.30 mm | ~~C49261569~~ → **C3020560** | ❌ MUDAR LCSC |
| J2 | 2.54-2*8P | `PinHeader_2x08_P2.54mm_Vertical` | 20.3 x 5.08 mm (THT) | **C68234** | ✅ |
| J3 | KF301-5.0-2P | `TerminalBlock_Phoenix_MKDS-3-2-5.08_1x02_P5.08mm_Horizontal` | 10.16 x 7.5 mm (THT) | **C474881** | ✅ |

### Buzzer

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| BZ1 | GPC12075YB-5V | `Buzzer_12x9.5RM7.6` | Ø12 x 6.5 mm (THT) | **C252948** | ⚠️ Confirmar THT |

### Botões

| Ref | Componente | Footprint KiCad | Dimensões | LCSC | Verificado |
|-----|------------|-----------------|-----------|------|------------|
| SW1,SW2 | TS-1088-AR02016 | `SW_SPST_TS-1088-xR025` | 4.0 x 3.0 mm (SMD) | **C720477** | ✅ |

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
U3  = C701342   (ESP32-WROOM-32E-N8)
U4  = C84681    (CH340C)
U5  = C1980462  (D3V3XA4B10LP-7)
U6  = C82942    (ME6211C33M5G-N)
Q1  = C62892    (UMH3N)
Q2  = C916372   (MMBT2222A)
D1  = C84410    (1N4148TR)
D2  = C2286     (KT-0603R) ← Footprint 0603!
D3  = C123799   (SMF9.0CA)
C3  = C12891    (22µF 1206) ← Footprint 1206!
C4,C6,C8 = C307331 (100nF 0402)
C5  = C19702    (10µF 0603)
R1  = C25104    (330Ω)
R2  = C106232   (100Ω)
R3,R4 = C25905  (5.1kΩ)
R5  = C25744    (10kΩ)
R6  = C106235   (1kΩ)
F1  = C106264   (PTC 500mA 1206)
J1  = C3020560  (USB4105-GF-A) ← CORRIGIDO!
J2  = C68234    (2x8 Header)
J3  = C474881   (KF301-5.0-2P)
BZ1 = C252948   (GPC12075YB-5V)
SW1,SW2 = C720477 (TS-1088)
```

---

*Documento gerado: Janeiro 2026*
