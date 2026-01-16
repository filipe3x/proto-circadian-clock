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

# Guia KiCad - Bloco 2: Buck Converter SY8368AQQC

## Passo 0: Verificar Componentes

Antes de começar, confirma que tens estes componentes no esquema:

| Ref | Componente | Verificação |
|-----|------------|-------------|
| U2 | SY8368AQQC | ☐ Presente |
| L1 | Indutor 2.2µH | ☐ Presente |
| C_VIN1, C_VIN2 | 22µF 25V | ☐ Presentes |
| C_OUT1-4 | 22µF 10V (×4) | ☐ Presentes |
| C_BOOT | 100nF | ☐ Presente |
| C_SS | 10nF | ☐ Presente |
| R_FB1 | 22kΩ | ☐ Presente |
| R_FB2 | 3kΩ | ☐ Presente |

---

## Passo 1: Layout Sugerido Bloco 2

Posiciona os componentes assim (coordenadas aproximadas):

```
                    VIN (do Q1:Source)
                          │
                          ▼
    ┌─────────────────────┼─────────────────────┐
    │                     │                     │
   ═╧═                   ═╧═                    │
  C_VIN1               C_VIN2                   │
 (200,95)             (200,105)                 │
    │                     │                     │
    └──────────┬──────────┘                     │
               │                                │
               ▼                                │
        ┌──────────────┐                        │
        │   U2         │                        │
        │  SY8368      │◄───────────────────────┘
        │  (210,100)   │
        │              │
        └──────┬───────┘
               │ SW
               ▼
              ═╪═ L1 ═╪═ ──────► VOUT (5V)
           (230,100)      │
                          │
               ┌──────────┼──────────┐
               │          │          │
              ═╧═        ═╧═        ═╧═
            C_OUT1     C_OUT2     C_OUT3
           (245,95)   (245,100)  (245,105)
               │          │          │
              GND        GND        GND
```

---

## Passo 2: Ligação 1 - Entrada VIN

**Esta é a primeira ligação. Vai devagar!**

### 2.1 Ligar VIN aos condensadores de entrada

1. Pressiona **W** para começar um fio (wire)
2. Clica no pino **VIN** do U2 (pinos 19 e 20)
3. Arrasta até ao terminal **+** do C_VIN1
4. Clica para terminar

```
Ligação:
U2:VIN (pinos 19,20) ──► C_VIN1 (+)
                    └──► C_VIN2 (+)
```

### 2.2 Ligar condensadores ao GND

1. Pressiona **W**
2. Clica no terminal **-** do C_VIN1
3. Pressiona **Ctrl+H** para adicionar símbolo GND (ou **P** → `power:GND`)
4. Repete para C_VIN2

```
C_VIN1 (-) ──► GND
C_VIN2 (-) ──► GND
```

### 2.3 Ligar VIN à entrada do Bloco 1

1. Pressiona **L** para adicionar Global Label
2. Escreve: `20V_OUT`
3. Coloca junto ao nó VIN
4. Este label liga ao Q1:Source do Bloco 1

**✓ Checkpoint**: VIN tem 2 condensadores e um label `20V_OUT`

---

## Passo 3: Ligação 2 - Enable (EN)

**O EN liga-se diretamente ao VIN para always-on.**

1. Pressiona **W**
2. Liga U2:EN (pino 18) ao nó VIN

```
U2:EN (pino 18) ──► VIN (mesmo nó dos condensadores)
```

**Nota**: Se quiseres controlo externo, liga a um GPIO com pull-up.

---

## Passo 4: Ligação 3 - Bootstrap (BOOT)

**Crítico! O C_BOOT liga BOOT ao SW.**

1. Posiciona C_BOOT (100nF) entre os pinos BOOT e SW
2. Pressiona **W**
3. Liga U2:BOOT (pino 1) → C_BOOT terminal 1
4. Liga C_BOOT terminal 2 → U2:SW (pinos 2,3,4)

```
U2:BOOT (pino 1) ──[C_BOOT 100nF]──► U2:SW (pinos 2,3,4)
```

**⚠️ IMPORTANTE**: C_BOOT NÃO vai ao GND! Liga BOOT a SW.

---

## Passo 5: Ligação 4 - Soft-Start (SS)

1. Posiciona C_SS (10nF) perto do pino SS
2. Pressiona **W**
3. Liga U2:SS (pino 17) → C_SS terminal 1
4. Liga C_SS terminal 2 → GND

```
U2:SS (pino 17) ──[C_SS 10nF]──► GND
```

---

## Passo 6: Ligação 5 - MODE

**MODE = GND força modo CCM (melhor para cargas constantes).**

1. Pressiona **W**
2. Liga U2:MODE (pino 11) → GND

```
U2:MODE (pino 11) ──► GND
```

---

## Passo 7: Ligação 6 - Grounds (PGND e SGND)

**Múltiplos pinos de GND - liga todos!**

### 7.1 Power Ground (PGND)

1. Pressiona **W**
2. Liga cada pino PGND ao GND:
   - U2:PGND (pinos 5, 6, 7, 8, 9) → GND

### 7.2 Signal Ground (SGND)

1. Liga pinos SGND ao GND:
   - U2:SGND (pinos 12, 13) → GND

### 7.3 Thermal Pad

1. Liga U2:PAD (centro) → GND

```
U2:PGND (5,6,7,8,9) ──┬──► GND
U2:SGND (12,13) ──────┤
U2:PAD ───────────────┘
```

**✓ Checkpoint**: Todos os grounds ligados (10 conexões ao GND)

---

## Passo 8: Ligação 7 - Switch (SW) e Indutor

**Esta é a ligação de potência principal!**

1. Pressiona **W**
2. Liga U2:SW (pinos 2,3,4) → L1 terminal 1
3. Liga L1 terminal 2 → nó VOUT

```
U2:SW (pinos 2,3,4) ──► L1 (terminal 1)
                              │
                       L1 ~~~~│~~~~ (2.2µH)
                              │
                       L1 (terminal 2) ──► VOUT
```

**Nota**: O C_BOOT já está ligado a SW (do Passo 4).

---

## Passo 9: Ligação 8 - Condensadores de Saída

1. Posiciona C_OUT1, C_OUT2, C_OUT3, C_OUT4 em paralelo após L1
2. Liga todos ao nó VOUT:

```
L1 saída ──┬──► C_OUT1 (+) ──► GND
           ├──► C_OUT2 (+) ──► GND
           ├──► C_OUT3 (+) ──► GND
           └──► C_OUT4 (+) ──► GND
```

3. Adiciona Global Label `5V_OUT` no nó VOUT

---

## Passo 10: Ligação 9 - Divisor Feedback (CRÍTICO!)

**Este divisor define a tensão de saída = 5V**

### 10.1 Posicionar resistências

```
VOUT (5V) ───┬─── para cargas
             │
           [R_FB1]
            22kΩ
             │
             ├───► U2:FB (pino 14)
             │
           [R_FB2]
             3kΩ
             │
            GND
```

### 10.2 Fazer ligações

1. Liga VOUT → R_FB1 (terminal superior)
2. Liga R_FB1 (terminal inferior) → R_FB2 (terminal superior)
3. Liga R_FB1/R_FB2 junção → U2:FB (pino 14)
4. Liga R_FB2 (terminal inferior) → GND

```
VOUT ──► R_FB1 ──┬──► U2:FB (pino 14)
                 │
                 R_FB2
                 │
                GND
```

**⚠️ VERIFICAÇÃO**:
- R_FB1 = 22kΩ 0603 (**C31850** Basic)
- R_FB2 = 3kΩ 0603 (**C4211** Basic)
- Footprint: `Resistor_SMD:R_0603_1608Metric`
- Resultado: 0.6V × (1 + 22/3) = **5.0V** ✓

---

## Passo 11: Ligação 10 - COMP (Compensação)

O pino COMP pode ficar NC (não conectado) se o circuito for estável.

**Opção simples**: Deixar NC (não ligar nada)

**Opção com rede RC** (se houver oscilação):
```
U2:COMP ──[R_COMP 10kΩ]──┬──[C_COMP 100pF]──► GND
                         │
                    [C_COMP2 10nF]
                         │
                        GND
```

Para começar, deixa **NC** e testa.

---

## Passo 12: Ligação 11 - PGOOD (Opcional)

O PGOOD indica quando a saída está estável.

**Opção 1**: Não ligar (NC)

**Opção 2**: Ligar LED indicador
```
U2:PGOOD ──[R 1kΩ]──► LED ──► GND
```

**Opção 3**: Ligar a MCU para monitorização
```
U2:PGOOD ──► GPIO do MCU
```

---

## Passo 13: Ligação Final - VOUT ao J_MODE

Liga a saída do Buck ao J_MODE:3 do Bloco 1:

1. Adiciona Global Label `5V_OUT` na saída do Buck (se não fizeste)
2. No Bloco 1, liga `5V_OUT` ao J_MODE:3

```
Buck VOUT ──[Label: 5V_OUT]──► J_MODE:3
```

---

## Diagrama Completo Bloco 2

```
        VIN (20V_OUT do Bloco 1)
               │
    ┌──────────┼──────────┐
    │          │          │
   ═╧═        ═╧═         │
  C_VIN1    C_VIN2        │
  22µF      22µF          │
    │          │          │
   GND        GND         │
                          │
    ┌─────────────────────┤
    │                     │
    │  ┌─────────────────────────────────────────┐
    │  │             SY8368AQQC                  │
    │  │                                         │
    ├──┤ VIN (19,20)                  SW (2,3,4) ├──┐
    │  │                                         │  │
    ├──┤ EN (18)                                 │  │
    │  │                    ┌──[C_BOOT 100nF]────┤  │
    │  │ BOOT (1) ──────────┘                    │  │
    │  │                                         │  │
    │  │ SS (17) ──────[C_SS 10nF]──► GND       │  │
    │  │                                         │  │
    │  │ MODE (11) ─────────────────► GND       │  │
    │  │                                         │  │
    │  │ FB (14) ◄──────────────────────────────│──│──┐
    │  │                                         │  │  │
    │  │ COMP (15) ── NC                        │  │  │
    │  │                                         │  │  │
    │  │ PGOOD (16) ── NC                       │  │  │
    │  │                                         │  │  │
    │  │ PGND (5-9) ────────────────► GND       │  │  │
    │  │ SGND (12,13) ──────────────► GND       │  │  │
    │  │ PAD ───────────────────────► GND       │  │  │
    │  │                                         │  │  │
    │  └─────────────────────────────────────────┘  │  │
    │                                               │  │
   GND                        ┌─────────────────────┘  │
                              │                        │
                              │  L1 (2.2µH)            │
                             ═╪═ ~~~~~ ═╪═             │
                              │         │              │
                              │    ┌────┴────┬────┐   │
                              │    │         │    │   │
                             ═╧═  ═╧═       ═╧═  ═╧═  │
                           C_OUT1 C_OUT2  C_OUT3 C_OUT4
                           22µF   22µF    22µF   22µF │
                              │    │         │    │   │
                             GND  GND       GND  GND  │
                                                      │
                              ┌────────────────────────┤
                              │                        │
                         VOUT (5V) ◄───────────────────┤
                              │                        │
                              │                   [R_FB1] 22kΩ
                              │                        │
                              ├──► J_MODE:3       FB ◄─┤
                              │                        │
                              │                   [R_FB2] 3kΩ
                              │                        │
                             ═╧═                      GND
                           C_OUT
                              │
                             GND
```

---

## Verificação Final Bloco 2

- [ ] U2:VIN (19,20) → C_VIN1, C_VIN2 → GND
- [ ] U2:VIN → Label `20V_OUT` (entrada do Bloco 1)
- [ ] U2:EN (18) → VIN (always-on)
- [ ] U2:BOOT (1) → C_BOOT (100nF) → U2:SW
- [ ] U2:SS (17) → C_SS (10nF) → GND
- [ ] U2:MODE (11) → GND
- [ ] U2:PGND (5-9) → GND
- [ ] U2:SGND (12,13) → GND
- [ ] U2:PAD → GND
- [ ] U2:SW (2,3,4) → L1 → VOUT
- [ ] VOUT → C_OUT1-4 → GND
- [ ] VOUT → R_FB1 (22kΩ C31850) → FB → R_FB2 (3kΩ C4211) → GND
- [ ] U2:FB (14) → junção R_FB1/R_FB2
- [ ] VOUT → Label `5V_OUT` → J_MODE:3
- [ ] Todos os LCSC codes definidos

---

## Tabela Resumo de Ligações

| De | Para | Componente | Notas |
|----|------|------------|-------|
| 20V_OUT (Q1:S) | U2:VIN | - | Entrada 9-20V |
| U2:VIN | C_VIN1, C_VIN2 | 22µF ×2 | Filtro entrada |
| U2:EN | VIN | - | Always-on |
| U2:BOOT | C_BOOT | 100nF | → SW |
| C_BOOT | U2:SW | - | Bootstrap |
| U2:SS | C_SS | 10nF | → GND |
| U2:MODE | GND | - | CCM forçado |
| U2:PGND (5-9) | GND | - | Power ground |
| U2:SGND (12,13) | GND | - | Signal ground |
| U2:PAD | GND | - | Thermal |
| U2:SW | L1 | 2.2µH | Nó comutação |
| L1 | VOUT | - | Saída |
| VOUT | C_OUT1-4 | 22µF ×4 | Filtro saída |
| VOUT | R_FB1 | 22kΩ | Divisor |
| R_FB1 | R_FB2 + FB | 3kΩ | Junção |
| R_FB2 | GND | - | Ref 0.6V |
| VOUT | J_MODE:3 | - | Via label |

---

*Documento atualizado: Janeiro 2025*
*Bloco 1 + Bloco 2 completos*
