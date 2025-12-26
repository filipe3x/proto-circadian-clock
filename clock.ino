#include <P10_32x16_QuarterScan.h> //https://github.com/filipe3x/P10_32x16_QuarterScan
#include <RTClib.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <Dusk2Dawn.h>
#include "captive_portal.h"  // Modulo do captive portal
#include "auto_solar.h"       // Modulo de calculo solar

// ============= PINOUT =============
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 19
#define C_PIN 5
#define D_PIN 17
#define E_PIN -1
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

// ============= DISPLAY =============
MatrixPanel_I2S_DMA *dma_display = nullptr;
P10_32x16_QuarterScan *display = nullptr;

// ============= CONFIGURACAO RTC =============
RTC_DS3231 rtc;
bool rtcAvailable = false;

// ============= CONFIGURACAO WIFI MULTI-SSID =============
const int WIFI_TIMEOUT_MS = 10000;  // 10s por rede
const int WIFI_RETRY_PER_NETWORK = 2;  // Tentar cada rede 2x

// ============= STATUS DO SISTEMA =============
enum SystemStatus {
  STATUS_RTC_WIFI_NTP,      // 🟢 VERDE - Perfeito
  STATUS_WIFI_NTP,          // 🔵 AZUL - Bom
  STATUS_RTC_WIFI,          // 🟡 AMARELO - OK
  STATUS_RTC_ONLY,          // 🟣 ROXO - Funcional
  STATUS_WIFI_ONLY,         // 🟠 LARANJA - Limitado
  STATUS_OFFLINE            // 🔴 VERMELHO - Crítico
};

SystemStatus currentStatus = STATUS_OFFLINE;

// ============= LOCALIZACAO =============
// Configuracoes dinamicas em captive_portal.h: configLatitude, configLongitude, configTimezone
// Usar referencias para as variaveis do modulo captive_portal
#define LATITUDE configLatitude
#define LONGITUDE configLongitude
#define TIMEZONE_OFFSET configTimezone
const int SOLAR_OFFSET_HOURS = 1;

// ============= DEVICE ID UNICO =============
// O ESP32 tem um eFuse de 64 bits unico de fabrica
// Usamos 6 bytes (48 bits) como ID principal
// Combinamos com um hash do MAC BT como fallback para garantir unicidade
uint8_t deviceId[6];
char deviceIdStr[18];  // "XX:XX:XX:XX:XX:XX\0"
Preferences preferences;

// ============= MESH SYNC (ESP-NOW) =============
// Estrutura para sincronizacao entre dispositivos Proto Circadian Clock
#define MESH_CHANNEL 1
#define SYNC_MAGIC 0x50434C4B  // "PCLK" - Proto CLock
#define SYNC_VERSION 1
#define BROADCAST_INTERVAL_MS 30000  // Broadcast a cada 30s (quando ativo)
#define ACK_TIMEOUT_MS 500
#define MAX_PEERS 10

// Configuracao do modo hibrido (economia de energia)
#define MESH_SCAN_INTERVAL_MS 60000   // Scan a cada 60s quando sozinho
#define MESH_SCAN_DURATION_MS 3000    // 3s de escuta durante scan normal
#define MESH_INITIAL_SCAN_MS 65000    // 65s no primeiro scan (> 1 ciclo completo)
#define MESH_IDLE_TIMEOUT_MS 300000   // 5 min sem peers = desligar mesh

// Estados do mesh (maquina de estados hibrida)
enum MeshState {
  MESH_OFF,       // WiFi desligado, a poupar energia
  MESH_SCANNING,  // A procurar peers (WiFi ON temporario)
  MESH_ACTIVE     // Peers encontrados, mesh sempre ativo
};

MeshState meshState = MESH_OFF;
unsigned long lastScanTime = 0;
unsigned long lastPeerActivity = 0;
bool isFirstScan = true;  // Primeiro scan e mais longo

// Endereco broadcast ESP-NOW
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Tipos de mensagem
enum MeshMsgType {
  MSG_DISCOVERY = 0x01,    // Anuncio de presenca
  MSG_SETTINGS = 0x02,     // Broadcast de definicoes (do master)
  MSG_REQUEST = 0x03,      // Pedido de definicoes (de um slave)
  MSG_ACK = 0x04           // Confirmacao de recepcao
};

// Estrutura de definicoes sincronizaveis
struct __attribute__((packed)) SyncSettings {
  float latitude;
  float longitude;
  int8_t timezoneOffset;
  int8_t solarOffsetHours;
  uint8_t mode;            // AUTO_SOLAR, THERAPY_RED, OFF
  uint8_t reserved[5];     // Para expansao futura
};

// Estrutura de mensagem ESP-NOW
struct __attribute__((packed)) MeshMessage {
  uint32_t magic;          // Identificador PCLK
  uint8_t version;         // Versao do protocolo
  uint8_t msgType;         // Tipo de mensagem
  uint8_t senderId[6];     // ID do remetente
  uint32_t firstBootTime;  // Timestamp da primeira instalacao (segundos desde 2020)
  uint16_t msgId;          // ID da mensagem para ACKs
  union {
    SyncSettings settings; // Para MSG_SETTINGS
    uint16_t ackMsgId;     // Para MSG_ACK - ID da mensagem confirmada
  } payload;
};

// Estado do mesh
bool meshEnabled = false;
bool isMaster = false;
uint32_t myFirstBootTime = 0;
uint16_t nextMsgId = 0;
unsigned long lastBroadcast = 0;

// Tabela de peers conhecidos
struct PeerInfo {
  uint8_t id[6];
  uint32_t firstBootTime;
  unsigned long lastSeen;
  bool active;
};
PeerInfo knownPeers[MAX_PEERS];
int numPeers = 0;

// Fila de ACKs pendentes
struct PendingAck {
  uint16_t msgId;
  uint8_t targetId[6];
  unsigned long sentTime;
  bool waiting;
};
PendingAck pendingAcks[5];

// Callback de recepcao ESP-NOW (declaracao antecipada)
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

// ============= BOTAO =============
#define BUTTON_PIN 0
volatile bool buttonPressed = false;

void IRAM_ATTR buttonISR() {
  buttonPressed = true;
}

// ============= MODOS =============
enum Mode {
  AUTO_SOLAR,
  THERAPY_RED,
  OFF
};

Mode currentMode = AUTO_SOLAR;

// SolarColor agora definida em auto_solar.h

// ============= DECLARACOES ANTECIPADAS =============
void fillPanel(SolarColor color);
DateTime getCurrentTime();
void showBootAnimation();
void updateBootStatus(uint16_t color, int progress);
void showTimeOnBoot(DateTime now, uint16_t color);
SystemStatus connectWiFiAndSync();
bool tryConnectWiFi();
bool syncWithNTP();
void printSolarInfo();
// calculateSolarElevation, colorTempToRGB e mapSolarElevationToColor declaradas em auto_solar.h
void displayAutoSolar();
void displayTherapyRed();
void displayOff();
void handleButton();
void updateDisplay();
uint16_t getStatusColor(SystemStatus status);
const char* getStatusName(SystemStatus status);

// Button feedback icons
void showModeFeedback(Mode mode);
void drawSunIcon(uint8_t brightness);
void drawRedText(uint8_t brightness);
void drawOffText(uint8_t brightness);
void fadeTransition(void (*drawFunc)(uint8_t), int holdMs, bool fadeToFill, uint8_t fillR, uint8_t fillG, uint8_t fillB);

// Device ID e Mesh Sync
void initDeviceId();
void initMeshSync();
void updateMeshSync();
void sendDiscovery();
void sendSettings();
void broadcastModeChange();
void sendAck(const uint8_t* targetMac, uint16_t ackMsgId);
void handleDiscovery(const uint8_t* mac, MeshMessage* msg);
void handleSettings(MeshMessage* msg);
void handleAck(MeshMessage* msg);
void updateMasterStatus();
bool compareMac(const uint8_t* a, const uint8_t* b);
void formatMac(const uint8_t* mac, char* str);
void startMeshScan();
void stopMesh();
const char* getMeshStateName(MeshState state);

// Captive Portal - declaracoes em captive_portal.h

// ============= ANIMACAO DE BOOT =============
void showBootAnimation() {
  display->fillScreen(display->color565(0, 0, 0));
  delay(200);
  display->fillRect(0, 14, 32, 2, display->color565(50, 0, 0));
  delay(300);
}

void updateBootStatus(uint16_t color, int progress) {
  display->fillScreen(display->color565(0, 0, 0));
  int barWidth = map(progress, 0, 100, 0, 32);
  display->fillRect(0, 14, barWidth, 2, color);
  delay(200);
}

void showTimeOnBoot(DateTime now, uint16_t color) {
  display->fillScreen(display->color565(0, 0, 0));
  
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", now.hour(), now.minute());
  
  display->setTextSize(1);
  display->setTextColor(color);
  display->setCursor(1, 4);
  display->print(timeStr);
  
  Serial.printf("Mostrando hora no boot: %s\n", timeStr);
  delay(2000);
}

// ============= CORES DE STATUS =============
uint16_t getStatusColor(SystemStatus status) {
  switch(status) {
    case STATUS_RTC_WIFI_NTP: return display->color565(0, 80, 0);   // 🟢 Verde
    case STATUS_WIFI_NTP:     return display->color565(0, 0, 80);   // 🔵 Azul
    case STATUS_RTC_WIFI:     return display->color565(80, 80, 0);  // 🟡 Amarelo
    case STATUS_RTC_ONLY:     return display->color565(80, 0, 80);  // 🟣 Roxo
    case STATUS_WIFI_ONLY:    return display->color565(80, 40, 0);  // 🟠 Laranja
    case STATUS_OFFLINE:      return display->color565(80, 0, 0);   // 🔴 Vermelho
    default:                  return display->color565(80, 0, 0);
  }
}

const char* getStatusName(SystemStatus status) {
  switch(status) {
    case STATUS_RTC_WIFI_NTP: return "RTC+WiFi+NTP (Perfeito)";
    case STATUS_WIFI_NTP:     return "WiFi+NTP (Bom)";
    case STATUS_RTC_WIFI:     return "RTC+WiFi (OK, NTP falhou)";
    case STATUS_RTC_ONLY:     return "Só RTC (Funcional)";
    case STATUS_WIFI_ONLY:    return "WiFi (Limitado, sem hora)";
    case STATUS_OFFLINE:      return "Offline (Crítico)";
    default:                  return "Desconhecido";
  }
}

// ============= BUTTON FEEDBACK ICONS =============

// Draw a beautiful amber sun with radiating rays
void drawSunIcon(uint8_t brightness) {
  display->fillScreen(display->color565(0, 0, 0));

  // Scale colors by brightness (amber/golden sun color)
  uint8_t r = (255 * brightness) / 255;
  uint8_t g = (180 * brightness) / 255;  // Amber gold
  uint8_t b = (20 * brightness) / 255;   // Slight warmth

  uint16_t sunColor = display->color565(r, g, b);
  uint16_t rayColor = display->color565(r * 3/4, g * 3/4, b * 3/4);

  // Center of display
  int cx = 15;
  int cy = 7;

  // Draw 8 rays first (behind the sun)
  // Diagonal rays (longer)
  display->drawLine(cx - 6, cy - 5, cx - 4, cy - 3, rayColor);  // Top-left
  display->drawLine(cx + 6, cy - 5, cx + 4, cy - 3, rayColor);  // Top-right
  display->drawLine(cx - 6, cy + 5, cx - 4, cy + 3, rayColor);  // Bottom-left
  display->drawLine(cx + 6, cy + 5, cx + 4, cy + 3, rayColor);  // Bottom-right

  // Cardinal rays (shorter, thicker feel)
  display->drawLine(cx, cy - 6, cx, cy - 4, rayColor);          // Top
  display->drawLine(cx, cy + 6, cx, cy + 4, rayColor);          // Bottom
  display->drawLine(cx - 7, cy, cx - 5, cy, rayColor);          // Left
  display->drawLine(cx + 7, cy, cx + 5, cy, rayColor);          // Right

  // Draw sun circle (filled) - 3 pixel radius approximation
  display->fillCircle(cx, cy, 3, sunColor);

  // Add a brighter center for depth
  uint8_t rBright = min(255, (int)(r * 1.2));
  uint8_t gBright = min(255, (int)(g * 1.1));
  display->drawPixel(cx, cy, display->color565(rBright, gBright, b));
  display->drawPixel(cx - 1, cy, display->color565(rBright, gBright, b));
  display->drawPixel(cx, cy - 1, display->color565(rBright, gBright, b));
}

// Draw "RED" text in bold red
void drawRedText(uint8_t brightness) {
  display->fillScreen(display->color565(0, 0, 0));

  uint8_t r = (255 * brightness) / 255;
  uint16_t textColor = display->color565(r, 0, 0);

  display->setTextSize(1);
  display->setTextColor(textColor);

  // Center "RED" on 32x16 display
  // Each char is 6px wide at size 1, "RED" = 18px, so start at (7, 4)
  display->setCursor(7, 4);
  display->print("RED");
}

// Draw "OFF" text in red
void drawOffText(uint8_t brightness) {
  display->fillScreen(display->color565(0, 0, 0));

  uint8_t r = (180 * brightness) / 255;  // Slightly dimmer red for OFF
  uint16_t textColor = display->color565(r, 0, 0);

  display->setTextSize(1);
  display->setTextColor(textColor);

  // Center "OFF" on 32x16 display
  display->setCursor(7, 4);
  display->print("OFF");
}

// Smooth fade transition with optional fill at end
void fadeTransition(void (*drawFunc)(uint8_t), int holdMs, bool fadeToFill, uint8_t fillR, uint8_t fillG, uint8_t fillB) {
  // Fade in (0 -> 255)
  for (int b = 0; b <= 255; b += 15) {
    drawFunc(b);
    delay(20);
  }
  drawFunc(255);  // Ensure full brightness

  // Hold at full brightness
  delay(holdMs);

  if (fadeToFill) {
    // Crossfade to solid fill color
    for (int t = 0; t <= 100; t += 5) {
      // Draw the icon at decreasing brightness
      uint8_t iconBright = 255 - (t * 255 / 100);
      drawFunc(iconBright);

      // Overlay with increasing fill opacity (blend effect)
      uint8_t fillBright = (t * 255 / 100);
      uint8_t r = (fillR * fillBright) / 255;
      uint8_t g = (fillG * fillBright) / 255;
      uint8_t bVal = (fillB * fillBright) / 255;

      // Draw fill pixels over the icon area
      if (t > 50) {
        // Past halfway, start showing solid fill
        uint16_t fillColor = display->color565(r, g, bVal);
        for (int y = 0; y < 16; y++) {
          for (int x = 0; x < 32; x++) {
            display->drawPixel(x, y, fillColor);
          }
        }
      }
      delay(25);
    }

    // Final solid fill
    display->fillScreen(display->color565(fillR, fillG, fillB));
  } else {
    // Simple fade out
    for (int b = 255; b >= 0; b -= 15) {
      drawFunc(b);
      delay(20);
    }
    display->fillScreen(display->color565(0, 0, 0));
  }
}

// Main feedback function called on mode change
void showModeFeedback(Mode mode) {
  switch (mode) {
    case AUTO_SOLAR:
      // Beautiful amber sun with rays, fade in, hold, fade out
      fadeTransition(drawSunIcon, 400, false, 0, 0, 0);
      break;

    case THERAPY_RED:
      // "RED" text fades in, then crossfades to red fill
      fadeTransition(drawRedText, 300, true, 100, 0, 0);
      break;

    case OFF:
      // "OFF" text fades in, hold, then fade to black
      fadeTransition(drawOffText, 400, false, 0, 0, 0);
      break;
  }
}

// ============= CONEXAO WIFI MULTI-SSID =============
bool tryConnectWiFi() {
  Serial.println("\n=== Tentando conectar WiFi ===");
  
  // Tentar cada rede
  for(int net = 0; net < NUM_WIFI_NETWORKS; net++) {
    Serial.printf("\nRede %d/%d: %s\n", net + 1, NUM_WIFI_NETWORKS, wifiNetworks[net].ssid.c_str());
    
    // Tentar cada rede múltiplas vezes
    for(int attempt = 0; attempt < WIFI_RETRY_PER_NETWORK; attempt++) {
      if(attempt > 0) {
        Serial.printf("  Tentativa %d/%d...\n", attempt + 1, WIFI_RETRY_PER_NETWORK);
      }
      
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifiNetworks[net].ssid.c_str(), wifiNetworks[net].password.c_str());
      
      unsigned long startAttempt = millis();
      bool toggle = false;
      
      while (WiFi.status() != WL_CONNECTED && 
             millis() - startAttempt < WIFI_TIMEOUT_MS) {
        
        display->fillScreen(display->color565(0, 0, 0));
        
        if (toggle) {
          display->fillRect(0, 14, 32, 2, display->color565(30, 10, 0));
        } else {
          display->fillRect(0, 14, 32, 2, display->color565(10, 3, 0));
        }
        
        toggle = !toggle;
        delay(300);
        Serial.print(".");
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi conectado!");
        Serial.printf("   SSID: %s\n", wifiNetworks[net].ssid.c_str());
        Serial.printf("   IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("   RSSI: %d dBm\n", WiFi.RSSI());
        
        display->fillScreen(display->color565(0, 0, 0));
        display->fillRect(0, 14, 32, 2, display->color565(0, 80, 0));
        delay(500);
        
        return true;
      }
      
      Serial.println(" Falhou");
      WiFi.disconnect();
      delay(1000);
    }
  }
  
  Serial.println("\n❌ Nenhuma rede WiFi disponível");
  display->fillScreen(display->color565(0, 0, 0));
  display->fillRect(0, 14, 32, 2, display->color565(80, 0, 0));
  delay(500);
  
  return false;
}

// ============= SINCRONIZACAO NTP =============
bool syncWithNTP() {
  Serial.println("\n=== Sincronizando NTP ===");
  
  configTime(TIMEZONE_OFFSET * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  
  struct tm timeinfo;
  int attempts = 0;
  
  while (!getLocalTime(&timeinfo) && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (getLocalTime(&timeinfo)) {
    Serial.println("✅ NTP sincronizado!");
    Serial.printf("   %02d/%02d/%04d %02d:%02d:%02d\n",
                  timeinfo.tm_mday, timeinfo.tm_mon + 1, 
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_hour, timeinfo.tm_min, 
                  timeinfo.tm_sec);
    
    // Atualizar RTC se disponível
    if (rtcAvailable) {
      rtc.adjust(DateTime(timeinfo.tm_year + 1900, 
                         timeinfo.tm_mon + 1, 
                         timeinfo.tm_mday, 
                         timeinfo.tm_hour, 
                         timeinfo.tm_min, 
                         timeinfo.tm_sec));
      Serial.println("   RTC atualizado com NTP");
    }
    
    return true;
  } else {
    Serial.println("❌ NTP falhou");
    return false;
  }
}

// ============= CONEXAO E SINCRONIZACAO COMPLETA =============
SystemStatus connectWiFiAndSync() {
  bool wifiOK = false;
  bool ntpOK = false;
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  INICIALIZANDO CONECTIVIDADE     ║");
  Serial.println("╚═══════════════════════════════════╝");
  
  // PASSO 1: Tentar conectar WiFi
  wifiOK = tryConnectWiFi();
  
  // PASSO 2: Se WiFi OK, tentar NTP
  if (wifiOK) {
    ntpOK = syncWithNTP();
    
    // Desconectar WiFi para poupar energia
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi desligado (economia de energia)");
  }
  
  // PASSO 3: Determinar status final
  SystemStatus status;
  
  if (rtcAvailable && wifiOK && ntpOK) {
    status = STATUS_RTC_WIFI_NTP;  // 🟢 Perfeito
  } else if (wifiOK && ntpOK) {
    status = STATUS_WIFI_NTP;       // 🔵 Bom
  } else if (rtcAvailable && wifiOK) {
    status = STATUS_RTC_WIFI;       // 🟡 OK (NTP falhou)
  } else if (rtcAvailable) {
    status = STATUS_RTC_ONLY;       // 🟣 Funcional (sem WiFi)
  } else if (wifiOK) {
    status = STATUS_WIFI_ONLY;      // 🟠 Limitado (sem NTP nem RTC)
  } else {
    status = STATUS_OFFLINE;        // 🔴 Crítico
  }
  
  // Mostrar resumo
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  STATUS FINAL                     ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("RTC:  %s\n", rtcAvailable ? "✅ Disponível" : "❌ Não disponível");
  Serial.printf("WiFi: %s\n", wifiOK ? "✅ Conectado" : "❌ Falhou");
  Serial.printf("NTP:  %s\n", ntpOK ? "✅ Sincronizado" : "❌ Falhou");
  Serial.printf("\n🎯 STATUS: %s\n\n", getStatusName(status));
  
  return status;
}

// ============= DEVICE ID E MESH SYNC =============

// Inicializa o ID unico do dispositivo
void initDeviceId() {
  // Obter o chip ID do ESP32 (baseado no MAC address do eFuse)
  // Este e um identificador de 48 bits unico de fabrica
  uint64_t chipId = ESP.getEfuseMac();

  // Extrair os 6 bytes do chip ID
  deviceId[0] = (chipId >> 0) & 0xFF;
  deviceId[1] = (chipId >> 8) & 0xFF;
  deviceId[2] = (chipId >> 16) & 0xFF;
  deviceId[3] = (chipId >> 24) & 0xFF;
  deviceId[4] = (chipId >> 32) & 0xFF;
  deviceId[5] = (chipId >> 40) & 0xFF;

  // Formatar como string
  formatMac(deviceId, deviceIdStr);

  Serial.println("\n=== DEVICE ID ===");
  Serial.printf("Chip ID: %s\n", deviceIdStr);
  Serial.printf("Raw: 0x%012llX\n", chipId);

  // Carregar/guardar timestamp de primeira instalacao
  preferences.begin("pclk", false);  // namespace "pclk" = Proto CLock

  myFirstBootTime = preferences.getUInt("firstBoot", 0);

  if (myFirstBootTime == 0) {
    // Primeira instalacao! Guardar timestamp atual
    // Usamos segundos desde 2020-01-01 para caber em 32 bits
    // Epoch de 2020: 1577836800
    myFirstBootTime = (millis() / 1000) + 1;  // +1 para nunca ser 0

    // Se tivermos hora real (RTC ou NTP), usar essa
    DateTime now = getCurrentTime();
    if (now.year() >= 2020) {
      myFirstBootTime = now.unixtime() - 1577836800;  // Segundos desde 2020
    }

    preferences.putUInt("firstBoot", myFirstBootTime);
    Serial.printf("Primeira instalacao! Timestamp: %u\n", myFirstBootTime);
  } else {
    Serial.printf("Dispositivo instalado ha: %u segundos\n", myFirstBootTime);
  }

  preferences.end();
}

// Formata MAC address como string
void formatMac(const uint8_t* mac, char* str) {
  sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Compara dois MAC addresses
bool compareMac(const uint8_t* a, const uint8_t* b) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// Callback quando dados sao enviados
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    char macStr[18];
    formatMac(mac_addr, macStr);
    Serial.printf("[MESH] Falha envio para %s\n", macStr);
  }
}

// Callback quando dados sao recebidos
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len < sizeof(MeshMessage)) return;

  MeshMessage* msg = (MeshMessage*)data;

  // Verificar magic number e versao
  if (msg->magic != SYNC_MAGIC || msg->version != SYNC_VERSION) {
    return;  // Nao e uma mensagem nossa
  }

  // Ignorar mensagens de nos mesmos
  if (compareMac(msg->senderId, deviceId)) {
    return;
  }

  char senderStr[18];
  formatMac(msg->senderId, senderStr);

  switch (msg->msgType) {
    case MSG_DISCOVERY:
      handleDiscovery(info->src_addr, msg);
      break;
    case MSG_SETTINGS:
      handleSettings(msg);
      // Enviar ACK
      sendAck(info->src_addr, msg->msgId);
      break;
    case MSG_REQUEST:
      // Se somos master, enviar settings
      if (isMaster) {
        sendSettings();
      }
      break;
    case MSG_ACK:
      handleAck(msg);
      break;
  }
}

// Processa mensagem de discovery
void handleDiscovery(const uint8_t* mac, MeshMessage* msg) {
  char senderStr[18];
  formatMac(msg->senderId, senderStr);

  // Procurar se ja conhecemos este peer
  int freeSlot = -1;
  for (int i = 0; i < MAX_PEERS; i++) {
    if (knownPeers[i].active && compareMac(knownPeers[i].id, msg->senderId)) {
      // Atualizar lastSeen
      knownPeers[i].lastSeen = millis();
      knownPeers[i].firstBootTime = msg->firstBootTime;
      Serial.printf("[MESH] Peer atualizado: %s (age: %u)\n", senderStr, msg->firstBootTime);
      updateMasterStatus();
      return;
    }
    if (!knownPeers[i].active && freeSlot == -1) {
      freeSlot = i;
    }
  }

  // Novo peer!
  if (freeSlot != -1) {
    memcpy(knownPeers[freeSlot].id, msg->senderId, 6);
    knownPeers[freeSlot].firstBootTime = msg->firstBootTime;
    knownPeers[freeSlot].lastSeen = millis();
    knownPeers[freeSlot].active = true;
    numPeers++;

    Serial.printf("[MESH] Novo peer: %s (age: %u)\n", senderStr, msg->firstBootTime);

    // Adicionar como peer ESP-NOW
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = MESH_CHANNEL;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      Serial.printf("[MESH] Peer ESP-NOW adicionado\n");
    }

    updateMasterStatus();

    // Se somos novos (não master), pedir settings
    if (!isMaster) {
      // Enviar pedido de settings
      MeshMessage request;
      request.magic = SYNC_MAGIC;
      request.version = SYNC_VERSION;
      request.msgType = MSG_REQUEST;
      memcpy(request.senderId, deviceId, 6);
      request.firstBootTime = myFirstBootTime;
      request.msgId = nextMsgId++;

      esp_now_send(mac, (uint8_t*)&request, sizeof(request));
      Serial.printf("[MESH] Pedido de sync enviado\n");
    }
  }
}

// Processa settings recebidas de outro dispositivo
void handleSettings(MeshMessage* msg) {
  SyncSettings* settings = &msg->payload.settings;

  // MODO: Aceitar de qualquer dispositivo para sync imediato
  // Isto permite que qualquer pessoa mude o modo e todos sincronizem
  if (settings->mode != currentMode && settings->mode < 3) {
    const char* modeNames[] = {"AUTO_SOLAR", "THERAPY_RED", "OFF"};
    Serial.printf("[MESH] Modo sync: %s -> %s\n",
                  modeNames[currentMode], modeNames[settings->mode]);

    currentMode = (Mode)settings->mode;

    // Atualizar display imediatamente (sem animacao para ser instantaneo)
    updateDisplay();
  }

  // OUTRAS SETTINGS: So aceitar do master (dispositivo mais antigo)
  if (msg->firstBootTime < myFirstBootTime) {
    // Aqui poderiamos aplicar latitude/longitude/timezone se fossem dinamicos
    // Por agora sao constantes, entao apenas logamos
    Serial.printf("[MESH] Settings do master recebidas (lat: %.2f, lon: %.2f)\n",
                  settings->latitude, settings->longitude);
  }
}

// Processa ACK recebido
void handleAck(MeshMessage* msg) {
  uint16_t ackedId = msg->payload.ackMsgId;

  for (int i = 0; i < 5; i++) {
    if (pendingAcks[i].waiting && pendingAcks[i].msgId == ackedId) {
      pendingAcks[i].waiting = false;
      Serial.printf("[MESH] ACK recebido para msg %d\n", ackedId);
      return;
    }
  }
}

// Envia ACK com backoff aleatorio para evitar colisoes
void sendAck(const uint8_t* targetMac, uint16_t ackMsgId) {
  // Backoff aleatorio: 10-150ms baseado no device ID para distribuir no tempo
  uint8_t slot = deviceId[5] % 10;  // 0-9
  int backoff = 10 + (slot * 15) + random(0, 20);  // 10-170ms
  delay(backoff);

  MeshMessage ack;
  ack.magic = SYNC_MAGIC;
  ack.version = SYNC_VERSION;
  ack.msgType = MSG_ACK;
  memcpy(ack.senderId, deviceId, 6);
  ack.firstBootTime = myFirstBootTime;
  ack.msgId = nextMsgId++;
  ack.payload.ackMsgId = ackMsgId;

  esp_now_send(targetMac, (uint8_t*)&ack, sizeof(ack));
}

// Atualiza status de master/slave
void updateMasterStatus() {
  bool wasMaster = isMaster;
  isMaster = true;  // Assumir que somos master

  // Verificar se algum peer e mais antigo
  for (int i = 0; i < MAX_PEERS; i++) {
    if (knownPeers[i].active) {
      // Remover peers inativos (>5 min sem comunicacao)
      if (millis() - knownPeers[i].lastSeen > 300000) {
        knownPeers[i].active = false;
        numPeers--;
        char peerStr[18];
        formatMac(knownPeers[i].id, peerStr);
        Serial.printf("[MESH] Peer removido (timeout): %s\n", peerStr);
        continue;
      }

      if (knownPeers[i].firstBootTime < myFirstBootTime) {
        isMaster = false;  // Ha um dispositivo mais antigo
      }
    }
  }

  if (wasMaster != isMaster) {
    Serial.printf("[MESH] Estado alterado: %s\n", isMaster ? "MASTER" : "SLAVE");
  }
}

// Envia mensagem de discovery (broadcast)
void sendDiscovery() {
  MeshMessage discovery;
  discovery.magic = SYNC_MAGIC;
  discovery.version = SYNC_VERSION;
  discovery.msgType = MSG_DISCOVERY;
  memcpy(discovery.senderId, deviceId, 6);
  discovery.firstBootTime = myFirstBootTime;
  discovery.msgId = nextMsgId++;

  esp_now_send(broadcastAddress, (uint8_t*)&discovery, sizeof(discovery));
  Serial.printf("[MESH] Discovery broadcast (age: %u, peers: %d)\n", myFirstBootTime, numPeers);
}

// Envia settings (como master)
void sendSettings() {
  if (!isMaster) return;

  MeshMessage msg;
  msg.magic = SYNC_MAGIC;
  msg.version = SYNC_VERSION;
  msg.msgType = MSG_SETTINGS;
  memcpy(msg.senderId, deviceId, 6);
  msg.firstBootTime = myFirstBootTime;
  msg.msgId = nextMsgId++;

  // Preencher settings atuais
  msg.payload.settings.latitude = LATITUDE;
  msg.payload.settings.longitude = LONGITUDE;
  msg.payload.settings.timezoneOffset = TIMEZONE_OFFSET;
  msg.payload.settings.solarOffsetHours = SOLAR_OFFSET_HOURS;
  msg.payload.settings.mode = currentMode;
  memset(msg.payload.settings.reserved, 0, 5);

  // Enviar broadcast
  esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  Serial.printf("[MESH] Settings broadcast enviado\n");
}

// Broadcast imediato de mudanca de modo (para feedback instantaneo)
// Qualquer dispositivo pode enviar, nao precisa ser master
void broadcastModeChange() {
  // So enviar se mesh esta ativo e ha peers
  if (meshState != MESH_ACTIVE || numPeers == 0) return;

  MeshMessage msg;
  msg.magic = SYNC_MAGIC;
  msg.version = SYNC_VERSION;
  msg.msgType = MSG_SETTINGS;  // Reutilizar tipo SETTINGS
  memcpy(msg.senderId, deviceId, 6);
  msg.firstBootTime = myFirstBootTime;
  msg.msgId = nextMsgId++;

  msg.payload.settings.latitude = LATITUDE;
  msg.payload.settings.longitude = LONGITUDE;
  msg.payload.settings.timezoneOffset = TIMEZONE_OFFSET;
  msg.payload.settings.solarOffsetHours = SOLAR_OFFSET_HOURS;
  msg.payload.settings.mode = currentMode;
  memset(msg.payload.settings.reserved, 0, 5);

  esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  Serial.printf("[MESH] Mode change broadcast: %d\n", currentMode);
}

// Retorna nome do estado do mesh
const char* getMeshStateName(MeshState state) {
  switch (state) {
    case MESH_OFF: return "OFF";
    case MESH_SCANNING: return "SCANNING";
    case MESH_ACTIVE: return "ACTIVE";
    default: return "UNKNOWN";
  }
}

// Inicia scan para descoberta de peers (liga WiFi temporariamente)
void startMeshScan() {
  Serial.println("[MESH] Iniciando scan...");

  // Configurar WiFi em modo STA para ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Definir canal WiFi fixo para mesh
  esp_wifi_set_channel(MESH_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[MESH] Erro ao inicializar ESP-NOW");
    meshState = MESH_OFF;
    return;
  }

  // Registar callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // Adicionar peer broadcast
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = MESH_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[MESH] Erro ao adicionar peer broadcast");
    esp_now_deinit();
    WiFi.mode(WIFI_OFF);
    meshState = MESH_OFF;
    return;
  }

  meshEnabled = true;
  meshState = MESH_SCANNING;
  lastScanTime = millis();

  // Enviar discovery imediatamente
  sendDiscovery();

  int scanDuration = isFirstScan ? MESH_INITIAL_SCAN_MS : MESH_SCAN_DURATION_MS;
  Serial.printf("[MESH] Scan iniciado (WiFi ON por %ds) %s\n",
                scanDuration / 1000,
                isFirstScan ? "[INICIAL]" : "");
}

// Para o mesh e desliga WiFi para poupar energia
void stopMesh() {
  Serial.println("[MESH] Desligando mesh (economia energia)...");

  // Desinicializar ESP-NOW
  esp_now_deinit();

  // Desligar WiFi completamente
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  meshEnabled = false;
  meshState = MESH_OFF;

  // Limpar tabela de peers
  for (int i = 0; i < MAX_PEERS; i++) {
    if (knownPeers[i].active) {
      knownPeers[i].active = false;
    }
  }
  numPeers = 0;

  Serial.printf("[MESH] WiFi OFF - Proximo scan em %ds (%ds duracao)\n",
                MESH_SCAN_INTERVAL_MS / 1000,
                MESH_SCAN_DURATION_MS / 1000);
}

// Inicializa o sistema mesh (chamado no setup)
void initMeshSync() {
  Serial.println("\n=== MESH SYNC (HIBRIDO) ===");
  Serial.printf("[MESH] Device ID: %s\n", deviceIdStr);
  Serial.printf("[MESH] First Boot: %u\n", myFirstBootTime);
  Serial.printf("[MESH] Modo: Hibrido (scan %ds a cada %ds)\n",
                MESH_SCAN_DURATION_MS / 1000, MESH_SCAN_INTERVAL_MS / 1000);

  // Inicializar tabela de peers
  for (int i = 0; i < MAX_PEERS; i++) {
    knownPeers[i].active = false;
  }
  for (int i = 0; i < 5; i++) {
    pendingAcks[i].waiting = false;
  }

  isMaster = true;  // Assumir master ate descobrir peers mais antigos

  // Iniciar primeiro scan imediatamente
  startMeshScan();
}

// Atualiza mesh sync no loop (maquina de estados hibrida)
void updateMeshSync() {
  unsigned long now = millis();

  switch (meshState) {
    case MESH_OFF:
      // Modo economia: WiFi desligado, verificar se e hora de fazer scan
      if (now - lastScanTime >= MESH_SCAN_INTERVAL_MS) {
        startMeshScan();
      }
      break;

    case MESH_SCANNING:
      // A procurar peers: esperar pela duracao do scan
      {
        unsigned long scanDuration = isFirstScan ? MESH_INITIAL_SCAN_MS : MESH_SCAN_DURATION_MS;

        // Durante scan inicial, enviar discovery periodicamente (a cada 10s)
        if (isFirstScan && (now - lastBroadcast >= 10000)) {
          lastBroadcast = now;
          sendDiscovery();
        }

        if (now - lastScanTime >= scanDuration) {
          if (numPeers > 0) {
            // Encontrou peers! Transitar para modo ativo
            meshState = MESH_ACTIVE;
            lastPeerActivity = now;
            lastBroadcast = now;
            isFirstScan = false;
            Serial.printf("[MESH] %d peer(s) encontrado(s) - Mesh ATIVO\n", numPeers);
          } else {
            // Nenhum peer encontrado, desligar WiFi
            isFirstScan = false;  // Proximo scan sera curto
            stopMesh();
          }
        }
      }
      break;

    case MESH_ACTIVE:
      // Mesh ativo: comportamento normal com verificacao de timeout

      // Atualizar lastPeerActivity se ha peers ativos
      if (numPeers > 0) {
        lastPeerActivity = now;
      }

      // Broadcast periodico
      if (now - lastBroadcast >= BROADCAST_INTERVAL_MS) {
        lastBroadcast = now;

        sendDiscovery();

        // Se somos master, enviar settings tambem
        if (isMaster && numPeers > 0) {
          delay(100);  // Pequeno delay para nao colidir
          sendSettings();
        }

        updateMasterStatus();
      }

      // Verificar ACKs pendentes (timeout)
      for (int i = 0; i < 5; i++) {
        if (pendingAcks[i].waiting && (now - pendingAcks[i].sentTime > ACK_TIMEOUT_MS)) {
          pendingAcks[i].waiting = false;
          char targetStr[18];
          formatMac(pendingAcks[i].targetId, targetStr);
          Serial.printf("[MESH] ACK timeout para %s (msg %d)\n", targetStr, pendingAcks[i].msgId);
        }
      }

      // Verificar se perdemos todos os peers (timeout de inatividade)
      if (numPeers == 0 && (now - lastPeerActivity >= MESH_IDLE_TIMEOUT_MS)) {
        Serial.println("[MESH] Sem peers ha 5 minutos");
        stopMesh();
      }
      break;
  }
}

// Funcao auxiliar para mostrar feedback visual do captive portal no display
void showCaptivePortalDisplay() {
  display->fillScreen(display->color565(0, 0, 0));

  // Desenhar icone de WiFi estilizado
  uint16_t lavender = display->color565(167, 139, 250);
  uint16_t amber = display->color565(251, 191, 36);

  // Arcos de WiFi
  display->drawPixel(15, 2, lavender);
  display->drawPixel(16, 2, lavender);
  display->drawLine(13, 4, 18, 4, lavender);
  display->drawLine(11, 6, 20, 6, lavender);

  // Texto "SETUP"
  display->setTextSize(1);
  display->setTextColor(amber);
  display->setCursor(4, 9);
  display->print("SETUP");
}

// Wrapper para iniciar o captive portal com feedback visual
void startCaptivePortalWithDisplay() {
  // Parar mesh se estiver ativo
  if (meshState != MESH_OFF) {
    stopMesh();
  }

  // Iniciar o portal (modulo externo)
  startCaptivePortal(nullptr);

  // Mostrar feedback no display
  showCaptivePortalDisplay();
}

// colorTempToRGB movida para auto_solar.cpp

// ============= PREENCHER PAINEL =============
void fillPanel(SolarColor color) {
  uint8_t r = ((uint16_t)color.r * color.brightness) / 255;
  uint8_t g = ((uint16_t)color.g * color.brightness) / 255;
  uint8_t b = ((uint16_t)color.b * color.brightness) / 255;
  
  uint16_t color565 = display->color565(r, g, b);
  display->fillScreen(color565);
}

// ============= OBTER HORA ATUAL =============
DateTime getCurrentTime() {
  if (rtcAvailable) {
    return rtc.now();
  } else {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      return DateTime(timeinfo.tm_year + 1900, 
                     timeinfo.tm_mon + 1, 
                     timeinfo.tm_mday, 
                     timeinfo.tm_hour, 
                     timeinfo.tm_min, 
                     timeinfo.tm_sec);
    } else {
      // Fallback: hora de compilação
      Serial.println("⚠️  Usando hora de compilação");
      return DateTime(F(__DATE__), F(__TIME__));
    }
  }
}

// ============= SETUP =============
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════╗");
  Serial.println("║     RELÓGIO SOLAR LED v3.0        ║");
  Serial.println("║   Multi-WiFi + Mesh Sync          ║");
  Serial.println("╚═══════════════════════════════════╝");
  
  // Configurar pinos
  HUB75_I2S_CFG::i2s_pins _pins = {
    R1_PIN, G1_PIN, B1_PIN,
    R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };
  
  // Criar display base
  HUB75_I2S_CFG mxconfig(64, 8, 1, _pins);
  mxconfig.clkphase = false;
  mxconfig.driver = HUB75_I2S_CFG::SHIFTREG;  // ← Faltava isto!
  
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->setBrightness8(80);
  
  if (!dma_display->begin()) {
    Serial.println("❌ ERRO: Display não inicializou!");
    while(1) delay(1000);
  }
  
  dma_display->clearScreen();
  // O wrapper P10_32x16_QuarterScan expõe interface lógica 32x16
  display = new P10_32x16_QuarterScan(dma_display);
  
  Serial.println("✅ Display P10 inicializado");
  
  showBootAnimation();
  updateBootStatus(display->color565(50, 0, 0), 10);
  
  // Botão
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  currentMode = AUTO_SOLAR;
  
  Serial.println("✅ Botão configurado");
  updateBootStatus(display->color565(50, 0, 0), 20);
  
  // RTC
  Wire.begin(21, 22);
  if (rtc.begin()) {
    rtcAvailable = true;
    Serial.println("✅ RTC DS3231 encontrado");
    
    if (rtc.lostPower()) {
      Serial.println("⚠️  RTC perdeu energia, será sincronizado");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    DateTime now = rtc.now();
    Serial.printf("   RTC: %02d/%02d/%04d %02d:%02d:%02d\n",
                  now.day(), now.month(), now.year(),
                  now.hour(), now.minute(), now.second());
    
    float temp = rtc.getTemperature();
    Serial.printf("   Temperatura: %.1f°C\n", temp);
  } else {
    rtcAvailable = false;
    Serial.println("⚠️  RTC não encontrado");
  }
  
  updateBootStatus(display->color565(50, 0, 0), 40);

  // Verificar se ha configuracao guardada
  if (!hasStoredConfig()) {
    Serial.println("\n⚠️  Sem configuracao guardada!");
    Serial.println("    Iniciando Captive Portal...\n");
    startCaptivePortalWithDisplay();
    return;  // Sair do setup - o loop vai gerir o portal
  }

  // Carregar configuracao
  loadConfig();
  initSolarCalc();

  // Aplicar brilho configurado
  dma_display->setBrightness8(configBrightness);

  updateBootStatus(display->color565(50, 0, 0), 45);

  // Inicializar Device ID unico
  initDeviceId();

  updateBootStatus(display->color565(50, 0, 0), 50);

  // WiFi + NTP (com fallback automático)
  currentStatus = connectWiFiAndSync();

  updateBootStatus(display->color565(50, 0, 0), 70);

  // Inicializar Mesh Sync (ESP-NOW)
  initMeshSync();
  
  updateBootStatus(display->color565(50, 0, 0), 80);
  
  printSolarInfo();
  
  // Mostrar status com cor
  uint16_t statusColor = getStatusColor(currentStatus);
  
  display->fillScreen(display->color565(0, 0, 0));
  display->fillRect(0, 14, 32, 2, statusColor);
  delay(1000);
  
  // Mostrar hora atual
  DateTime bootTime = getCurrentTime();
  showTimeOnBoot(bootTime, statusColor);
  
  // Flash de status
  display->fillScreen(statusColor);
  delay(1000);
  
  // Fade out
  for (int i = 100; i >= 0; i -= 10) {
    uint8_t r = ((statusColor >> 11) << 3) * i / 100;
    uint8_t g = (((statusColor >> 5) & 0x3F) << 2) * i / 100;
    uint8_t b = ((statusColor & 0x1F) << 3) * i / 100;
    
    display->fillScreen(display->color565(r, g, b));
    delay(50);
  }
  
  display->clearScreen();
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║     SISTEMA PRONTO!               ║");
  Serial.println("╚═══════════════════════════════════╝");
  Serial.printf("Device ID: %s\n", deviceIdStr);
  Serial.printf("Mesh: %s | Peers: %d\n", getMeshStateName(meshState), numPeers);
  Serial.printf("Modo inicial: AUTO_SOLAR\n");
  Serial.printf("Offset solar: %+d hora(s)\n\n", SOLAR_OFFSET_HOURS);

  updateDisplay();
}

// ============= INFORMACOES SOLARES =============
void printSolarInfo() {
  DateTime now = getCurrentTime();
  int sunrise = solarCalc->sunrise(now.year(), now.month(), now.day(), true);
  int sunset = solarCalc->sunset(now.year(), now.month(), now.day(), true);
  
  Serial.println("\n--- Info Solar ---");
  Serial.printf("Data: %02d/%02d/%04d\n", now.day(), now.month(), now.year());
  Serial.printf("Nascer: %02d:%02d\n", sunrise / 60, sunrise % 60);
  Serial.printf("Pôr: %02d:%02d\n", sunset / 60, sunset % 60);
  Serial.println("------------------\n");
}

// calculateSolarElevation e mapSolarElevationToColor movidas para auto_solar.cpp

// ============= MODO AUTO SOLAR =============
void displayAutoSolar() {
  DateTime now = getCurrentTime();
  
  DateTime solarTime = now;
  if (SOLAR_OFFSET_HOURS != 0) {
    int offsetMinutes = SOLAR_OFFSET_HOURS * 60;
    solarTime = now + TimeSpan(0, 0, offsetMinutes, 0);
  }
  
  float elevation = calculateSolarElevation(solarTime, LATITUDE, LONGITUDE, TIMEZONE_OFFSET);
  SolarColor color = mapSolarElevationToColor(elevation);
  
  fillPanel(color);
  
  static uint8_t lastMinute = 255;
  if (now.minute() % 5 == 0 && now.minute() != lastMinute) {
    lastMinute = now.minute();
    Serial.printf("[AUTO] %02d:%02d (offset %+dh) | Elev: %.2f° | %dK | RGB(%d,%d,%d) | Brilho: %d%%\n",
                  now.hour(), now.minute(),
                  SOLAR_OFFSET_HOURS,
                  elevation, color.colorTemp, color.r, color.g, color.b,
                  (color.brightness * 100) / 255);
  }
}

// ============= OUTROS MODOS =============
void displayTherapyRed() {
  SolarColor color;
  color.r = 255;
  color.g = 0;
  color.b = 0;
  color.brightness = 100;
  color.colorTemp = 0;
  fillPanel(color);
}

void displayOff() {
  SolarColor color = {0, 0, 0, 0, 0};
  fillPanel(color);
}

// ============= GESTAO DE BOTAO =============
void handleButton() {
  static unsigned long buttonPressStart = 0;
  static bool longPressTriggered = false;
  static bool showingProgress = false;

  if (buttonPressed) {
    buttonPressed = false;
    delay(20);

    if (digitalRead(BUTTON_PIN) == LOW) {
      // Botao acabou de ser pressionado
      buttonPressStart = millis();
      longPressTriggered = false;
      showingProgress = false;
    }
  }

  // Se o botao esta pressionado, verificar duracao
  if (digitalRead(BUTTON_PIN) == LOW && buttonPressStart > 0) {
    unsigned long pressDuration = millis() - buttonPressStart;

    // Mostrar progresso visual apos 1 segundo
    if (pressDuration > 1000 && !showingProgress) {
      showingProgress = true;
      display->fillScreen(display->color565(0, 0, 0));
      display->setTextSize(1);
      display->setTextColor(display->color565(167, 139, 250));  // Lavender
      display->setCursor(1, 4);
      display->print("SETUP");
    }

    // Atualizar barra de progresso (1s a 5s = 4s total)
    if (showingProgress && !longPressTriggered) {
      int progress = constrain(map(pressDuration, 1000, BUTTON_LONG_PRESS_MS, 0, 32), 0, 32);
      display->fillRect(0, 14, progress, 2, display->color565(251, 191, 36));  // Amber
    }

    // Long press detectado (5 segundos)
    if (pressDuration >= BUTTON_LONG_PRESS_MS && !longPressTriggered) {
      longPressTriggered = true;
      Serial.println("\n[BUTTON] Long press detectado - entrando em modo config...");

      // Feedback visual
      display->fillScreen(display->color565(0, 0, 0));
      display->fillRect(0, 14, 32, 2, display->color565(110, 231, 183));  // Mint green

      delay(500);

      // Iniciar captive portal
      startCaptivePortalWithDisplay();

      buttonPressStart = 0;
      return;
    }
  }
  // Botao foi libertado
  else if (buttonPressStart > 0) {
    unsigned long pressDuration = millis() - buttonPressStart;
    buttonPressStart = 0;

    // Short press (< 1s) - mudar modo
    if (pressDuration < 1000 && !longPressTriggered) {
      currentMode = (Mode)((currentMode + 1) % 3);

      const char* modeNames[] = {"AUTO_SOLAR", "THERAPY_RED", "OFF"};
      Serial.printf("\n>>> MODO: %s <<<\n\n", modeNames[currentMode]);

      // SYNC IMEDIATO: Broadcast para todos os peers ANTES da animacao
      broadcastModeChange();

      // Show beautiful feedback icon with fade animation
      showModeFeedback(currentMode);

      // Then update to the actual mode display
      updateDisplay();
    }
    // Medium press (1-5s) - cancelado, restaurar display
    else if (!longPressTriggered) {
      updateDisplay();
    }

    showingProgress = false;
  }
}

// ============= ATUALIZAR DISPLAY =============
void updateDisplay() {
  switch (currentMode) {
    case AUTO_SOLAR: displayAutoSolar(); break;
    case THERAPY_RED: displayTherapyRed(); break;
    case OFF: displayOff(); break;
  }
}

// ============= LOOP =============
void loop() {
  static unsigned long lastUpdate = 0;
  static unsigned long lastSync = 0;

  // Se estamos em modo configuracao, processar apenas o captive portal
  if (configMode) {
    if (!processCaptivePortal()) {
      // Timeout - reiniciar
      Serial.println("\n[CONFIG] Timeout - reiniciando...");
      ESP.restart();
    }
    delay(10);
    return;
  }

  handleButton();

  // Atualizar mesh sync
  updateMeshSync();

  unsigned long currentMillis = millis();
  
  // Atualizar display a cada 60s (só em AUTO_SOLAR)
  if (currentMillis - lastUpdate >= 60000) {
    lastUpdate = currentMillis;
    
    if (currentMode == AUTO_SOLAR) {
      updateDisplay();
    }
  }
  
  // Re-sincronizar periodicamente
  // RTC+WiFi+NTP: 24h
  // WiFi+NTP: 12h
  // RTC only: 7 dias
  // Outros: 1h
  unsigned long syncInterval;
  
  switch(currentStatus) {
    case STATUS_RTC_WIFI_NTP: syncInterval = 86400000; break;  // 24h
    case STATUS_WIFI_NTP:     syncInterval = 43200000; break;  // 12h
    case STATUS_RTC_WIFI:     syncInterval = 21600000; break;  // 6h
    case STATUS_RTC_ONLY:     syncInterval = 604800000; break; // 7 dias
    case STATUS_WIFI_ONLY:    syncInterval = 3600000; break;   // 1h
    case STATUS_OFFLINE:      syncInterval = 3600000; break;   // 1h
  }
  
  if (currentMillis - lastSync >= syncInterval) {
    lastSync = currentMillis;
    Serial.println("\n[SYNC] Re-sincronização periódica...");
    currentStatus = connectWiFiAndSync();
  }
  
  delay(20);
}