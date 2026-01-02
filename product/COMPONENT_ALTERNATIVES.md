# Component Alternatives - JLCPCB Stock Issues

**Date:** January 2026
**Issue:** Several extended parts unavailable at JLCPCB SMT assembly

---

## Summary of Issues

| Ref | Original Part | LCSC | Status | Issue |
|-----|--------------|------|--------|-------|
| BZ1 | CMT-7525-80-SMT-TR | C3151660 | Extended | 6 shortfall |
| D2 | YLED0805R | C19171391 | Extended | 20 shortfall |
| F1 | JK-NSMD050-13.2V | C369159 | Extended | 20 shortfall |
| J2 | AP2305GN-HF | C124516 | **BOM ERROR** | Wrong part type! |
| U6 | AP2112K-3.3TRG1 | C51118 | Extended | 7 shortfall |

---

## 1. BZ1 - Buzzer

### Original
- **Part:** CMT-7525-80-SMT-TR (CUI Devices)
- **LCSC:** C3151660
- **Specs:** 7.5x7.5mm SMD, passive piezo, 80dB

### Recommended Alternatives

| Part | LCSC | Type | Size | SPL | Stock | Notes |
|------|------|------|------|-----|-------|-------|
| ~~MLT-5030~~ | ~~C95297~~ | Passive | 5.2x5.7mm | 80dB | ❌ OOS | Design guide spec - OUT OF STOCK |
| **HYG9605C** | **C438342** | Passive | 9.6x5mm | 85dB | Check | Louder, good alternative |
| **KLJ-1102** | **C201047** | Passive | 9x11mm | 70dB | Check | KELIKING |
| **GPC12075YB-5V** | **C252948** | Passive | 12mm | 80dB | Check | INGHAi, 4kHz resonance |
| FUET-1230 | C391037 | Passive | 12x12mm | 75dB | Check | 4kHz |
| PKM13EPYH4000 | C94599 | Passive | 13mm | 70dB | Extended | Murata, better high freq |

⚠️ **Do NOT use TMB12A05 (C96093)** - it's an ACTIVE buzzer (won't work with transistor+PWM driver)

**Recommendation:** Try **HYG9605C (C438342)** or **GPC12075YB-5V (C252948)** - passive buzzers, verify stock before ordering.

**Footprint Change:** Will vary - check datasheet for chosen part

---

## 2. D2 - Red LED (WiFi Status)

### Original
- **Part:** YLED0805R
- **LCSC:** C19171391
- **Specs:** 0805, Red, 615-630nm, 120mcd

### Recommended Alternatives

| Part | LCSC | Package | Color | Vf | mcd | Stock | Notes |
|------|------|---------|-------|-----|-----|-------|-------|
| **17-21SURC/S530-A3/TR8** | **C2286** | 0805 | Red | 2.0V | 150 | **Basic** | Standard, well-stocked |
| XL-1608URC | C965799 | 0603 | Red | 2.0V | 100 | Basic | Smaller option |
| 19-217/R6C-AL1M2VY/3T | C965804 | 0805 | Red | 2.0V | 110 | Basic | Everlight |
| KT-0805R | C2290 | 0805 | Red | 2.0V | 100 | Basic | KingTai |

**Recommendation:** Use **C2286** - JLCPCB basic part, high stock, same 0805 footprint.

**Footprint Change:** None required (same 0805 package)

---

## 3. F1 - PTC Resettable Fuse

### Original
- **Part:** JK-NSMD050-13.2V
- **LCSC:** C369159
- **Specs:** 1206 package, 500mA hold, 1A trip, 13.2V

### Design Note
The PCB_DESIGN_GUIDE.md specifies **MF-MSMF300** (3A hold, 6A trip, 1812 package) for main power protection. The 500mA fuse may be for a different application.

### Recommended Alternatives

**For 500mA (original spec):**

| Part | LCSC | Package | Hold | Trip | Voltage | Stock |
|------|------|---------|------|------|---------|-------|
| **SMD1206P050TF/15** | **C106264** | 1206 | 500mA | 1A | 15V | Available |
| nSMD050-30V | C2836575 | 1206 | 500mA | 1A | 30V | Extended |

**For 3A (design guide spec):**

| Part | LCSC | Package | Hold | Trip | Voltage | Stock |
|------|------|---------|------|------|---------|-------|
| **MF-MSMF250-2** | **C17313** | 1812 | 2.5A | 5A | 15V | Basic |
| SMD1812P200TF/16 | C106265 | 1812 | 2A | 4A | 16V | Available |
| nSMD300-8V | C262674 | 1812 | 3A | 6A | 8V | Extended |

**Recommendation:**
- If protecting USB 5V/3A rail: Use **MF-MSMF250-2 (C17313)** - 2.5A hold, 1812, basic part
- If original 500mA spec needed: Use **SMD1206P050TF/15 (C106264)**

**Footprint Change:** Check if 1206 or 1812 footprint is on PCB

---

## 4. J2 - 2x08 Pin Header (BOM ERROR!)

### Problem
The BOM lists **AP2305GN-HF (C124516)** which is a **P-channel MOSFET**, NOT a pin header!

This appears to be a BOM file error - wrong LCSC part number was assigned.

### Required Part
- **Type:** 2x8 (16-pin) pin header
- **Pitch:** 2.54mm
- **Mounting:** Through-hole vertical

### Correct Alternatives

| Part | LCSC | Pitch | Rows | Pins | Stock | Notes |
|------|------|-------|------|------|-------|-------|
| **2.54-2*8P** | **C68234** | 2.54mm | 2 | 16 | Available | BOOMELE, gold pins, 3A rated |
| 2.54-2*8P H8.5mm | C124404 | 2.54mm | 2 | 16 | Available | Gold plated |
| PM2.54-2X8 | C65114 | 2.54mm | 2 | 16 | Available | Boom Precision |

**Recommendation:** Use **C68234** - standard 2x8 pin header from BOOMELE.

**Action Required:** Fix BOM file to replace C124516 with correct pin header part number!

---

## 5. U6 - AP2112K-3.3 LDO Regulator

### Original
- **Part:** AP2112K-3.3TRG1 (Diodes Inc)
- **LCSC:** C51118
- **Specs:** 3.3V 600mA LDO, SOT-23-5, 250mV dropout

### Note on Reference Designator
In PCB_DESIGN_GUIDE.md, the AP2112K-3.3 is designated as **U3**, not U6. U6 is the 74AHCT245 level shifter. Please verify the correct reference designator.

### Recommended Alternatives

| Part | LCSC | Package | Iout | Dropout | Iq | Stock | Notes |
|------|------|---------|------|---------|-----|-------|-------|
| **ME6211C33M5G-N** | **C82942** | SOT-23-5 | 500mA | 100mV | 40µA | **Basic** | Best stock |
| RT9013-33GB | C47773 | SOT-23-5 | 500mA | 200mV | 25µA | Basic | Low Iq |
| XC6220B331MR-G | C86534 | SOT-23-5 | 1A | 300mV | 8µA | Available | Better specs |
| HT7333-A | C21583 | SOT-89 | 250mA | 100mV | 4µA | Basic | Different package |
| AP2112K-3.3TRG1 | C460204 | SOT-23-5 | 600mA | 250mV | 55µA | Available | Same part, alt vendor |

**Recommendation:** Use **ME6211C33M5G-N (C82942)** - JLCPCB basic part, same pinout, adequate current.

**Alternative:** **XC6220B331MR-G (C86534)** - superior specs (1A, 8µA Iq) if extended parts acceptable.

**Footprint Change:** None required (same SOT-23-5 package)

---

## Quick Reference - Recommended Replacements

| Ref | Original LCSC | Replace With | New LCSC | Part Type |
|-----|--------------|--------------|----------|-----------|
| BZ1 | C3151660 | HYG9605C (passive) | **C438342** | Check stock |
| D2 | C19171391 | 17-21SURC/S530-A3/TR8 | **C2286** | Basic |
| F1 | C369159 | SMD1206P050TF/15 | **C106264** | Available |
| J2 | C124516 ❌ | 2.54-2*8P Header | **C68234** | Available |
| U6 | C51118 | ME6211C33M5G-N | **C82942** | Basic |

---

## BOM Update Instructions

Update your BOM CSV file with these replacements:

```csv
# Original → Replacement
BZ1,C3151660 → C438342  # HYG9605C passive buzzer (verify stock!)
D2,C19171391 → C2286
F1,C369159 → C106264
J2,C124516 → C68234  # CRITICAL: Fix wrong part type!
U6,C51118 → C82942
```

---

## Notes

1. **Footprint Updates Required:**
   - BZ1: Update footprint from 7.5x7.5mm to 5.2x5.7mm for MLT-5030

2. **Verify Reference Designators:**
   - U6 in your BOM is AP2112K-3.3, but design guide has this as U3
   - U6 in design guide is 74AHCT245 level shifter
   - Please verify which is correct for your schematic

3. **Stock Verification:**
   - Always verify stock before ordering at [JLCPCB Parts](https://jlcpcb.com/parts)
   - Basic parts have lower assembly fees than Extended parts

---

## Sources

- [JLCPCB Parts Library](https://jlcpcb.com/parts)
- [JLCPCB Resettable Fuses](https://jlcpcb.com/parts/2nd/Circuit_Protection/Resettable_Fuses_3294)
- [JLCPCB LED Indication](https://jlcpcb.com/parts/2nd/Optoelectronics/LED_Indication_Discrete_71)
- [JLCPCB Basic Parts](https://jlcpcb.com/parts/basic_parts)

*Document generated: January 2026*
