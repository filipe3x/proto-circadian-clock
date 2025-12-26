# Proto Circadian Clock - Mesh Sync

Sistema de sincronização automática entre múltiplos dispositivos Proto Circadian Clock usando ESP-NOW.

## Visão Geral

Quando tens vários Proto Circadian Clocks na mesma casa/espaço, eles sincronizam-se automaticamente:
- **Mudança de modo instantânea** - Quando mudas o modo num dispositivo, todos os outros mudam imediatamente
- **Descoberta automática** - Novos dispositivos são detectados automaticamente
- **Sem configuração** - Funciona out-of-the-box, sem router WiFi necessário
- **Economia de energia** - WiFi desliga-se automaticamente quando não há outros dispositivos

## Modo Híbrido (Economia de Energia)

O mesh usa uma **máquina de estados adaptativa** para poupar energia:

```
┌─────────────────────────────────────────────────────────────────┐
│                     MÁQUINA DE ESTADOS                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌──────────────┐                      ┌──────────────┐        │
│   │   MESH_OFF   │◄────────────────────►│ MESH_ACTIVE  │        │
│   │  (WiFi OFF)  │    encontrou peers   │  (WiFi ON)   │        │
│   │   ~0mA WiFi  │◄────────────────────►│   ~95mA      │        │
│   └──────┬───────┘    perdeu peers      └──────────────┘        │
│          │            (5 min timeout)          ▲                │
│          │ Timer 60s                           │                │
│          ▼                                     │                │
│   ┌──────────────┐                             │                │
│   │MESH_SCANNING │─────────────────────────────┘                │
│   │ (WiFi ON 3s) │     peers responderam                        │
│   │   ~95mA      │                                              │
│   └──────────────┘                                              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Configuração

```cpp
#define MESH_SCAN_INTERVAL_MS 60000   // Scan a cada 60s quando sozinho
#define MESH_SCAN_DURATION_MS 3000    // 3s de escuta durante scan normal
#define MESH_INITIAL_SCAN_MS 65000    // 65s no primeiro scan (garante descoberta)
#define MESH_IDLE_TIMEOUT_MS 300000   // 5 min sem peers = desligar mesh
```

### Scan Inicial Longo

Para garantir que dispositivos se encontram mesmo que os scans estejam dessincronizados:

```
Boot:     ████████████████████████████████████████████████████████████████░░░░░░███░░░░░░
          ←───────────────── 65s ────────────────────────────────────────→     ←3s→
          Primeiro scan (longo, > 1 ciclo)                                     Seguintes
```

**Porquê 65s?** É maior que o intervalo de scan (60s), garantindo que apanha pelo menos um ciclo completo de qualquer outro dispositivo na rede.

### Consumo por Estado

| Estado | WiFi | Consumo | Duração |
|--------|------|---------|---------|
| MESH_OFF | Desligado | ~20mA (só CPU+display) | 57s |
| MESH_SCANNING (inicial) | Ligado | ~95mA | 65s (só no boot) |
| MESH_SCANNING (normal) | Ligado | ~95mA | 3s |
| MESH_ACTIVE | Ligado | ~95mA | Contínuo |

**Consumo médio quando sozinho (após boot):**
```
(20mA × 57s + 95mA × 3s) / 60s = 23.75mA ≈ 24mA
```

**Comparação:**
| Cenário | Antes | Agora | Redução |
|---------|-------|-------|---------|
| 1 dispositivo sozinho | 95mA | 24mA | **75%** |
| 2+ dispositivos | 95mA | 95mA | 0% |

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

**Tamanho total:** ~32 bytes por mensagem

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

---

## Análise de Tráfego de Rede (Spam)

### Configuração Atual

| Parâmetro | Valor |
|-----------|-------|
| `BROADCAST_INTERVAL_MS` | 30.000ms (30s) |
| Tamanho mensagem | ~32 bytes |
| Timeout de peer | 300.000ms (5 min) |

### Tráfego por Dispositivo

| Tipo | Frequência | Quem envia | Bytes/msg |
|------|------------|------------|-----------|
| DISCOVERY | 30s | Todos | 32 |
| SETTINGS | 30s | Só master | 32 |
| ACK | Por SETTINGS | Slaves | 32 |

### Cálculo de Tráfego Total

**Cenário: 1 Master + 4 Slaves (5 dispositivos)**

```
Por ciclo de 30s:
├── 5× DISCOVERY (todos)     = 160 bytes
├── 1× SETTINGS (master)     = 32 bytes
├── 4× ACK (slaves)          = 128 bytes
└── Total por ciclo          = 320 bytes

Por hora:  320 × 120 = 38.4 KB
Por dia:   38.4 × 24 = 922 KB ≈ 0.9 MB
```

### Escala com Número de Dispositivos

| Dispositivos | Msgs/hora | KB/hora | MB/dia |
|--------------|-----------|---------|--------|
| 2 | 360 | 11.5 | 0.27 |
| 5 | 840 | 26.9 | 0.65 |
| 10 | 1560 | 49.9 | 1.20 |
| 20 | 2880 | 92.2 | 2.21 |

### Avaliação

| Métrica | Estado | Comentário |
|---------|--------|------------|
| Largura de banda | ✅ OK | <100 KB/hora é negligível |
| Colisões 802.11 | ⚠️ Baixo risco | Backoff mitiga |
| Interferência WiFi | ✅ OK | Canal 1 fixo, baixo duty cycle |
| Bateria (se aplicável) | ⚠️ Problema | Ver secção de energia |

### Otimizações Possíveis

#### Opção 1: Aumentar Intervalo de Broadcast
```cpp
#define BROADCAST_INTERVAL_MS 60000  // 60s em vez de 30s
```
**Impacto:** Reduz tráfego 50%, aumenta tempo de descoberta

#### Opção 2: Adaptive Broadcast
```cpp
// Se não há peers, broadcast mais frequente (descoberta)
// Se há peers estáveis, broadcast menos frequente
if (numPeers == 0) {
  interval = 10000;   // 10s - modo descoberta
} else if (allPeersStable) {
  interval = 120000;  // 2min - modo estável
} else {
  interval = 30000;   // 30s - modo normal
}
```

#### Opção 3: Eliminar ACKs para Broadcasts
Os ACKs para broadcasts periódicos são redundantes - a próxima mensagem serve como heartbeat implícito.
```cpp
// Em handleSettings:
// Não enviar ACK para broadcasts periódicos
// Só enviar ACK para mudanças de modo explícitas
```

#### Opção 4: Compressão de Discovery
Combinar DISCOVERY + SETTINGS numa só mensagem:
```cpp
// Reduz msgs/ciclo de 2 para 1 (master)
MSG_HEARTBEAT = DISCOVERY + modo atual
```

---

## Análise de Consumo de Energia

### Estados de Energia do ESP32

| Estado | Corrente | Descrição |
|--------|----------|-----------|
| Active (CPU + WiFi TX) | ~240mA | A transmitir |
| Active (CPU + WiFi RX) | ~95-100mA | A escutar |
| Modem Sleep | ~20mA | CPU ativo, WiFi off |
| Light Sleep | ~0.8mA | CPU pausado, WiFi off |
| Deep Sleep | ~10µA | Quase tudo off |

### Consumo Atual (Implementação Presente)

**Problema:** O ESP32 está em modo **WiFi STA contínuo** para ESP-NOW.

```
Estado atual:
├── WiFi RX sempre ativo: ~95mA contínuo
├── Picos de TX (30s): ~240mA × 5ms = desprezável
└── CPU ativo: incluído nos 95mA

Consumo médio: ~95mA
```

### Impacto em Bateria

| Bateria | Capacidade | Autonomia (95mA) |
|---------|------------|------------------|
| 18650 (1S) | 2500mAh | 26 horas |
| 2× 18650 | 5000mAh | 52 horas |
| USB powerbank 10Ah | 10000mAh | 4-5 dias |

**Conclusão:** Inviável para operação a bateria sem otimizações.

### Comparação: Com vs Sem Mesh

| Modo | Corrente média | Autonomia (2500mAh) |
|------|----------------|---------------------|
| Sem mesh (display only) | ~50mA | 50 horas |
| Com mesh (atual) | ~95mA | 26 horas |
| Com mesh otimizado | ~25mA | 100 horas |

### Estratégias de Otimização

#### Nível 1: Modem Sleep entre Broadcasts (Fácil)
```cpp
// Após enviar/receber, desligar modem por 25s
WiFi.setSleep(WIFI_PS_MAX_MODEM);

// Problema: Perde mensagens durante sleep
// Solução: Janelas de escuta sincronizadas
```
**Redução:** 95mA → ~30mA

#### Nível 2: Light Sleep com Wake Periódico (Médio)
```cpp
// Dormir 29s, acordar 1s para escutar/enviar
esp_sleep_enable_timer_wakeup(29000000);  // 29s
esp_light_sleep_start();

// Ao acordar:
initMeshSync();
delay(1000);  // Janela de escuta
sendDiscovery();
```
**Redução:** 95mA → ~5mA médio

#### Nível 3: Deep Sleep com ESP-NOW Wake (Avançado)
```cpp
// O ESP32 pode acordar com ESP-NOW em deep sleep
// mas requer configuração específica do RTC
esp_now_set_wake_window(50);  // 50ms janela
esp_wifi_set_inactive_time(ESP_IF_WIFI_STA, 1);
```
**Redução:** 95mA → ~0.5mA médio

### Trade-offs

| Otimização | Consumo | Latência sync | Complexidade |
|------------|---------|---------------|--------------|
| Atual | 95mA | <50ms | Baixa |
| Modem Sleep | 30mA | <1s | Baixa |
| Light Sleep | 5mA | <30s | Média |
| Deep Sleep | 0.5mA | <60s | Alta |

### Recomendação por Caso de Uso

| Caso | Alimentação | Recomendação |
|------|-------------|--------------|
| Ligado à corrente | 5V USB | Manter atual (simplicidade) |
| Powerbank ocasional | USB | Modem Sleep |
| Bateria permanente | 18650 | Light Sleep mínimo |
| Solar/bateria longa | Painel | Deep Sleep obrigatório |

---

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
5. **Consumo elevado** - Não otimizado para bateria (ver análise acima)

## Resumo de Impacto

| Vetor | Estado Atual | Severidade | Ação Recomendada |
|-------|--------------|------------|------------------|
| Spam de rede | ~40 KB/hora | ✅ Baixa | Nenhuma (aceitável) |
| Colisões | Backoff implementado | ✅ Baixa | Monitorar em produção |
| Consumo energia | ~95mA contínuo | ⚠️ Alta | Implementar sleep se bateria |
| Latência sync | <50ms | ✅ Excelente | Manter |

## Extensões Futuras

- [ ] Encriptação de mensagens (ESP-NOW suporta CCMP)
- [ ] Modo "follow the sun" - Um dispositivo partilha elevação solar calculada
- [ ] Sync de brightness individual
- [ ] OTA update coordenado entre dispositivos
- [ ] Heartbeat visual (LED pisca quando recebe de peer)
- [ ] **Light Sleep entre broadcasts** (prioridade se bateria)
- [ ] **Adaptive broadcast interval** (menos tráfego quando estável)
- [ ] **Eliminar ACKs redundantes** (heartbeat implícito)
