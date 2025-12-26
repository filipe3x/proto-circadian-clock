#include "captive_portal.h"

// ============= VARIAVEIS GLOBAIS =============
WiFiCredentials wifiNetworks[2];
int NUM_WIFI_NETWORKS = 2;

DNSServer dnsServer;
AsyncWebServer webServer(80);
bool configMode = false;
unsigned long configModeStart = 0;

float configLatitude = DEFAULT_LATITUDE;
float configLongitude = DEFAULT_LONGITUDE;
int configTimezone = DEFAULT_TIMEZONE;
int configBrightness = 80;
int configSolarOffset = DEFAULT_SOLAR_OFFSET;

Dusk2Dawn* solarCalc = nullptr;

// IP configuration
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_GATEWAY(192, 168, 4, 1);
static const IPAddress AP_SUBNET(255, 255, 255, 0);

// Preferences para acesso ao NVS
static Preferences portalPrefs;

// ============= IMPLEMENTACAO =============

void initSolarCalc() {
  if (solarCalc != nullptr) {
    delete solarCalc;
  }
  solarCalc = new Dusk2Dawn(configLatitude, configLongitude, configTimezone);
  Serial.printf("[CONFIG] Solar calc: lat=%.4f, lon=%.4f, tz=%d\n",
                configLatitude, configLongitude, configTimezone);
}

bool hasStoredConfig() {
  portalPrefs.begin("clock", true);  // read-only
  bool hasConfig = portalPrefs.isKey("wifi_ssid_1");
  portalPrefs.end();
  Serial.printf("[CONFIG] Configuracao guardada: %s\n", hasConfig ? "SIM" : "NAO");
  return hasConfig;
}

void loadConfig() {
  portalPrefs.begin("clock", true);  // read-only

  // WiFi credentials
  wifiNetworks[0].ssid = portalPrefs.getString("wifi_ssid_1", "");
  wifiNetworks[0].password = portalPrefs.getString("wifi_pass_1", "");
  wifiNetworks[1].ssid = portalPrefs.getString("wifi_ssid_2", "");
  wifiNetworks[1].password = portalPrefs.getString("wifi_pass_2", "");

  // Localizacao
  configLatitude = portalPrefs.getFloat("latitude", DEFAULT_LATITUDE);
  configLongitude = portalPrefs.getFloat("longitude", DEFAULT_LONGITUDE);
  configTimezone = portalPrefs.getInt("timezone", DEFAULT_TIMEZONE);
  configBrightness = portalPrefs.getInt("brightness", 80);
  configSolarOffset = portalPrefs.getInt("solar_offset", DEFAULT_SOLAR_OFFSET);

  portalPrefs.end();

  // Contar redes validas
  NUM_WIFI_NETWORKS = 0;
  if (wifiNetworks[0].ssid.length() > 0) NUM_WIFI_NETWORKS++;
  if (wifiNetworks[1].ssid.length() > 0) NUM_WIFI_NETWORKS++;

  // DEBUG: Se nao ha redes configuradas, usar credenciais de fallback
  #if DEBUG_MODE
  if (NUM_WIFI_NETWORKS == 0) {
    wifiNetworks[0].ssid = FALLBACK_WIFI_SSID;
    wifiNetworks[0].password = FALLBACK_WIFI_PASS;
    NUM_WIFI_NETWORKS = 1;
    Serial.println("[DEBUG] Usando credenciais WiFi de fallback");
  }
  #endif

  Serial.println("[CONFIG] Configuracao carregada:");
  Serial.printf("  WiFi 1: %s\n", wifiNetworks[0].ssid.c_str());
  Serial.printf("  WiFi 2: %s\n", wifiNetworks[1].ssid.length() > 0 ? wifiNetworks[1].ssid.c_str() : "(nao configurado)");
  Serial.printf("  Lat: %.4f, Lon: %.4f, TZ: %d\n", configLatitude, configLongitude, configTimezone);
  Serial.printf("  Brilho: %d, Offset Solar: %+d h\n", configBrightness, configSolarOffset);
}

void startCaptivePortal(void* displayPtr) {
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║     CAPTIVE PORTAL INICIADO       ║");
  Serial.println("╚═══════════════════════════════════╝");

  configMode = true;
  configModeStart = millis();

  // Configurar Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASS);

  // DNS - redireciona tudo para o ESP32
  dnsServer.start(53, "*", AP_IP);

  // Configurar web server
  setupCaptiveWebServer();

  Serial.printf("SSID: %s\n", AP_SSID);
  Serial.printf("IP: %s\n", AP_IP.toString().c_str());
  Serial.println("Aguardando configuracao...\n");
}

void setupCaptiveWebServer() {
  // Pagina principal de configuracao
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getConfigPage());
  });

  // Processar formulario
  webServer.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Guardar configuracoes
    portalPrefs.begin("clock", false);  // read-write

    if (request->hasParam("ssid1", true)) {
      portalPrefs.putString("wifi_ssid_1", request->getParam("ssid1", true)->value());
    }
    if (request->hasParam("pass1", true)) {
      portalPrefs.putString("wifi_pass_1", request->getParam("pass1", true)->value());
    }
    if (request->hasParam("ssid2", true)) {
      portalPrefs.putString("wifi_ssid_2", request->getParam("ssid2", true)->value());
    }
    if (request->hasParam("pass2", true)) {
      portalPrefs.putString("wifi_pass_2", request->getParam("pass2", true)->value());
    }
    if (request->hasParam("lat", true)) {
      portalPrefs.putFloat("latitude", request->getParam("lat", true)->value().toFloat());
    }
    if (request->hasParam("lon", true)) {
      portalPrefs.putFloat("longitude", request->getParam("lon", true)->value().toFloat());
    }
    if (request->hasParam("tz", true)) {
      portalPrefs.putInt("timezone", request->getParam("tz", true)->value().toInt());
    }
    if (request->hasParam("bright", true)) {
      portalPrefs.putInt("brightness", request->getParam("bright", true)->value().toInt());
    }
    if (request->hasParam("solar_offset", true)) {
      portalPrefs.putInt("solar_offset", request->getParam("solar_offset", true)->value().toInt());
    }

    portalPrefs.end();

    Serial.println("[CONFIG] Configuracao guardada!");

    // Resposta de sucesso
    request->send(200, "text/html", getSuccessPage());

    // Agendar reinicio
    delay(2000);
    ESP.restart();
  });

  // Captive portal detection (iOS, Android, Windows)
  webServer.on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect);
  webServer.on("/generate_204", HTTP_GET, handleCaptiveRedirect);
  webServer.on("/fwlink", HTTP_GET, handleCaptiveRedirect);
  webServer.on("/connecttest.txt", HTTP_GET, handleCaptiveRedirect);
  webServer.on("/redirect", HTTP_GET, handleCaptiveRedirect);
  webServer.on("/success.txt", HTTP_GET, handleCaptiveRedirect);
  webServer.onNotFound(handleCaptiveRedirect);

  webServer.begin();
  Serial.println("[WEB] Servidor iniciado");
}

void handleCaptiveRedirect(AsyncWebServerRequest *request) {
  request->redirect("http://192.168.4.1/");
}

bool processCaptivePortal() {
  if (!configMode) return false;

  dnsServer.processNextRequest();

  // Timeout de 5 minutos sem atividade
  if (millis() - configModeStart > SETUP_TIMEOUT) {
    Serial.println("\n[CONFIG] Timeout - reiniciando...");
    ESP.restart();
    return false;
  }

  return true;
}

String getConfigPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Circadian Clock — Setup</title>
  <style>
    :root {
      --bg-deep: #0a0e1a;
      --bg-card: #111827;
      --accent-lavender: #a78bfa;
      --accent-amber: #fbbf24;
      --accent-moon: #e0e7ff;
      --accent-mint: #6ee7b7;
      --text-primary: #f1f5f9;
      --text-secondary: #94a3b8;
      --text-muted: #64748b;
      --border-subtle: rgba(167, 139, 250, 0.15);
    }
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: var(--bg-deep);
      min-height: 100vh;
      padding: 1rem;
      color: var(--text-primary);
    }
    .container { max-width: 480px; margin: 0 auto; }
    .header { text-align: center; padding: 2rem 0; }
    .moon-icon { width: 48px; height: 48px; margin-bottom: 1rem; }
    .title { font-size: 1.75rem; font-weight: 300; color: var(--text-primary); letter-spacing: 0.05em; }
    .subtitle { font-size: 0.8rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.15em; margin-top: 0.5rem; }
    .card {
      background: linear-gradient(145deg, rgba(17, 24, 39, 0.95), rgba(15, 23, 42, 0.9));
      border: 1px solid var(--border-subtle);
      border-radius: 16px;
      padding: 1.5rem;
      margin-bottom: 1rem;
      backdrop-filter: blur(10px);
      animation: fadeIn 0.5s ease-out forwards;
    }
    .card:nth-child(2) { animation-delay: 0.1s; }
    .card:nth-child(3) { animation-delay: 0.2s; }
    .card:nth-child(4) { animation-delay: 0.3s; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }
    .card-title { display: flex; align-items: center; gap: 0.75rem; margin-bottom: 1rem; color: var(--accent-lavender); font-size: 0.9rem; font-weight: 500; }
    .card-icon { width: 20px; height: 20px; opacity: 0.8; }
    label { display: block; font-size: 0.8rem; color: var(--text-secondary); margin-bottom: 0.4rem; margin-top: 1rem; }
    label:first-of-type { margin-top: 0; }
    input, select {
      width: 100%; padding: 0.875rem 1rem;
      border: 1px solid var(--border-subtle); border-radius: 10px;
      background: rgba(10, 14, 26, 0.8); color: var(--text-primary);
      font-size: 1rem; transition: all 0.2s;
    }
    input:focus, select:focus { outline: none; border-color: var(--accent-lavender); box-shadow: 0 0 0 3px rgba(167, 139, 250, 0.15); }
    input::placeholder { color: var(--text-muted); }
    .row { display: grid; grid-template-columns: 1fr 1fr auto; gap: 0.75rem; align-items: end; }
    .btn-gps {
      padding: 0.875rem 1rem;
      background: rgba(110, 231, 183, 0.15); border: 1px solid rgba(110, 231, 183, 0.3);
      border-radius: 10px; color: var(--accent-mint);
      font-size: 0.85rem; cursor: pointer; transition: all 0.2s; white-space: nowrap;
    }
    .btn-gps:hover { background: rgba(110, 231, 183, 0.25); }
    .hint { font-size: 0.7rem; color: var(--text-muted); margin-top: 0.5rem; }
    .slider-container { display: flex; align-items: center; gap: 1rem; }
    input[type="range"] { flex: 1; -webkit-appearance: none; height: 6px; border-radius: 3px; background: rgba(167, 139, 250, 0.2); border: none; padding: 0; }
    input[type="range"]::-webkit-slider-thumb { -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%; background: var(--accent-lavender); cursor: pointer; box-shadow: 0 2px 8px rgba(167, 139, 250, 0.4); }
    .slider-value { min-width: 3rem; text-align: center; color: var(--accent-moon); font-weight: 500; }
    .btn-submit {
      width: 100%; padding: 1rem; margin-top: 1.5rem; border: none; border-radius: 12px;
      background: linear-gradient(135deg, var(--accent-lavender), #8b5cf6);
      color: white; font-size: 1rem; font-weight: 600; cursor: pointer;
      transition: all 0.3s; box-shadow: 0 4px 15px rgba(167, 139, 250, 0.3);
    }
    .btn-submit:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(167, 139, 250, 0.4); }
    .btn-submit:active { transform: translateY(0); }
    .footer { text-align: center; padding: 1.5rem; color: var(--text-muted); font-size: 0.75rem; }
  </style>
</head>
<body>
  <div class="container">
    <header class="header">
      <svg class="moon-icon" viewBox="0 0 48 48" fill="none">
        <path d="M24 4C12.954 4 4 12.954 4 24s8.954 20 20 20c7.862 0 14.691-4.535 17.957-11.128.386-.779-.322-1.628-1.166-1.372A16.002 16.002 0 0120 16c0-3.527 1.14-6.787 3.072-9.428.481-.659.066-1.572-.756-1.572H24z" fill="url(#moonGrad)"/>
        <defs><linearGradient id="moonGrad" x1="4" y1="4" x2="44" y2="44"><stop stop-color="#e0e7ff"/><stop offset="1" stop-color="#a78bfa"/></linearGradient></defs>
      </svg>
      <h1 class="title">Circadian Clock</h1>
      <p class="subtitle">Configuracao Inicial</p>
    </header>

    <form action="/save" method="POST">
      <div class="card">
        <div class="card-title">
          <svg class="card-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
            <path d="M8.288 15.038a5.25 5.25 0 017.424 0M5.106 11.856c3.807-3.808 9.98-3.808 13.788 0M1.924 8.674c5.565-5.565 14.587-5.565 20.152 0M12.53 18.22l-.53.53-.53-.53a.75.75 0 011.06 0z"/>
          </svg>
          WiFi Principal
        </div>
        <label>Nome da rede (SSID)</label>
        <input type="text" name="ssid1" placeholder="Nome da rede WiFi">
        <label>Password</label>
        <input type="password" name="pass1" placeholder="Password da rede">
      </div>

      <div class="card">
        <div class="card-title">
          <svg class="card-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
            <path d="M8.288 15.038a5.25 5.25 0 017.424 0M5.106 11.856c3.807-3.808 9.98-3.808 13.788 0"/>
          </svg>
          WiFi Secundario (opcional)
        </div>
        <label>Nome da rede (SSID)</label>
        <input type="text" name="ssid2" placeholder="Rede alternativa">
        <label>Password</label>
        <input type="password" name="pass2" placeholder="Password">
      </div>

      <div class="card">
        <div class="card-title">
          <svg class="card-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
            <path d="M15 10.5a3 3 0 11-6 0 3 3 0 016 0z"/>
            <path d="M19.5 10.5c0 7.142-7.5 11.25-7.5 11.25S4.5 17.642 4.5 10.5a7.5 7.5 0 1115 0z"/>
          </svg>
          Localizacao
        </div>
        <div class="row">
          <div>
            <label>Latitude</label>
            <input type="number" name="lat" step="0.0001" value=")rawliteral";

  html += String(configLatitude, 4);
  html += R"rawliteral(" required>
          </div>
          <div>
            <label>Longitude</label>
            <input type="number" name="lon" step="0.0001" value=")rawliteral";

  html += String(configLongitude, 4);
  html += R"rawliteral(" required>
          </div>
          <div>
            <label>&nbsp;</label>
            <button type="button" class="btn-gps" onclick="getGPS()">📍 GPS</button>
          </div>
        </div>
        <p class="hint">Clica GPS para usar a localizacao do telemovel</p>

        <label>Fuso horario</label>
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
          <option value="0")rawliteral";

  if (configTimezone == 0) html += " selected";
  html += R"rawliteral(>UTC+0 (Lisboa, Londres)</option>
          <option value="1")rawliteral";

  if (configTimezone == 1) html += " selected";
  html += R"rawliteral(>UTC+1 (Paris, Berlin)</option>
          <option value="2">UTC+2</option>
          <option value="3">UTC+3 (Moscow)</option>
          <option value="4">UTC+4</option>
          <option value="5">UTC+5</option>
          <option value="6">UTC+6</option>
          <option value="7">UTC+7</option>
          <option value="8">UTC+8 (Singapore, Hong Kong)</option>
          <option value="9">UTC+9 (Tokyo)</option>
          <option value="10">UTC+10 (Sydney)</option>
          <option value="11">UTC+11</option>
          <option value="12">UTC+12</option>
        </select>
      </div>

      <div class="card">
        <div class="card-title">
          <svg class="card-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5">
            <path d="M12 3v2.25m6.364.386l-1.591 1.591M21 12h-2.25m-.386 6.364l-1.591-1.591M12 18.75V21m-4.773-4.227l-1.591 1.591M5.25 12H3m4.227-4.773L5.636 5.636M15.75 12a3.75 3.75 0 11-7.5 0 3.75 3.75 0 017.5 0z"/>
          </svg>
          Display
        </div>
        <label>Brilho</label>
        <div class="slider-container">
          <input type="range" name="bright" min="10" max="255" value=")rawliteral";

  html += String(configBrightness);
  html += R"rawliteral(" oninput="document.getElementById('bval').textContent=this.value">
          <span class="slider-value" id="bval">)rawliteral";

  html += String(configBrightness);
  html += R"rawliteral(</span>
        </div>

        <label style="margin-top: 1.5rem;">Ajuste de Despertar</label>
        <div class="slider-container" style="gap: 0.5rem;">
          <svg class="slider-icon" viewBox="0 0 24 24" fill="currentColor" style="width:24px;height:24px;color:var(--accent-moon);flex-shrink:0;">
            <path d="M3 5a2 2 0 012-2h14a2 2 0 012 2v2a1 1 0 01-1 1H4a1 1 0 01-1-1V5zm0 6a1 1 0 011-1h16a1 1 0 011 1v7a2 2 0 01-2 2H5a2 2 0 01-2-2v-7zm3 2v3h12v-3H6z"/>
          </svg>
          <input type="range" name="solar_offset" min="-3" max="3" value=")rawliteral";

  html += String(configSolarOffset);
  html += R"rawliteral(" oninput="updateOffsetLabel(this.value)" style="flex:1;">
          <svg class="slider-icon" viewBox="0 0 24 24" fill="currentColor" style="width:24px;height:24px;color:var(--accent-amber);flex-shrink:0;">
            <path d="M12 2.25a.75.75 0 01.75.75v2.25a.75.75 0 01-1.5 0V3a.75.75 0 01.75-.75zM7.5 12a4.5 4.5 0 119 0 4.5 4.5 0 01-9 0zM18.894 6.166a.75.75 0 00-1.06-1.06l-1.591 1.59a.75.75 0 101.06 1.061l1.591-1.59zM21.75 12a.75.75 0 01-.75.75h-2.25a.75.75 0 010-1.5H21a.75.75 0 01.75.75zM17.834 18.894a.75.75 0 001.06-1.06l-1.59-1.591a.75.75 0 10-1.061 1.06l1.59 1.591zM12 18a.75.75 0 01.75.75V21a.75.75 0 01-1.5 0v-2.25A.75.75 0 0112 18zM7.758 17.303a.75.75 0 00-1.061-1.06l-1.591 1.59a.75.75 0 001.06 1.061l1.591-1.59zM6 12a.75.75 0 01-.75.75H3a.75.75 0 010-1.5h2.25A.75.75 0 016 12zM6.697 7.757a.75.75 0 001.06-1.06l-1.59-1.591a.75.75 0 00-1.061 1.06l1.59 1.591z"/>
          </svg>
          <span class="slider-value" id="soval" style="min-width:4rem;">)rawliteral";

  // Valor inicial do offset
  if (configSolarOffset > 0) {
    html += "+" + String(configSolarOffset) + "h cedo";
  } else if (configSolarOffset < 0) {
    html += String(configSolarOffset) + "h tarde";
  } else {
    html += "Normal";
  }

  html += R"rawliteral(</span>
        </div>
        <p class="hint">🛏️ Dormir ate mais tarde ← → Acordar mais cedo ☀️</p>
      </div>

      <button type="submit" class="btn-submit">Guardar e Reiniciar</button>
    </form>

    <footer class="footer">
      <p>Circadian Clock &bull; Luz natural sincronizada</p>
    </footer>
  </div>

  <script>
    function getGPS() {
      if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(
          function(pos) {
            document.querySelector('[name=lat]').value = pos.coords.latitude.toFixed(4);
            document.querySelector('[name=lon]').value = pos.coords.longitude.toFixed(4);
          },
          function(err) { alert('Erro ao obter localizacao: ' + err.message); },
          { enableHighAccuracy: true, timeout: 10000 }
        );
      } else { alert('Geolocalizacao nao suportada neste browser'); }
    }
    function updateOffsetLabel(val) {
      var label = document.getElementById('soval');
      var v = parseInt(val);
      if (v > 0) {
        label.textContent = '+' + v + 'h cedo';
      } else if (v < 0) {
        label.textContent = v + 'h tarde';
      } else {
        label.textContent = 'Normal';
      }
    }
  </script>
</body>
</html>
)rawliteral";

  return html;
}

String getSuccessPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="pt">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Configuracao Guardada</title>
  <style>
    :root { --bg-deep: #0a0e1a; --accent-mint: #6ee7b7; --accent-lavender: #a78bfa; --text-primary: #f1f5f9; --text-secondary: #94a3b8; }
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: var(--bg-deep); min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 2rem; color: var(--text-primary); }
    .container { text-align: center; animation: fadeIn 0.5s ease-out; }
    @keyframes fadeIn { from { opacity: 0; transform: scale(0.95); } to { opacity: 1; transform: scale(1); } }
    .check { width: 80px; height: 80px; border-radius: 50%; background: rgba(110, 231, 183, 0.15); display: flex; align-items: center; justify-content: center; margin: 0 auto 1.5rem; animation: pulse 2s ease-in-out infinite; }
    @keyframes pulse { 0%, 100% { box-shadow: 0 0 0 0 rgba(110, 231, 183, 0.4); } 50% { box-shadow: 0 0 0 20px rgba(110, 231, 183, 0); } }
    .check svg { width: 40px; height: 40px; stroke: var(--accent-mint); }
    h1 { font-size: 1.5rem; font-weight: 400; margin-bottom: 0.75rem; color: var(--accent-mint); }
    p { color: var(--text-secondary); font-size: 0.9rem; line-height: 1.6; }
    .loader { margin-top: 2rem; display: flex; justify-content: center; gap: 0.5rem; }
    .loader span { width: 8px; height: 8px; border-radius: 50%; background: var(--accent-lavender); animation: bounce 1.4s ease-in-out infinite; }
    .loader span:nth-child(1) { animation-delay: 0s; }
    .loader span:nth-child(2) { animation-delay: 0.2s; }
    .loader span:nth-child(3) { animation-delay: 0.4s; }
    @keyframes bounce { 0%, 80%, 100% { transform: scale(0.6); opacity: 0.5; } 40% { transform: scale(1); opacity: 1; } }
  </style>
</head>
<body>
  <div class="container">
    <div class="check">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 13l4 4L19 7"/></svg>
    </div>
    <h1>Configuracao Guardada!</h1>
    <p>O dispositivo vai reiniciar e conectar-se<br>a rede WiFi configurada.</p>
    <p style="margin-top: 1rem; font-size: 0.8rem;">Pode fechar esta pagina.</p>
    <div class="loader"><span></span><span></span><span></span></div>
  </div>
</body>
</html>
)rawliteral";
}
