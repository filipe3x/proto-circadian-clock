# Guia KiCad - Bloco 1: USB-C PD Input

## Passo 0: Adicionar Biblioteca de Símbolos

1. No KiCad, vai a **Preferences → Manage Symbol Libraries**
2. Clica em **Add existing library to table** (ícone de pasta)
3. Navega para: `clockv2_kicad/symbols/PSU_Components.kicad_sym`
4. Clica OK

---

## Passo 1: Corrigir F1 (PTC Fuse)

O F1 atual tem valores errados. Corrigir:

1. Clica no F1 existente no esquema
2. Pressiona **E** para editar propriedades
3. Alterar:
   - **Value**: `3A 30V`
   - **Footprint**: `Fuse:Fuse_2920_7451Metric`
   - **LCSC**: `C2982291`
   - **BASIC**: `Extended`

---

## Passo 2: Layout Sugerido

Posições aproximadas (coordenadas X, Y em mm):

```
                    (120, 100)
                       F1
                       │
    J1 (USB-C)         ▼
    (97, 150)    ──► VBUS ──► Q1 (AO3400A) ──► Saída 20V
                      │       (145, 100)        (170, 100)
                      │           │
                      │      U1 (IP2721)
                      │      (145, 130)
                      │
                   C_IN
                   (115, 115)
```

---

## Passo 3: Adicionar Componentes

### 3.1 U1 - IP2721 (PD Trigger)

1. Pressiona **A** (Add Symbol)
2. Procura: `PSU_Components:IP2721`
3. Coloca em **~(145, 130)**
4. As propriedades já estão corretas no símbolo

### 3.2 Q1 - AO3400A (N-MOSFET)

1. Pressiona **A**
2. Procura: `Transistor_FET:AO3400A`
3. Coloca em **~(145, 100)**
4. Pressiona **E** e define:
   - **LCSC**: `C20917`
   - **BASIC**: `Basic`
   - **Footprint**: `Package_TO_SOT_SMD:SOT-23`

**NOTA**: Se não encontrares AO3400A, usa `BSS138` ou qualquer N-MOSFET genérico e edita as propriedades.

### 3.3 Condensadores

| Ref | Símbolo | Posição | Value | Footprint | LCSC |
|-----|---------|---------|-------|-----------|------|
| C_IN | `Device:C` | ~(115, 115) | 10µF 50V | `Capacitor_SMD:C_1206_3216Metric` | C13585 |
| C1 | `Device:C` | ~(160, 130) | 1µF 50V | `Capacitor_SMD:C_0603_1608Metric` | C15849 |
| C2 | `Device:C` | ~(170, 115) | 10µF 50V | `Capacitor_SMD:C_1206_3216Metric` | C13585 |

### 3.4 Resistências

| Ref | Símbolo | Posição | Value | Footprint | LCSC |
|-----|---------|---------|-------|-----------|------|
| R_SEL | `Device:R` | ~(165, 140) | 100kΩ | `Resistor_SMD:R_0402_1005Metric` | C25741 |
| R_CC1 | `Device:R` | ~(115, 145) | 5.1kΩ | `Resistor_SMD:R_0402_1005Metric` | C25905 |
| R_CC2 | `Device:R` | ~(115, 155) | 5.1kΩ | `Resistor_SMD:R_0402_1005Metric` | C25905 |

### 3.5 J_BYPASS (Jumper)

1. Pressiona **A**
2. Procura: `Connector_Generic:Conn_01x02`
3. Coloca em **~(135, 95)**
4. Pressiona **E** e define:
   - **Reference**: `J_BYPASS`
   - **Value**: `Bypass 5V`
   - **Footprint**: `Connector_PinHeader_2.54mm:PinHeader_1x02_P2.54mm_Vertical`
   - **LCSC**: `C36717`

---

## Passo 4: Fazer Ligações

### 4.1 Fluxo Principal VBUS

```
J1:VBUS (A4,A9,B4,B9) → F1:1
F1:2 → C_IN → GND
F1:2 → U1:VBUS (pin 16)
F1:2 → Q1:D (Drain) → J_BYPASS:1
```

### 4.2 Ligações IP2721

```
U1:VBUSG (pin 4) → Q1:G (Gate)
U1:VIN (pin 1) → Q1:S (Source)
U1:VIN (pin 1) → C1 → GND
U1:SEL (pin 7) → R_SEL → U1:VIN (100kΩ pull-up)
U1:GND (pins 5,6,14,15) → GND
U1:CC1 (pin 12) → J1:CC1 (A5)
U1:CC2 (pin 13) → J1:CC2 (B5)
```

### 4.3 Resistências CC

```
J1:CC1 → R_CC1 → GND
J1:CC2 → R_CC2 → GND
```

### 4.4 Saída 20V

```
Q1:S (Source) → C2 → GND
Q1:S (Source) → J_BYPASS:2
Q1:S (Source) → Label "20V_OUT" (para próximo bloco)
```

### 4.5 Bypass

```
J_BYPASS:1 → F1:2 (antes do MOSFET)
J_BYPASS:2 → Q1:S (depois do MOSFET)
```

---

## Passo 5: Adicionar Labels

Adiciona Global Labels para clareza:

1. **VBUS_USB** - Saída do F1
2. **20V_OUT** - Saída do Q1 (para Buck converter)
3. **CC1** - Linha CC1 do USB-C
4. **CC2** - Linha CC2 do USB-C

---

## Diagrama de Referência

```
                          J_BYPASS
                     ┌───[○ ○]───┐
                     │           │
USB-C VBUS ──[F1]────┼───► Q1:D  │
              │      │      │    │
              │      │      G    │
              │      │      │    │
             ═╧═    IP2721:VBUSG │
            C_IN         │       │
              │          │       │
             GND    ─────┴───────┴──► Q1:S ──► 20V_OUT
                              │            │
                         IP2721:VIN       ═╧═ C1+C2
                              │            │
                         [R_SEL]          GND
                              │
                             GND

USB-C CC1 ──┬──► IP2721:CC1
            │
           [R_CC1] 5.1kΩ
            │
           GND

USB-C CC2 ──┬──► IP2721:CC2
            │
           [R_CC2] 5.1kΩ
            │
           GND
```

---

## Verificação Final

- [ ] F1 com LCSC C2982291, footprint 2920
- [ ] IP2721 com todos os GND ligados (pins 5,6,14,15)
- [ ] AO3400A com Gate, Source, Drain corretos
- [ ] C_IN antes do MOSFET, C1+C2 depois
- [ ] R_SEL ligada entre SEL e VIN
- [ ] R_CC1 e R_CC2 ligadas a GND
- [ ] J_BYPASS permite curto-circuitar D-S do MOSFET
- [ ] Todos os LCSC codes definidos

---

*Próximo: Bloco 2 - Buck Converter SY8368*
