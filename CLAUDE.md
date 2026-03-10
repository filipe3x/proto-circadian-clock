# Proto Circadian Clock - Regras de Contexto

## Sintaxe easyeda2kicad
```
python3 -m easyeda2kicad --symbol --footprint --3d --lcsc_id=CXXXXXX
```

## 1. KiCad - READ ONLY
- A pasta `product/kicad/` contém o esquemático do custom PCB encomendado.
- Este esquemático é **apenas um guia de leitura** — NUNCA escrever ou alterar ficheiros KiCad.
- O conteúdo pode estar desatualizado relativamente a commits mais recentes.

## 2. Adição de novos sensores / redes de componentes
Ao pedir para adicionar sensores ou redes de componentes novos, seguir sempre esta ordem:

### a) Componentes JLCPCB
- Sugerir sempre componentes **JLCPCB Basic** ou **Promo Extended** para minimizar custos.

### b) Ligações pin-a-pin (sem alterar KiCad)
- Consultar o estado atual do esquemático e a documentação da rede (se existir).
- Sugerir como ligar **pin a pin** cada componente novo, incluindo pitfalls conhecidos.

### c) Footprints em falta
- Se o footprint do componente ainda não existir no esquema atual, fornecer o comando correto:
```
python3 -m easyeda2kicad --symbol --footprint --3d --lcsc_id=CXXXXXX
```
(substituindo `CXXXXXX` pelo LCSC ID real do componente)

### d) Instruções detalhadas para KiCad
- Fornecer sempre ligações **passo a passo** para executar no KiCad, incluindo:
  - Distância recomendada entre componentes
  - Espessura dos traces
  - Decoupling capacitors necessários

### e) Reutilização de componentes
- Procurar sempre **reutilizar componentes já presentes na board** para evitar a taxa extra de assembly de novo reel na JLCPCB.

## 3. Modo Tutor
- Quando for necessário ligar **mais de 3 componentes** com vários pins associados, entrar em **modo tutor**:
  - Avançar passo a passo
  - Pedir autorização antes de avançar para o próximo passo

## 4. Firmware ESP32
- Buscar as bibliotecas ESP32 necessárias e dar **exemplos de código** para operacionalizar os novos componentes.
- Assumir sempre que estamos a usar um chip **ESP32-E 8MB**.
- **Ignorar o Adafruit Matrix S3** que ainda está no código — será brevemente removido para simplificar.
