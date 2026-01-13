# PCB Debug Checklist - Primeiro Teste

Checklist para verificar se um PCB está funcional ao receber do fornecedor (JLCPCB, PCBWay, etc.)

---

## Ferramentas Necessárias

- Multímetro digital
- Cabo USB-C
- Computador com terminal série (PuTTY, screen, minicom, Arduino IDE)

---

## Passo 1: Inspecção Visual (sem ligar)

| Verificação | OK |
|-------------|:--:|
| Sem pontes de solda visíveis | ☐ |
| Componentes na orientação correcta (polaridade ICs, LEDs, díodos) | ☐ |
| Todos os componentes presentes | ☐ |
| Soldas brilhantes e côncavas (não foscas ou em bola) | ☐ |
| PCB sem riscos ou danos visíveis | ☐ |

---

## Passo 2: Teste de Continuidade (sem ligar)

> ⚠️ **PCB desligado!** Multímetro em modo continuidade (beep) ou resistência (Ω)

| Teste | Esperado | Resultado |
|-------|----------|:---------:|
| VBUS ↔ GND | ABERTO (>1MΩ, não apita) | ☐ |
| 5V ↔ GND | ABERTO (>1MΩ, não apita) | ☐ |
| 3V3 ↔ GND | ABERTO (>1MΩ, não apita) | ☐ |

> 🔴 **Se algum apitar = CURTO!** Não ligar. Procurar ponte de solda ou componente defeituoso.

---

## Passo 3: Teste de Alimentação (ligar USB)

> Ligar a um carregador USB básico (não ao computador) para primeiro teste

| Medição | Esperado | Resultado |
|---------|----------|:---------:|
| Rail VBUS/5V | 4.9V - 5.1V | ☐ ___V |
| Saída LDO (3V3) | 3.2V - 3.4V | ☐ ___V |
| Pino 3V3 do ESP32 | 3.2V - 3.4V | ☐ ___V |
| Consumo em idle | < 100mA | ☐ |

> 🔴 **Se não há 3.3V:** Problema no LDO, curto no rail 3V3, ou ESP32 em curto.
>
> 🔴 **Se consumo > 500mA:** Curto-circuito algures. Desligar imediatamente.

---

## Passo 4: Reconhecimento USB (ligar ao PC)

| Sistema | Comando/Verificação |
|---------|---------------------|
| **macOS** | `ls /dev/cu.usb*` |
| **Linux** | `ls /dev/ttyUSB*` ou `ls /dev/ttyACM*` |
| **Windows** | Gestor de Dispositivos → Portas (COM & LPT) |

| Verificação | OK |
|-------------|:--:|
| Porta série aparece (ex: `/dev/cu.usbserial-XXX` ou `COM3`) | ☐ |
| Identificada como "CH340" ou similar | ☐ |

> 🔴 **Se não aparece:**
> - CH340C mal soldado
> - Problema nas linhas D+/D-
> - Driver CH340 não instalado (Windows)

---

## Passo 5: ESP32 Responde

1. Abrir terminal série:
   ```bash
   # macOS/Linux
   screen /dev/cu.usbserial-XXX 115200

   # Ou usar Arduino IDE → Serial Monitor → 115200 baud
   ```

2. Premir botão **EN** (reset)

3. Verificar output do bootloader:

```
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
load:0x3fff0030,len:XXX
...
```

| Verificação | OK |
|-------------|:--:|
| Bootloader responde após reset | ☐ |
| Sem mensagens de erro ou crash loops | ☐ |

> 🔴 **Se não responde:**
> - ESP32 mal soldado
> - Problema no cristal/oscilador
> - Flash corrompida (raro em módulos novos)

---

## Passo 6: Teste de Programação

1. Premir e segurar **BOOT**
2. Premir e soltar **EN** (mantendo BOOT)
3. Soltar **BOOT**
4. Upload de código de teste:

```cpp
// Blink test
void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);  // LED interno (se existir)
}

void loop() {
  Serial.println("Alive!");
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  delay(500);
}
```

| Verificação | OK |
|-------------|:--:|
| Upload completa sem erros | ☐ |
| "Alive!" aparece no terminal | ☐ |
| LED pisca (se aplicável) | ☐ |

---

## Resumo Rápido

| Fase | Teste | Ferramenta | OK = |
|------|-------|------------|------|
| 1 | Visual | Olhos/lupa | Sem defeitos visíveis |
| 2 | Curtos | Multímetro | Não apita |
| 3 | Tensões | Multímetro | 5V e 3.3V correctos |
| 4 | USB | PC | Porta série aparece |
| 5 | Bootloader | Terminal | ESP32 responde |
| 6 | Programar | IDE | Upload OK |

---

## Se Falhar

| Sintoma | Causa Provável | Acção |
|---------|----------------|-------|
| Curto VBUS↔GND | Ponte solda, TVS em curto | Inspeccionar, remover TVS |
| Sem 3.3V | LDO defeituoso ou curto | Verificar LDO, medir consumo |
| USB não reconhecido | CH340C, D+/D- | Re-soldar CH340C |
| ESP32 não responde | Soldadura, alimentação | Verificar 3.3V no ESP32 |
| Upload falha | Auto-reset, BOOT/EN | Usar método manual de boot |

---

## Notas

- Primeiro teste sempre com carregador USB descartável (não MacBook!)
- Ter sempre DRC = 0 antes de fabricar
- Visualizar 3D no KiCad antes de encomendar
- Guardar este checklist junto com o projecto

---

*Criado: Janeiro 2026*
*Baseado em: Debug do clockv2 PCB*
