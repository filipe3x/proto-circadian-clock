// =====================================================================
// Proto Circadian Clock - Remote Control (ESP32-C3)
// =====================================================================
// Sketch independente que se faz passar por mais um Proto Circadian
// Clock na rede mesh ESP-NOW.
//
//   * Short press: cicla modo (AUTO_SOLAR -> THERAPY_RED -> OFF -> ...)
//   * Long press em AUTO: avanca o relogio (devagar -> rapido, acelera
//     enquanto pressionado). Ao soltar, o offset final fica aplicado.
//     OLED mostra HH:MM simulado em tempo real.
//   * Long press em THERAPY_RED: cicla brightness para cima; ao chegar
//     ao topo, faz wrap para o minimo. Ao soltar, fica no valor actual.
//
// O protocolo (magic / versao / layout SyncSettings) tem de bater certo
// com clock.ino. Se mudares um, muda o outro.
// =====================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <U8g2lib.h>

// ===================== HARDWARE PINS =====================
#define BUTTON0_PIN 9
#define LED_IOB_PIN 8     // LED on-board, activo LOW
#define OLED_SDA    5
#define OLED_SCL    6

// ===================== PROTOCOLO MESH =====================
// Tem de bater certo com clock.ino
#define MESH_CHANNEL  1
#define SYNC_MAGIC    0x50434C4B   // "PCLK"
#define SYNC_VERSION  1

#define DISCOVERY_INTERVAL_MS 30000

#define SYNC_FLAG_BRIGHTNESS    0x01
#define SYNC_FLAG_TIME_OFFSET   0x02
#define SYNC_FLAG_EPOCH         0x04

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
  uint8_t  flags;
  uint8_t  brightness;
  int16_t  timeOffsetMin;
  uint32_t epoch;
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

// ===================== LONG PRESS / UI =====================
#define LONG_PRESS_MS         500
#define DEBOUNCE_MS           30
#define BCAST_MIN_GAP_MS      100

// Brightness (THERAPY_RED): step + min/max + tick rate.
// Em RED queremos uma alteracao bem lenta para o utilizador ter controlo
// preciso (o painel filtra com um ramp suave por cima disto).
#define BRIGHT_STEP           6
#define BRIGHT_MIN            20
#define BRIGHT_MAX            240
#define BRIGHT_TICK_MS        260

// ===================== ESTADO =====================
static uint8_t  broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static uint8_t  deviceId[6];
// firstBootTime artificialmente ALTO: o C3 nunca deve ser eleito master.
static uint32_t myFirstBootTime = 0xFFFFFF00UL;
static uint16_t nextMsgId       = 0;

static Mode     currentMode     = AUTO_SOLAR;
static uint32_t lastDiscoveryMs = 0;
static uint32_t lastTxMs        = 0;
static uint32_t txCount         = 0;
static uint32_t rxCount         = 0;
static bool     lastSendOk      = true;

// Relogio cacheado do painel (para mostrar HH:MM no OLED).
static uint32_t cachedEpoch     = 0;
static uint32_t cachedEpochRxMs = 0;

// Preview controlado pelo long press
static int16_t  previewOffsetMin = 0;     // delta aplicado em AUTO
static uint8_t  previewBrightness = 128;  // valor cycled em RED

// LED feedback nao bloqueante
static bool     ledBlinking    = false;
static uint32_t ledBlinkUntil  = 0;

// Maquina de estados do botao
static bool     btnDown          = false;
static uint32_t btnDownAt        = 0;
static bool     longPressActive  = false;
static uint32_t nextLongTickAt   = 0;
static uint32_t lastBcastAt      = 0;
static bool     longPressDirty   = false;  // valor mudou desde ultimo broadcast?

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

// Calcula HH:MM "actual segundo o painel" (epoch cacheado + tempo decorrido
// desde a recepcao). Devolve true se temos relogio cacheado.
static bool computePanelHHMM(int16_t addMinutes, int& outH, int& outM) {
  if (cachedEpoch == 0) return false;
  uint32_t elapsed = (millis() - cachedEpochRxMs) / 1000UL;
  int32_t epochAdj = (int32_t)cachedEpoch + (int32_t)elapsed + (int32_t)addMinutes * 60;
  // Normalizar para "hora do dia" — assume UTC; o painel aplica o seu TZ.
  uint32_t secOfDay = ((uint32_t)epochAdj % 86400UL + 86400UL) % 86400UL;
  outH = (int)(secOfDay / 3600U);
  outM = (int)((secOfDay % 3600U) / 60U);
  return true;
}

// ===================== OLED =====================
static void drawIdleScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 7, "PCLK Remote");

  u8g2.setFont(u8g2_font_7x13B_tf);
  char line[16];
  snprintf(line, sizeof(line), "Mode:%s", modeName(currentMode));
  u8g2.drawStr(0, 22, line);

  u8g2.setFont(u8g2_font_5x8_tf);
  snprintf(line, sizeof(line), "T%lu R%lu",
           (unsigned long)txCount, (unsigned long)rxCount);
  u8g2.drawStr(0, 32, line);
  u8g2.drawStr(0, 40, lastSendOk ? "TX:OK" : "TX:FAIL");

  u8g2.sendBuffer();
}

static void drawAutoLongPressScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 7, "AUTO advance");

  // Relogio simulado em destaque
  u8g2.setFont(u8g2_font_logisoso16_tn);
  int h, m;
  char clk[8];
  if (computePanelHHMM(previewOffsetMin, h, m)) {
    snprintf(clk, sizeof(clk), "%02d:%02d", h, m);
  } else {
    snprintf(clk, sizeof(clk), "--:--");
  }
  u8g2.drawStr(0, 27, clk);

  // Offset aplicado
  u8g2.setFont(u8g2_font_5x8_tf);
  char off[16];
  int absMin = previewOffsetMin < 0 ? -previewOffsetMin : previewOffsetMin;
  snprintf(off, sizeof(off), "%c%02d:%02d",
           previewOffsetMin < 0 ? '-' : '+',
           absMin / 60, absMin % 60);
  u8g2.drawStr(0, 40, off);

  u8g2.sendBuffer();
}

static void drawRedLongPressScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(0, 7, "RED bright");

  u8g2.setFont(u8g2_font_logisoso16_tn);
  char val[8];
  snprintf(val, sizeof(val), "%3d", (int)previewBrightness);
  u8g2.drawStr(0, 27, val);

  u8g2.setFont(u8g2_font_5x8_tf);
  char pct[16];
  snprintf(pct, sizeof(pct), "%d%%", (previewBrightness * 100) / 255);
  u8g2.drawStr(0, 40, pct);

  u8g2.sendBuffer();
}

static void drawScreen() {
  if (longPressActive) {
    if (currentMode == AUTO_SOLAR)       drawAutoLongPressScreen();
    else if (currentMode == THERAPY_RED) drawRedLongPressScreen();
    else                                  drawIdleScreen();
  } else {
    drawIdleScreen();
  }
}

// ===================== ESP-NOW CALLBACKS =====================
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

  if (msg.msgType == MSG_SETTINGS) {
    if (msg.payload.settings.mode < 3) {
      Mode incoming = (Mode)msg.payload.settings.mode;
      if (incoming != currentMode) {
        currentMode = incoming;
      }
    }
    // Durante long press nao deixamos o master sobrescrever o nosso preview
    // local (poderia ser uma versao mais antiga, antes do nosso ultimo step).
    bool autoLockOut = (longPressActive && currentMode == AUTO_SOLAR);
    bool redLockOut  = (longPressActive && currentMode == THERAPY_RED);
    if ((msg.payload.settings.flags & SYNC_FLAG_TIME_OFFSET) && !autoLockOut) {
      previewOffsetMin = msg.payload.settings.timeOffsetMin;
    }
    if ((msg.payload.settings.flags & SYNC_FLAG_BRIGHTNESS) && !redLockOut) {
      previewBrightness = msg.payload.settings.brightness;
    }
    if (msg.payload.settings.flags & SYNC_FLAG_EPOCH) {
      cachedEpoch     = msg.payload.settings.epoch;
      cachedEpochRxMs = millis();
    }
    if (!longPressActive) drawScreen();
  }
}

// ===================== ENVIO =====================
static void sendModeChange(Mode newMode) {
  MeshMessage msg;
  fillMessage(msg, MSG_SETTINGS);
  msg.payload.settings.mode = (uint8_t)newMode;
  esp_err_t r = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  if (r == ESP_OK) { txCount++; lastTxMs = millis(); } else { lastSendOk = false; }
}

static void sendTimeOffset(int16_t offsetMin) {
  MeshMessage msg;
  fillMessage(msg, MSG_SETTINGS);
  msg.payload.settings.mode          = (uint8_t)currentMode;
  msg.payload.settings.flags         = SYNC_FLAG_TIME_OFFSET;
  msg.payload.settings.timeOffsetMin = offsetMin;
  esp_err_t r = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  if (r == ESP_OK) { txCount++; lastTxMs = millis(); } else { lastSendOk = false; }
}

static void sendBrightness(uint8_t brightness) {
  MeshMessage msg;
  fillMessage(msg, MSG_SETTINGS);
  msg.payload.settings.mode       = (uint8_t)currentMode;
  msg.payload.settings.flags      = SYNC_FLAG_BRIGHTNESS;
  msg.payload.settings.brightness = brightness;
  esp_err_t r = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  if (r == ESP_OK) { txCount++; lastTxMs = millis(); } else { lastSendOk = false; }
}

static void sendDiscovery() {
  MeshMessage msg;
  fillMessage(msg, MSG_DISCOVERY);
  esp_err_t r = esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));
  if (r == ESP_OK) { txCount++; lastTxMs = millis(); } else { lastSendOk = false; }
}

// ===================== ACELERACAO =====================
// Curva de aceleracao para AUTO long press: quanto mais tempo pressionado,
// maior o step e mais curto o tick.
static void autoStepSchedule(uint32_t heldMs, int& stepMin, uint32_t& tickMs) {
  uint32_t in = (heldMs > LONG_PRESS_MS) ? heldMs - LONG_PRESS_MS : 0;
  if      (in < 1000)  { stepMin = 1;  tickMs = 200; }   //   5 min/s
  else if (in < 2000)  { stepMin = 5;  tickMs = 150; }   //  33 min/s
  else if (in < 3500)  { stepMin = 15; tickMs = 100; }   // 150 min/s
  else                 { stepMin = 60; tickMs = 80;  }   // 750 min/s (~12h/min)
}

// ===================== LONG PRESS HANDLERS =====================
static void onLongPressStart() {
  longPressActive = true;
  nextLongTickAt  = millis();
  longPressDirty  = true;
  // Inicializar valor previewBrightness no valor actual conhecido
  // (mantido via recepcao de SETTINGS); se nunca vimos, fica no default 128.
}

static void onLongPressTick() {
  uint32_t now = millis();
  if ((int32_t)(now - nextLongTickAt) < 0) return;

  if (currentMode == AUTO_SOLAR) {
    uint32_t held = now - btnDownAt;
    int stepMin;
    uint32_t tickMs;
    autoStepSchedule(held, stepMin, tickMs);

    int32_t v = (int32_t)previewOffsetMin + stepMin;
    if (v >  12 * 60) v -= 24 * 60;       // wrap em ciclo de 24h
    if (v < -12 * 60) v += 24 * 60;
    previewOffsetMin = (int16_t)v;
    nextLongTickAt   = now + tickMs;
    longPressDirty   = true;
  }
  else if (currentMode == THERAPY_RED) {
    int v = (int)previewBrightness + BRIGHT_STEP;
    if (v > BRIGHT_MAX) v = BRIGHT_MIN;   // wrap para minimo
    previewBrightness = (uint8_t)v;
    nextLongTickAt   = now + BRIGHT_TICK_MS;
    longPressDirty   = true;
  }
  else {
    // OFF: nada a fazer
    nextLongTickAt = now + 200;
  }
}

static void onLongPressRelease() {
  // Aplicacao final (garante que o ultimo valor chega ao painel mesmo
  // que tenha sido bloqueado pelo throttle de broadcast).
  if (currentMode == AUTO_SOLAR) {
    sendTimeOffset(previewOffsetMin);
    Serial.printf("[REMOTE] AUTO offset final: %+d min\n", previewOffsetMin);
  } else if (currentMode == THERAPY_RED) {
    sendBrightness(previewBrightness);
    Serial.printf("[REMOTE] RED bright final: %u\n", previewBrightness);
  }
  longPressActive = false;
  longPressDirty  = false;
}

static void maybeBroadcastDuringLongPress() {
  if (!longPressDirty) return;
  uint32_t now = millis();
  if (now - lastBcastAt < BCAST_MIN_GAP_MS) return;
  lastBcastAt = now;

  if (currentMode == AUTO_SOLAR)       sendTimeOffset(previewOffsetMin);
  else if (currentMode == THERAPY_RED) sendBrightness(previewBrightness);
  longPressDirty = false;
}

// ===================== INIT =====================
static bool setupEspNow() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(MESH_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

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
  digitalWrite(LED_IOB_PIN, HIGH);

  pinMode(BUTTON0_PIN, INPUT_PULLUP);

  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();

  bool ok = setupEspNow();
  Serial.printf("[REMOTE] ESP-NOW %s, MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                ok ? "OK" : "FAIL",
                deviceId[0], deviceId[1], deviceId[2],
                deviceId[3], deviceId[4], deviceId[5]);

  if (ok) sendDiscovery();
  lastDiscoveryMs = millis();
  drawScreen();
}

// ===================== LOOP =====================
void loop() {
  uint32_t now = millis();

  // Fim do blink do LED
  if (ledBlinking && (int32_t)(now - ledBlinkUntil) >= 0) {
    ledBlinking = false;
    digitalWrite(LED_IOB_PIN, HIGH);
  }

  // Polled debounce + state machine do botao
  static uint32_t lastEdgeAt = 0;
  bool raw = (digitalRead(BUTTON0_PIN) == LOW);
  if (raw != btnDown && (now - lastEdgeAt) > DEBOUNCE_MS) {
    lastEdgeAt = now;
    if (raw) {
      // Press
      btnDown   = true;
      btnDownAt = now;
    } else {
      // Release
      btnDown = false;
      if (longPressActive) {
        onLongPressRelease();
        drawScreen();
      } else {
        // Short press => cicla modo
        currentMode = nextMode(currentMode);
        sendModeChange(currentMode);
        // Pisca LED 200ms para feedback
        ledBlinking   = true;
        ledBlinkUntil = now + 200;
        digitalWrite(LED_IOB_PIN, LOW);
        Serial.printf("[REMOTE] short -> %s\n", modeName(currentMode));
        drawScreen();
      }
    }
  }

  // Long press: deteccao + ticks
  if (btnDown && !longPressActive && (now - btnDownAt) >= LONG_PRESS_MS) {
    onLongPressStart();
    Serial.printf("[REMOTE] long start (%s)\n", modeName(currentMode));
    drawScreen();
  }
  if (btnDown && longPressActive) {
    onLongPressTick();
    maybeBroadcastDuringLongPress();
    // Redesenhar OLED ~30Hz para o relogio nao parecer congelado.
    static uint32_t lastDrawMs = 0;
    if (now - lastDrawMs >= 33) {
      lastDrawMs = now;
      drawScreen();
    }
  }

  // Discovery periodico
  if (now - lastDiscoveryMs >= DISCOVERY_INTERVAL_MS) {
    lastDiscoveryMs = now;
    sendDiscovery();
  }
}
