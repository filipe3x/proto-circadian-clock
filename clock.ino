#include <P10_32x16_QuarterScan.h> //https://github.com/filipe3x/P10_32x16_QuarterScan
#include <RTClib.h>
#include <WiFi.h>
#include <Dusk2Dawn.h>

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
struct WiFiCredentials {
  const char* ssid;
  const char* password;
};

// Lista de redes WiFi (ordena por prioridade)
WiFiCredentials wifiNetworks[] = {
  {"guest", "benfica00"},
  {"iPhone SE3", "b3jjgjg0c1m4j"},
  // Adiciona mais redes aqui
};

const int NUM_WIFI_NETWORKS = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);
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
const float LATITUDE = 41.5362;
const float LONGITUDE = -8.7813;
const int TIMEZONE_OFFSET = 0;
const int SOLAR_OFFSET_HOURS = 1;

Dusk2Dawn esposende(LATITUDE, LONGITUDE, TIMEZONE_OFFSET);

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

// ============= ESTRUTURA DE COR =============
struct SolarColor {
  uint8_t r, g, b;
  uint8_t brightness;
  int colorTemp;
};

// ============= DECLARACOES ANTECIPADAS =============
void fillPanel(SolarColor color);
SolarColor mapSolarElevationToColor(float elevation);
DateTime getCurrentTime();
void showBootAnimation();
void updateBootStatus(uint16_t color, int progress);
void showTimeOnBoot(DateTime now, uint16_t color);
SystemStatus connectWiFiAndSync();
bool tryConnectWiFi();
bool syncWithNTP();
void printSolarInfo();
float calculateSolarElevation(DateTime now);
void colorTempToRGB(int kelvin, uint8_t &r, uint8_t &g, uint8_t &b);
void displayAutoSolar();
void displayTherapyRed();
void displayOff();
void handleButton();
void updateDisplay();
uint16_t getStatusColor(SystemStatus status);
const char* getStatusName(SystemStatus status);

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

// ============= CONEXAO WIFI MULTI-SSID =============
bool tryConnectWiFi() {
  Serial.println("\n=== Tentando conectar WiFi ===");
  
  // Tentar cada rede
  for(int net = 0; net < NUM_WIFI_NETWORKS; net++) {
    Serial.printf("\nRede %d/%d: %s\n", net + 1, NUM_WIFI_NETWORKS, wifiNetworks[net].ssid);
    
    // Tentar cada rede múltiplas vezes
    for(int attempt = 0; attempt < WIFI_RETRY_PER_NETWORK; attempt++) {
      if(attempt > 0) {
        Serial.printf("  Tentativa %d/%d...\n", attempt + 1, WIFI_RETRY_PER_NETWORK);
      }
      
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifiNetworks[net].ssid, wifiNetworks[net].password);
      
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
        Serial.printf("   SSID: %s\n", wifiNetworks[net].ssid);
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

// ============= CONVERSAO TEMPERATURA DE COR =============
void colorTempToRGB(int kelvin, uint8_t &r, uint8_t &g, uint8_t &b) {
  float temp = kelvin / 100.0;
  float red, green, blue;
  
  if (temp <= 66) {
    red = 255;
  } else {
    red = temp - 60;
    red = 329.698727446 * pow(red, -0.1332047592);
    red = constrain(red, 0, 255);
  }
  
  if (temp <= 66) {
    green = temp;
    green = 99.4708025861 * log(green) - 161.1195681661;
  } else {
    green = temp - 60;
    green = 288.1221695283 * pow(green, -0.0755148492);
  }
  green = constrain(green, 0, 255);
  
  if (temp >= 66) {
    blue = 255;
  } else if (temp <= 19) {
    blue = 0;
  } else {
    blue = temp - 10;
    blue = 138.5177312231 * log(blue) - 305.0447927307;
    blue = constrain(blue, 0, 255);
  }
  
  r = (uint8_t)red;
  g = (uint8_t)green;
  b = (uint8_t)blue;
}

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
  Serial.println("║     RELÓGIO SOLAR LED v2.0        ║");
  Serial.println("║   Multi-WiFi + RTC Fallback       ║");
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
  
  // WiFi + NTP (com fallback automático)
  currentStatus = connectWiFiAndSync();
  
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
  Serial.printf("Modo inicial: AUTO_SOLAR\n");
  Serial.printf("Offset solar: %+d hora(s)\n\n", SOLAR_OFFSET_HOURS);
  
  updateDisplay();
}

// ============= INFORMACOES SOLARES =============
void printSolarInfo() {
  DateTime now = getCurrentTime();
  int sunrise = esposende.sunrise(now.year(), now.month(), now.day(), true);
  int sunset = esposende.sunset(now.year(), now.month(), now.day(), true);
  
  Serial.println("\n--- Info Solar ---");
  Serial.printf("Data: %02d/%02d/%04d\n", now.day(), now.month(), now.year());
  Serial.printf("Nascer: %02d:%02d\n", sunrise / 60, sunrise % 60);
  Serial.printf("Pôr: %02d:%02d\n", sunset / 60, sunset % 60);
  Serial.println("------------------\n");
}

// ============= CALCULO ELEVACAO SOLAR =============
float calculateSolarElevation(DateTime now) {
  int a = (14 - now.month()) / 12;
  int y = now.year() + 4800 - a;
  int m = now.month() + 12 * a - 3;
  
  int jdn = now.day() + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
  double jd = jdn + (now.hour() - 12.0) / 24.0 + now.minute() / 1440.0 + now.second() / 86400.0;
  double t = (jd - 2451545.0) / 36525.0;
  
  double L0 = fmod(280.46646 + t * (36000.76983 + t * 0.0003032), 360.0);
  double M = fmod(357.52911 + t * (35999.05029 - t * 0.0001537), 360.0);
  double M_rad = M * PI / 180.0;
  
  double C = (1.914602 - t * (0.004817 + t * 0.000014)) * sin(M_rad) +
             (0.019993 - t * 0.000101) * sin(2 * M_rad) +
             0.000289 * sin(3 * M_rad);
  
  double theta = L0 + C;
  double epsilon = 23.439291 - t * 0.0130042;
  double epsilon_rad = epsilon * PI / 180.0;
  double theta_rad = theta * PI / 180.0;
  
  double delta = asin(sin(epsilon_rad) * sin(theta_rad));
  double E = 4 * (L0 - 0.0057183 - atan2(tan(theta_rad), cos(epsilon_rad)) * 180.0 / PI + C);
  
  double solarTime = now.hour() * 60.0 + now.minute() + now.second() / 60.0 + 
                     E + 4 * LONGITUDE - 60 * TIMEZONE_OFFSET;
  
  double H = (solarTime / 4.0 - 180.0) * PI / 180.0;
  double phi = LATITUDE * PI / 180.0;
  
  double elevation = asin(sin(phi) * sin(delta) + cos(phi) * cos(delta) * cos(H));
  
  return elevation * 180.0 / PI;
}

// ============= MAPEAMENTO ELEVACAO PARA COR =============
SolarColor mapSolarElevationToColor(float elevation) {
  SolarColor color;
  int colorTemp;
  
  if (elevation < -18.0) {
    colorTemp = 0;
    color.r = 0; color.g = 0; color.b = 0;
    color.brightness = 0;
  }
  else if (elevation < -12.0) {
    colorTemp = 1500;
    float t = map((int)(elevation * 10), -180, -120, 0, 40);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  else if (elevation < -6.0) {
    colorTemp = map((int)(elevation * 10), -120, -60, 1500, 2000);
    float t = map((int)(elevation * 10), -120, -60, 40, 80);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  else if (elevation < -0.833) {
    colorTemp = map((int)(elevation * 10), -60, -8, 2000, 2800);
    float t = map((int)(elevation * 10), -60, -8, 80, 140);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  else if (elevation < 6.0) {
    colorTemp = map((int)(elevation * 10), -8, 60, 2800, 3800);
    float t = map((int)(elevation * 10), -8, 60, 140, 200);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  else if (elevation < 15.0) {
    colorTemp = map((int)(elevation * 10), 60, 150, 3800, 4800);
    float t = map((int)(elevation * 10), 60, 150, 200, 230);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  else if (elevation < 30.0) {
    colorTemp = map((int)(elevation * 10), 150, 300, 4800, 5500);
    float t = map((int)(elevation * 10), 150, 300, 230, 250);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  else if (elevation < 50.0) {
    colorTemp = map((int)(elevation * 10), 300, 500, 5500, 5900);
    color.brightness = 255;
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
  }
  else {
    colorTemp = 6500;
    color.brightness = 255;
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
  }
  
  color.colorTemp = colorTemp;
  return color;
}

// ============= MODO AUTO SOLAR =============
void displayAutoSolar() {
  DateTime now = getCurrentTime();
  
  DateTime solarTime = now;
  if (SOLAR_OFFSET_HOURS != 0) {
    int offsetMinutes = SOLAR_OFFSET_HOURS * 60;
    solarTime = now + TimeSpan(0, 0, offsetMinutes, 0);
  }
  
  float elevation = calculateSolarElevation(solarTime);
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
  if (buttonPressed) {
    buttonPressed = false;
    delay(20);
    
    if (digitalRead(BUTTON_PIN) == LOW) {
      currentMode = (Mode)((currentMode + 1) % 3);
      
      const char* modeNames[] = {"AUTO_SOLAR", "THERAPY_RED", "OFF"};
      Serial.printf("\n>>> MODO: %s <<<\n\n", modeNames[currentMode]);
      
      updateDisplay();
      
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
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
  
  handleButton();
  
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