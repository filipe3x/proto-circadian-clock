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

### 3.5 J_MODE (Jumper Seletor de Modo)

1. Pressiona **A**
2. Procura: `Connector_Generic:Conn_01x03`
3. Coloca perto da saída (após Buck)
4. Pressiona **E** e define:
   - **Reference**: `J_MODE`
   - **Value**: `Mode Select`
   - **Footprint**: `Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical`
   - **LCSC**: `C2337`

**Ligações J_MODE (configuração elegante):**
| Pino | Liga a | Função |
|------|--------|--------|
| 1 | F1 saída | Bypass direto (5V USB) |
| 2 | 5V cargas | Saída comum |
| 3 | Buck VOUT | Saída regulada |

**Modos:**
- `[○ ■ ■]` Pinos 2-3: Normal (Buck → cargas)
- `[■ ■ ○]` Pinos 1-2: Bypass (F1 → cargas, 5V limpos)

### 3.6 D1 - TVS (Proteção Overvoltage)

1. Pressiona **A**
2. Procura: `Device:D_TVS`
3. Coloca perto de J_MODE:2 (saída) **~(180, 115)**
4. Pressiona **E** e define:
   - **Reference**: `D1`
   - **Value**: `SMBJ5.0A`
   - **Footprint**: `Diode_SMD:D_SMB`
   - **LCSC**: `C19077558`

**Função**: Protege as cargas se alguém usar bypass (1-2) com fonte PD (20V). O TVS limita a tensão e F1 dispara.

---

## Passo 4: Fazer Ligações

### 4.1 Fluxo Principal VBUS

```
J1:VBUS (A4,A9,B4,B9) → F1:1
F1:2 → C_IN → GND
F1:2 → U1:VBUS (pin 16)
F1:2 → Q1:D (Drain)
F1:2 → J_MODE:1
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

### 4.4 Ligações do Buck (fixas, não passam pelo J_MODE)

```
Q1:S (Source) → Buck VIN
Q1:S (Source) → C1 → GND
```

### 4.5 Ligações J_MODE (saída)

```
F1:2 (saída fuse) → J_MODE:1
J_MODE:2 → 5V cargas
J_MODE:2 → C2 → GND
J_MODE:2 → D1 (catodo) → GND (anodo)
Buck VOUT → J_MODE:3
```

### 4.5.1 Proteção TVS

```
J_MODE:2 (saída) ──┬──► C2 ──► GND
                   │
                   ├──► 5V cargas
                   │
                   └──► D1 (catodo)
                              │
                             GND (anodo)
```

### 4.6 Modos J_MODE

```
Modo Normal [○ ■ ■] (jumper 2-3): Buck VOUT → cargas (5V regulado)
Modo Bypass [■ ■ ○] (jumper 1-2): F1 → cargas (5V USB direto)
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
                                              J_MODE
                                          ┌──[○ ○ ○]──┐
                                          │   1 2 3   │
                                          │   │ │ │   │
USB-C VBUS ──[F1]──┬──────────────────────┴───┘ │ │   │
              │    │                            │ │   │
              │    └──► Q1:D                    │ │   │
              │           │                     │ │   │
             ═╧═       Q1:Gate                  │ │   │
            C_IN          │                     │ │   │
              │    IP2721:VBUSG                 │ │   │
             GND          │                     │ │   │
                          │                     │ │   │
                       Q1:S ──► Buck VIN        │ │   │
                          │         │           │ │   │
                    IP2721:VIN      │      Buck VOUT   │
                          │         │           │     │
                         ═╧═ C1     │           └─────┘
                          │         │                 │
                         GND        │                 ▼
                                    │            5V cargas
                [R_SEL]             │                 │
            SEL ──┴── VIN           │                ═╧═ C2
                                    │                 │
                                    │                GND
                                    │
                               (ligação fixa)

USB-C CC1 ──┬──► IP2721:CC1      USB-C CC2 ──┬──► IP2721:CC2
            │                                 │
           [R_CC1] 5.1kΩ                    [R_CC2] 5.1kΩ
            │                                 │
           GND                               GND
```

**Modos de operação:**
- Jumper **2-3** `[○ ■ ■]`: Normal (PD → Q1 → Buck → 5V regulado)
- Jumper **1-2** `[■ ■ ○]`: Bypass (5V USB → cargas, **5V limpos!**)

---

## Verificação Final

- [ ] F1 com LCSC `C2982291`, footprint `Fuse_2920_7451Metric`
- [ ] IP2721 com todos os GND ligados (pins 5,6,14,15)
- [ ] AO3400A: G←IP2721:VBUSG, D←F1, S→Buck VIN+IP2721:VIN
- [ ] C_IN entre F1 saída e GND
- [ ] C1 entre Q1:S/IP2721:VIN e GND
- [ ] C2 entre J_MODE:2 (saída) e GND
- [ ] R_SEL entre IP2721:SEL e IP2721:VIN
- [ ] R_CC1 e R_CC2 entre CC e GND
- [ ] J_MODE: pino 1←F1, pino 2→cargas, pino 3←Buck VOUT
- [ ] D1 (TVS) entre J_MODE:2 (catodo) e GND (anodo)
- [ ] Todos os LCSC codes definidos

---

*Próximo: Bloco 2 - Buck Converter SY8368*
