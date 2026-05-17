// =====================================================================
// Proto Circadian Clock - Remote Control (ESP32-C3)
// =====================================================================
// Sketch independente para um ESP32-C3 SuperMini que se faz passar por
// mais um Proto Circadian Clock na rede mesh ESP-NOW. Ao pressionar o
// botao 0, faz broadcast de uma mensagem MSG_SETTINGS com o proximo
// modo (AUTO_SOLAR -> THERAPY_RED -> OFF -> ...) e todos os paineis
// reais aplicam-no instantaneamente.
//
// Hardware alvo: ESP32-C3 SuperMini + OLED 0.42" 72x40 (SSD1306 I2C)
//
// IMPORTANTE: O protocolo (magic, versao, layout das structs) tem de
// ficar SEMPRE em sintonia com clock.ino. Se mudares um, muda o outro.
// =====================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <U8g2lib.h>

// ===================== HARDWARE PINS =====================
#define BUTTON0_PIN 9
#define LED_IOB_PIN 8     // LED on-board, ativo LOW
#define OLED_SDA    5
#define OLED_SCL    6

// ===================== PROTOCOLO MESH =====================
// Tem de bater certo com clock.ino linhas 62-130
#define MESH_CHANNEL  1
#define SYNC_MAGIC    0x50434C4B   // "PCLK"
#define SYNC_VERSION  1

#define DISCOVERY_INTERVAL_MS 30000   // mesmo BROADCAST_INTERVAL_MS do clock.ino

enum MeshMsgType : uint8_t {
  MSG_DISCOVERY = 0x01,
  MSG_SETTINGS  = 0x02,
  MSG_REQUEST   = 0x03,
  MSG_ACK       = 0x04
};

enum Mode : uint8_t {
  AUTO_SOLAR  = 0,
  THERAPY_RED = 1,
  MODE_OFF    = 2
};

struct __attribute__((packed)) SyncSettings {
  float    latitude;
  float    longitude;
  int8_t   timezoneOffset;
  int8_t   solarOffsetHours;
  uint8_t  mode;
  uint8_t  reserved[5];
};

struct __attribute__((packed)) MeshMessage {
  uint32_t magic;
  uint8_t  version;
  uint8_t  msgType;
  uint8_t  senderId[6];
  uint32_t firstBootTime;
  uint16_t msgId;
  union {
    SyncSettings settings;
    uint16_t     ackMsgId;
  } payload;
};

// ===================== ESTADO =====================
static uint8_t  broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static uint8_t  deviceId[6];
// firstBootTime artificialmente ALTO: o C3 nunca deve ser eleito master.
// Os paineis usam "mais antigo = master" (firstBootTime mais baixo).
static uint32_t myFirstBootTime = 0xFFFFFF00UL;
static uint16_t nextMsgId       = 0;

static Mode     currentMode     = AUTO_SOLAR;
static uint32_t lastDiscoveryMs = 0;
static uint32_t lastRxMs        = 0;
static uint32_t lastTxMs        = 0;
static uint32_t txCount         = 0;
static uint32_t rxCount         = 0;
static bool     lastSendOk      = true;

// LED feedback sem bloquear
static bool     ledBlinking    = false;
static uint32_t ledBlinkUntil  = 0;

// Botao
static volatile bool btnPressedEdge = false;

// OLED 0.42" (72x40), rodado 180
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

// ===================== HELPERS =====================
static const char* modeName(Mode m) {
  switch (m) {
    case AUTO_SOLAR:  return "AUTO";
    case THERAPY_RED: return "RED";
    case MODE_OFF:    return "OFF";
  }
  return "?";
}

static Mode nextMode(Mode m) {
  return (Mode)((uint8_t)(m + 1) % 3);
}

static void fillMessage(MeshMessage& msg, MeshMsgType type) {
  msg.magic         = SYNC_MAGIC;
  msg.version       = SYNC_VERSION;
  msg.msgType       = (uint8_t)type;
  memcpy(msg.senderId, deviceId, 6);
  msg.firstBootTime = myFirstBootTime;
  msg.msgId         = nextMsgId++;
  memset(&msg.payload, 0, sizeof(msg.payload));
}

static void drawScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);

  // Linha 1: titulo
  u8g2.drawStr(0, 7, "PCLK Remote");

  // Linha 2: modo actual em destaque
  u8g2.setFont(u8g2_font_7x13B_tf);
  char line[16];
  snprintf(line, sizeof(line), "Mode:%s", modeName(currentMode));
  u8g2.drawStr(0, 22, line);
  u8g2.setFont(u8g2_font_5x8_tf);

  // Linha 3: contadores TX/RX
  snprintf(line, sizeof(line), "T%lu R%lu",
           (unsigned long)txCount, (unsigned long)rxCount);
  u8g2.drawStr(0, 32, line);

  // Linha 4: estado ultimo envio
  u8g2.drawStr(0, 40, lastSendOk ? "TX:OK" : "TX:FAIL");

  u8g2.sendBuffer();
}

// ===================== ESP-NOW CALLBACKS =====================
// Assinatura antiga (arduino-esp32 2.x). Se usares 3.x, muda para a
// versao com esp_now_recv_info_t* / esp_now_send_info_t*.
static void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  lastSendOk = (status == ESP_NOW_SEND_SUCCESS);
}

static void onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
  (void)mac;
  if (len < (int)sizeof(MeshMessage)) return;
  MeshMessage msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.magic != SYNC_MAGIC || msg.version != SYNC_VERSION) return;

  rxCount++;
  lastRxMs = millis();

  // Acompanhar o modo actual quando outro dispositivo o muda, para que o
  // proximo clique parta do estado real.
  if (msg.msgType == MSG_SETTINGS && msg.payload.settings.mode < 3) {
    Mode incoming = (Mode)msg.payload.settings.mode;
    if (incoming != currentMode) {
      currentMode = incoming;
      drawScreen();
    }
  }
}

// ===================== ENVIO =====================
static void sendModeChange(Mode newMode) {
  MeshMessage msg;
  fillMessage(msg, MSG_SETTINGS);
  msg.payload.settings.latitude         = 0.0f;
  msg.payload.settings.longitude        = 0.0f;
  msg.payload.settings.timezoneOffset   = 0;
  msg.payload.settings.solarOffsetHours = 0;
  msg.payload.settings.mode             = (uint8_t)newMode;

  esp_err_t r = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  if (r == ESP_OK) {
    txCount++;
    lastTxMs = millis();
  } else {
    lastSendOk = false;
  }
}

static void sendDiscovery() {
  MeshMessage msg;
  fillMessage(msg, MSG_DISCOVERY);
  esp_err_t r = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  if (r == ESP_OK) {
    txCount++;
    lastTxMs = millis();
  } else {
    lastSendOk = false;
  }
}

// ===================== ISR =====================
static void IRAM_ATTR isrButton0() {
  btnPressedEdge = true;
}

// ===================== INIT =====================
static bool setupEspNow() {
  WiFi.mode(WIFI_STA);
  // Fixar canal igual ao mesh dos paineis (canal 1).
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(MESH_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Usar MAC STA como deviceId (mesmo esquema dos clocks).
  esp_read_mac(deviceId, ESP_MAC_WIFI_STA);

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, broadcastAddress, 6);
  peer.channel = MESH_CHANNEL;
  peer.encrypt = false;
  esp_now_del_peer(broadcastAddress);
  if (esp_now_add_peer(&peer) != ESP_OK) return false;

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(50);

  pinMode(LED_IOB_PIN, OUTPUT);
  digitalWrite(LED_IOB_PIN, HIGH);   // LED activo LOW => HIGH = apagado

  pinMode(BUTTON0_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON0_PIN), isrButton0, FALLING);

  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();

  bool ok = setupEspNow();

  Serial.printf("[REMOTE] ESP-NOW %s, MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                ok ? "OK" : "FAIL",
                deviceId[0], deviceId[1], deviceId[2],
                deviceId[3], deviceId[4], deviceId[5]);

  // Discovery inicial para os paineis nos verem como peer.
  if (ok) sendDiscovery();
  lastDiscoveryMs = millis();
  drawScreen();
}

// ===================== LOOP =====================
void loop() {
  uint32_t now = millis();

  // Fim do blink
  if (ledBlinking && (int32_t)(now - ledBlinkUntil) >= 0) {
    ledBlinking = false;
    digitalWrite(LED_IOB_PIN, HIGH);
  }

  // Botao: avancar modo + broadcast
  if (btnPressedEdge) {
    btnPressedEdge = false;
    static uint32_t lastPress = 0;
    if (now - lastPress > 200) {       // debounce 200ms
      lastPress = now;

      currentMode = nextMode(currentMode);
      sendModeChange(currentMode);

      // Pisca LED 1s para feedback visual
      ledBlinking   = true;
      ledBlinkUntil = now + 1000;
      digitalWrite(LED_IOB_PIN, LOW);

      Serial.printf("[REMOTE] -> %s\n", modeName(currentMode));
      drawScreen();
    }
  }

  // Discovery periodico (heartbeat)
  if (now - lastDiscoveryMs >= DISCOVERY_INTERVAL_MS) {
    lastDiscoveryMs = now;
    sendDiscovery();
  }
}
