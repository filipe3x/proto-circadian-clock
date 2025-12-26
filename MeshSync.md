# Proto Circadian Clock - Mesh Sync

Sistema de sincronização automática entre múltiplos dispositivos Proto Circadian Clock usando ESP-NOW.

## Visão Geral

Quando tens vários Proto Circadian Clocks na mesma casa/espaço, eles sincronizam-se automaticamente:
- **Mudança de modo instantânea** - Quando mudas o modo num dispositivo, todos os outros mudam imediatamente
- **Descoberta automática** - Novos dispositivos são detectados automaticamente
- **Sem configuração** - Funciona out-of-the-box, sem router WiFi necessário

## Como Funciona

### Device ID Único

Cada ESP32 tem um identificador único de fábrica (eFuse MAC):
```
Chip ID: A4:CF:12:XX:XX:XX
```
Este ID é gravado no chip durante o fabrico e nunca muda.

### Timestamp de Primeira Instalação

Na primeira vez que o dispositivo arranca, guarda um timestamp em memória não-volátil (NVS):
```cpp
myFirstBootTime = now.unixtime() - 1577836800;  // Segundos desde 2020
```
Este timestamp determina a "antiguidade" do dispositivo.

### Eleição de Master

O dispositivo **mais antigo** (com o `firstBootTime` mais baixo) torna-se automaticamente o **Master**:

```
Dispositivo A: firstBootTime = 1000  → MASTER
Dispositivo B: firstBootTime = 2000  → SLAVE
Dispositivo C: firstBootTime = 3000  → SLAVE
```

Se o Master desaparecer (desligado >5 min), o próximo mais antigo assume.

## Protocolo ESP-NOW

### Porquê ESP-NOW?

| Característica | ESP-NOW | WiFi tradicional |
|----------------|---------|------------------|
| Necessita router | Não | Sim |
| Latência | <10ms | 50-200ms |
| Alcance | ~200m | ~50m |
| Consumo | Baixo | Alto |
| Setup | Zero | Configuração |

### Tipos de Mensagem

```cpp
MSG_DISCOVERY  // Anúncio de presença (broadcast a cada 30s)
MSG_SETTINGS   // Definições + modo atual
MSG_REQUEST    // Pedido de definições (novo dispositivo)
MSG_ACK        // Confirmação de receção
```

### Estrutura da Mensagem

```cpp
struct MeshMessage {
  uint32_t magic;          // "PCLK" - identifica mensagens nossas
  uint8_t version;         // Versão do protocolo
  uint8_t msgType;         // Tipo de mensagem
  uint8_t senderId[6];     // ID do remetente
  uint32_t firstBootTime;  // Antiguidade do dispositivo
  uint16_t msgId;          // ID para ACKs
  SyncSettings payload;    // Dados (modo, lat, lon, etc)
};
```

## Fluxo de Sincronização

### 1. Descoberta

```
┌──────────────────────────────────────────────┐
│  Dispositivo A (novo) arranca                │
│                                              │
│  A → Broadcast DISCOVERY ─────────────────►  │
│                           ┌─────────────┐    │
│                           │ B (master)  │    │
│                           │ C (slave)   │    │
│                           └─────────────┘    │
│                                              │
│  A ← DISCOVERY de B e C ◄─────────────────   │
│  A regista B e C como peers                  │
│  A determina: B é mais antigo → B é master   │
│  A → REQUEST settings a B ─────────────────► │
│  A ← SETTINGS de B ◄───────────────────────  │
└──────────────────────────────────────────────┘
```

### 2. Mudança de Modo (Sync Imediato)

```
┌──────────────────────────────────────────────┐
│  Utilizador pressiona botão no dispositivo C │
│                                              │
│  C: currentMode = THERAPY_RED                │
│  C → Broadcast SETTINGS (modo=THERAPY_RED)   │
│                                              │
│  A recebe: aplica modo imediatamente         │
│  B recebe: aplica modo imediatamente         │
│                                              │
│  A → ACK (com backoff aleatório)             │
│  B → ACK (com backoff aleatório)             │
│                                              │
│  Resultado: Todos em THERAPY_RED em <50ms    │
└──────────────────────────────────────────────┘
```

### 3. Prevenção de Colisões (ACKs)

Quando múltiplos dispositivos recebem um broadcast, usam **backoff baseado no ID** para evitar colisões:

```cpp
uint8_t slot = deviceId[5] % 10;  // 0-9 baseado no ID
int backoff = 10 + (slot * 15) + random(0, 20);  // 10-170ms
```

```
t=0ms    Master envia SETTINGS
t=25ms   Slave A (slot 1) → ACK
t=55ms   Slave B (slot 3) → ACK
t=85ms   Slave C (slot 5) → ACK
         ↑ Sem colisões!
```

## Definições Sincronizadas

| Definição | Sync imediato | Fonte |
|-----------|---------------|-------|
| Modo (AUTO/RED/OFF) | Sim | Qualquer dispositivo |
| Latitude | Eventual | Só master |
| Longitude | Eventual | Só master |
| Timezone | Eventual | Só master |
| Solar Offset | Eventual | Só master |

**Nota:** Atualmente latitude/longitude/timezone são constantes no código. Para torná-los dinâmicos seria necessário:
1. Converter para variáveis
2. Guardar em NVS
3. Implementar captive portal para configuração

## Debug Serial

O sistema produz logs detalhados via Serial (115200 baud):

```
=== DEVICE ID ===
Chip ID: A4:CF:12:XX:XX:XX
Raw: 0xXXXXXXXXXXXX
Primeira instalacao! Timestamp: 157234567

=== MESH SYNC ===
[MESH] ESP-NOW inicializado
[MESH] Device ID: A4:CF:12:XX:XX:XX
[MESH] First Boot: 157234567
[MESH] Estado: MASTER (default)
[MESH] Discovery broadcast (age: 157234567, peers: 0)

[MESH] Novo peer: B8:27:EB:XX:XX:XX (age: 157234590)
[MESH] Peer ESP-NOW adicionado
[MESH] Estado alterado: MASTER

[MESH] Mode change broadcast: 1
[MESH] Modo sync: AUTO_SOLAR -> THERAPY_RED
```

## Limitações

1. **Máximo 10 peers** - Pode ser aumentado em `MAX_PEERS`
2. **Canal fixo (1)** - Todos os dispositivos devem usar o mesmo canal
3. **Sem encriptação** - Mensagens não são cifradas (podia adicionar-se)
4. **Alcance ~200m** - Linha de vista; paredes reduzem significativamente

## Consumo de Energia

O ESP-NOW é eficiente, mas ainda assim consome:
- **Broadcast a cada 30s** - Pode ser aumentado se a bateria for problema
- **WiFi em modo STA** - Necessário para ESP-NOW funcionar
- **Desliga após NTP sync** - O WiFi infrastructure desliga, mas ESP-NOW continua

## Extensões Futuras

- [ ] Encriptação de mensagens (ESP-NOW suporta CCMP)
- [ ] Modo "follow the sun" - Um dispositivo partilha elevação solar calculada
- [ ] Sync de brightness individual
- [ ] OTA update coordenado entre dispositivos
- [ ] Heartbeat visual (LED pisca quando recebe de peer)
