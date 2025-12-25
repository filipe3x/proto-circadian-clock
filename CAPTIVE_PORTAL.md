# Captive Portal para Configuração Mobile

Este documento descreve a implementação de um sistema de configuração via telemóvel para o relógio circadiano ESP32, sem necessidade de instalar aplicações.

## Conceito

Um **Captive Portal** é uma técnica onde o ESP32 cria a sua própria rede WiFi (Access Point). Quando um dispositivo se conecta, é automaticamente redirecionado para uma página de configuração - similar ao que acontece em redes WiFi de hotéis ou aeroportos.

## Vantagens

- Funciona em qualquer telemóvel (iOS, Android, qualquer browser)
- Não requer instalação de apps
- Experiência familiar para o utilizador
- Configuração intuitiva via formulário web
- Leve para o ESP32 (~50-80KB Flash, ~15-20KB RAM)

## Fluxo de Utilizador

```
┌─────────────────────────────────────────────────────────────────┐
│                         PRIMEIRO USO                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Liga o ESP32                                                │
│        ↓                                                        │
│  2. Não há WiFi configurado                                     │
│        ↓                                                        │
│  3. ESP32 cria rede "CircadianClock-Setup"                      │
│        ↓                                                        │
│  4. Display mostra: "SETUP" + "WiFi: Clock"                     │
│        ↓                                                        │
│  5. Utilizador conecta o telemóvel à rede                       │
│        ↓                                                        │
│  6. Página de configuração abre AUTOMATICAMENTE                 │
│        ↓                                                        │
│  7. Preenche: WiFi, password, localização, brilho               │
│        ↓                                                        │
│  8. Clica "Guardar" → ESP32 reinicia                            │
│        ↓                                                        │
│  9. Conecta à rede configurada → Funciona normalmente           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      USO NORMAL (após config)                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  • ESP32 liga → Conecta ao WiFi guardado → Funciona             │
│                                                                  │
│  • Para reconfigurar: Pressionar botão 5 segundos               │
│    → Entra em modo Setup novamente                              │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Arquitetura Técnica

### Bibliotecas Necessárias

```cpp
#include <WiFi.h>              // Já incluída no projeto
#include <ESPAsyncWebServer.h> // Web server non-blocking e leve
#include <AsyncTCP.h>          // Dependência do ESPAsyncWebServer
#include <DNSServer.h>         // Redireccionamento para captive portal
#include <Preferences.h>       // Armazenamento NVS (já no ESP32-core)
```

**Instalação via Arduino Library Manager:**
- `ESPAsyncWebServer` by me-no-dev
- `AsyncTCP` by me-no-dev

### Estrutura de Ficheiros

```
clock.ino                 # Código principal (já existente)
├── Secção: Captive Portal
│   ├── setupAP()         # Inicia Access Point
│   ├── setupWebServer()  # Configura rotas HTTP
│   ├── setupDNS()        # Configura DNS para redirect
│   └── handleConfig()    # Processa formulário
│
├── Secção: Preferences
│   ├── loadConfig()      # Carrega do NVS
│   └── saveConfig()      # Guarda no NVS
│
└── HTML embebido         # Interface minimalista (~3KB)
```

### Modos de Operação

```cpp
enum DeviceMode {
  MODE_SETUP,      // Captive portal activo, a aguardar configuração
  MODE_CONNECTING, // A tentar conectar ao WiFi configurado
  MODE_RUNNING     // Funcionamento normal
};
```

## Configurações Disponíveis

| Configuração | Tipo | Descrição |
|--------------|------|-----------|
| `wifi_ssid_1` | String | Rede WiFi principal |
| `wifi_pass_1` | String | Password da rede principal |
| `wifi_ssid_2` | String | Rede WiFi secundária (opcional) |
| `wifi_pass_2` | String | Password da rede secundária |
| `latitude` | Float | Latitude para cálculos solares |
| `longitude` | Float | Longitude para cálculos solares |
| `timezone` | Int | Offset UTC em horas |
| `brightness` | Int | Brilho do display (0-255) |

## Implementação

### 1. Constantes e Variáveis Globais

```cpp
// Captive Portal
const char* AP_SSID = "CircadianClock";
const char* AP_PASS = "";  // Aberta para facilitar acesso
const IPAddress AP_IP(192, 168, 4, 1);

DNSServer dnsServer;
AsyncWebServer server(80);
Preferences preferences;

bool configMode = false;
```

### 2. Verificar se há Configuração Guardada

```cpp
bool hasStoredConfig() {
  preferences.begin("clock", true);  // read-only
  bool hasConfig = preferences.isKey("wifi_ssid_1");
  preferences.end();
  return hasConfig;
}
```

### 3. Carregar Configuração

```cpp
void loadConfig() {
  preferences.begin("clock", true);

  String ssid1 = preferences.getString("wifi_ssid_1", "");
  String pass1 = preferences.getString("wifi_pass_1", "");
  String ssid2 = preferences.getString("wifi_ssid_2", "");
  String pass2 = preferences.getString("wifi_pass_2", "");

  LATITUDE = preferences.getFloat("latitude", 41.5362);
  LONGITUDE = preferences.getFloat("longitude", -8.7813);
  TIMEZONE_OFFSET = preferences.getInt("timezone", 0);
  displayBrightness = preferences.getInt("brightness", 80);

  preferences.end();

  // Configurar WiFi credentials
  wifiNetworks[0] = {ssid1.c_str(), pass1.c_str()};
  wifiNetworks[1] = {ssid2.c_str(), pass2.c_str()};
}
```

### 4. Iniciar Access Point

```cpp
void startCaptivePortal() {
  configMode = true;

  // Configurar AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASS);

  // DNS - redireciona tudo para o ESP32
  dnsServer.start(53, "*", AP_IP);

  // Mostrar no display
  display->clearScreen();
  display->setTextColor(display->color565(255, 150, 0));
  display->setCursor(4, 0);
  display->print("SETUP");
  display->setCursor(0, 9);
  display->setTextColor(display->color565(100, 100, 100));
  display->print("WiFi:Clock");

  Serial.println("Captive Portal iniciado");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(AP_IP);
}
```

### 5. Configurar Web Server

```cpp
void setupWebServer() {
  // Página principal de configuração
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getConfigPage());
  });

  // Processar formulário
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Guardar configurações
    preferences.begin("clock", false);

    if (request->hasParam("ssid1", true)) {
      preferences.putString("wifi_ssid_1", request->getParam("ssid1", true)->value());
    }
    if (request->hasParam("pass1", true)) {
      preferences.putString("wifi_pass_1", request->getParam("pass1", true)->value());
    }
    if (request->hasParam("ssid2", true)) {
      preferences.putString("wifi_ssid_2", request->getParam("ssid2", true)->value());
    }
    if (request->hasParam("pass2", true)) {
      preferences.putString("wifi_pass_2", request->getParam("pass2", true)->value());
    }
    if (request->hasParam("lat", true)) {
      preferences.putFloat("latitude", request->getParam("lat", true)->value().toFloat());
    }
    if (request->hasParam("lon", true)) {
      preferences.putFloat("longitude", request->getParam("lon", true)->value().toFloat());
    }
    if (request->hasParam("tz", true)) {
      preferences.putInt("timezone", request->getParam("tz", true)->value().toInt());
    }
    if (request->hasParam("bright", true)) {
      preferences.putInt("brightness", request->getParam("bright", true)->value().toInt());
    }

    preferences.end();

    // Resposta e reinício
    request->send(200, "text/html", getSuccessPage());
    delay(2000);
    ESP.restart();
  });

  // Captive portal detection (iOS, Android, Windows)
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortal);
  server.on("/generate_204", HTTP_GET, handleCaptivePortal);
  server.on("/fwlink", HTTP_GET, handleCaptivePortal);
  server.onNotFound(handleCaptivePortal);

  server.begin();
}

void handleCaptivePortal(AsyncWebServerRequest *request) {
  request->redirect("http://192.168.4.1/");
}
```

### 6. Interface HTML (Minimalista)

```cpp
String getConfigPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Circadian Clock Setup</title>
  <style>
    * { box-sizing: border-box; font-family: -apple-system, sans-serif; }
    body { margin: 0; padding: 20px; background: #1a1a2e; color: #eee; }
    h1 { color: #ff9500; font-size: 1.5em; text-align: center; }
    .card { background: #252540; border-radius: 12px; padding: 20px; margin: 15px 0; }
    h2 { margin: 0 0 15px 0; font-size: 1.1em; color: #888; }
    label { display: block; margin: 10px 0 5px; font-size: 0.9em; }
    input, select { width: 100%; padding: 12px; border: none; border-radius: 8px;
                    background: #1a1a2e; color: #fff; font-size: 1em; }
    input:focus { outline: 2px solid #ff9500; }
    button { width: 100%; padding: 15px; margin-top: 20px; border: none;
             border-radius: 8px; background: #ff9500; color: #000;
             font-size: 1.1em; font-weight: bold; cursor: pointer; }
    button:active { background: #cc7700; }
    .row { display: flex; gap: 10px; }
    .row > div { flex: 1; }
    .loc-btn { padding: 12px; background: #333; color: #fff; margin-top: 27px; }
    small { color: #666; }
  </style>
</head>
<body>
  <h1>Circadian Clock</h1>

  <form action="/save" method="POST">
    <div class="card">
      <h2>WiFi Principal</h2>
      <label>Nome da rede (SSID)</label>
      <input type="text" name="ssid1" required placeholder="Nome da rede WiFi">
      <label>Password</label>
      <input type="password" name="pass1" placeholder="Password da rede">
    </div>

    <div class="card">
      <h2>WiFi Secundario (opcional)</h2>
      <label>Nome da rede (SSID)</label>
      <input type="text" name="ssid2" placeholder="Rede alternativa">
      <label>Password</label>
      <input type="password" name="pass2" placeholder="Password">
    </div>

    <div class="card">
      <h2>Localizacao</h2>
      <div class="row">
        <div>
          <label>Latitude</label>
          <input type="number" name="lat" step="0.0001" value="41.5362" required>
        </div>
        <div>
          <label>Longitude</label>
          <input type="number" name="lon" step="0.0001" value="-8.7813" required>
        </div>
        <div>
          <button type="button" class="loc-btn" onclick="getLocation()">GPS</button>
        </div>
      </div>
      <small>Ou clica GPS para usar a localizacao do telemovel</small>

      <label>Fuso horario (UTC)</label>
      <select name="tz">
        <option value="-12">UTC-12</option>
        <option value="-11">UTC-11</option>
        <option value="-10">UTC-10 (Hawaii)</option>
        <option value="-9">UTC-9 (Alaska)</option>
        <option value="-8">UTC-8 (Los Angeles)</option>
        <option value="-7">UTC-7 (Denver)</option>
        <option value="-6">UTC-6 (Chicago)</option>
        <option value="-5">UTC-5 (New York)</option>
        <option value="-4">UTC-4</option>
        <option value="-3">UTC-3 (Brasilia)</option>
        <option value="-2">UTC-2</option>
        <option value="-1">UTC-1 (Azores)</option>
        <option value="0" selected>UTC+0 (Lisboa, Londres)</option>
        <option value="1">UTC+1 (Paris, Berlin)</option>
        <option value="2">UTC+2</option>
        <option value="3">UTC+3 (Moscow)</option>
        <option value="4">UTC+4</option>
        <option value="5">UTC+5</option>
        <option value="5.5">UTC+5:30 (India)</option>
        <option value="6">UTC+6</option>
        <option value="7">UTC+7</option>
        <option value="8">UTC+8 (Singapore)</option>
        <option value="9">UTC+9 (Tokyo)</option>
        <option value="10">UTC+10 (Sydney)</option>
        <option value="11">UTC+11</option>
        <option value="12">UTC+12</option>
      </select>
    </div>

    <div class="card">
      <h2>Display</h2>
      <label>Brilho: <span id="bv">80</span></label>
      <input type="range" name="bright" min="10" max="255" value="80"
             oninput="document.getElementById('bv').textContent=this.value">
    </div>

    <button type="submit">Guardar e Reiniciar</button>
  </form>

  <script>
    function getLocation() {
      if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(function(pos) {
          document.querySelector('[name=lat]').value = pos.coords.latitude.toFixed(4);
          document.querySelector('[name=lon]').value = pos.coords.longitude.toFixed(4);
        }, function(err) {
          alert('Erro ao obter localizacao: ' + err.message);
        });
      } else {
        alert('Geolocalizacao nao suportada');
      }
    }
  </script>
</body>
</html>
)rawliteral";
}

String getSuccessPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: -apple-system, sans-serif; background: #1a1a2e;
           color: #eee; text-align: center; padding: 50px 20px; }
    .check { font-size: 4em; color: #4CAF50; }
    h1 { color: #ff9500; }
  </style>
</head>
<body>
  <div class="check">&#10004;</div>
  <h1>Configuracao Guardada!</h1>
  <p>O dispositivo vai reiniciar...</p>
  <p>Pode fechar esta pagina.</p>
</body>
</html>
)rawliteral";
}
```

### 7. Integração no Loop Principal

```cpp
void setup() {
  // ... inicialização existente ...

  // Verificar se há configuração guardada
  if (!hasStoredConfig()) {
    startCaptivePortal();
    setupWebServer();
  } else {
    loadConfig();
    // Continuar com boot normal...
  }
}

void loop() {
  if (configMode) {
    dnsServer.processNextRequest();
    // Não processar nada mais em modo config
    return;
  }

  // ... loop normal existente ...
}
```

### 8. Botão para Entrar em Modo Setup

```cpp
// Na função de handling do botão (já existente)
void handleButtonPress() {
  static unsigned long pressStart = 0;

  if (digitalRead(BUTTON_PIN) == LOW) {
    if (pressStart == 0) {
      pressStart = millis();
    } else if (millis() - pressStart > 5000) {
      // Long press > 5 segundos
      Serial.println("Entrando em modo configuração...");
      startCaptivePortal();
      setupWebServer();
      pressStart = 0;
    }
  } else {
    if (pressStart > 0 && millis() - pressStart < 1000) {
      // Short press - comportamento normal (trocar modo)
      cycleMode();
    }
    pressStart = 0;
  }
}
```

## Consumo de Recursos

| Recurso | Sem Portal | Com Portal | Diferença |
|---------|------------|------------|-----------|
| Flash | ~650 KB | ~720 KB | +70 KB |
| RAM (idle) | ~45 KB | ~60 KB | +15 KB |
| RAM (activo) | ~45 KB | ~80 KB | +35 KB |

**Nota:** O portal só usa RAM adicional enquanto está activo. Em funcionamento normal, o impacto é mínimo.

## Segurança

### Considerações

1. **Rede aberta por design** - Facilita o acesso inicial. A configuração acontece localmente.

2. **Sem autenticação** - Qualquer pessoa na proximidade pode reconfigurar. Aceitável para uso doméstico.

3. **Passwords WiFi** - Guardadas em NVS (encriptado por hardware no ESP32).

### Melhorias Opcionais

```cpp
// Password para aceder ao portal
const char* AP_PASS = "clock1234";

// Timeout - sai do modo setup após 5 minutos sem actividade
const unsigned long SETUP_TIMEOUT = 300000;
```

## Testes

### Checklist de Validação

- [ ] ESP32 cria rede WiFi quando não há config
- [ ] Telemóvel conecta e abre página automaticamente
- [ ] Formulário submete e guarda dados
- [ ] ESP32 reinicia e conecta ao WiFi configurado
- [ ] Botão long-press entra em modo setup
- [ ] GPS do telemóvel preenche coordenadas
- [ ] Configurações persistem após power cycle

### Dispositivos a Testar

- [ ] iPhone (Safari)
- [ ] Android (Chrome)
- [ ] Android (Samsung Internet)
- [ ] Windows laptop
- [ ] MacBook

## Referências

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ESP32 Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)
- [Captive Portal Detection](https://en.wikipedia.org/wiki/Captive_portal#Detection)
