#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Dusk2Dawn.h>

// ============= CONFIGURACAO CAPTIVE PORTAL =============
#define AP_SSID "CircadianClock"
#define AP_PASS ""  // Aberta para facilitar acesso
#define SETUP_TIMEOUT 300000  // 5 minutos sem atividade
#define BUTTON_LONG_PRESS_MS 5000  // 5 segundos para entrar em setup

// Flag de debug - quando 1, permite WiFi vazio (usa credenciais de fallback)
#define DEBUG_MODE 1

// Valores padrao de localizacao
#define DEFAULT_LATITUDE 41.5362
#define DEFAULT_LONGITUDE -8.7813
#define DEFAULT_TIMEZONE 0
#define DEFAULT_SOLAR_OFFSET 0

// Credenciais de fallback para debug (quando DEBUG_MODE=1 e campos vazios)
#define FALLBACK_WIFI_SSID "YourSSID"
#define FALLBACK_WIFI_PASS "YourPassword"

// ============= ESTRUTURAS =============
struct WiFiCredentials {
  String ssid;
  String password;
};

// ============= VARIAVEIS EXTERNAS =============
// Estas variaveis sao definidas em captive_portal.cpp
extern WiFiCredentials wifiNetworks[2];
extern int NUM_WIFI_NETWORKS;

extern DNSServer dnsServer;
extern AsyncWebServer webServer;
extern bool configMode;
extern unsigned long configModeStart;

extern float configLatitude;
extern float configLongitude;
extern int configTimezone;
extern int configBrightness;
extern int configSolarOffset;  // Offset solar em horas (-12 a +12)

extern Dusk2Dawn* solarCalc;

// ============= FUNCOES =============

/**
 * Inicializa o calculador solar com as coordenadas configuradas
 */
void initSolarCalc();

/**
 * Verifica se ha configuracao WiFi guardada no NVS
 * @return true se existe configuracao, false caso contrario
 */
bool hasStoredConfig();

/**
 * Carrega configuracao do NVS para as variaveis globais
 */
void loadConfig();

/**
 * Inicia o Captive Portal (Access Point + Web Server + DNS)
 * @param display Ponteiro para o display (para mostrar feedback visual)
 */
void startCaptivePortal(void* display);

/**
 * Configura o web server com as rotas do captive portal
 */
void setupCaptiveWebServer();

/**
 * Handler para redirecionar pedidos para a pagina principal
 */
void handleCaptiveRedirect(AsyncWebServerRequest *request);

/**
 * Gera o HTML da pagina de configuracao
 * @return String com o HTML completo
 */
String getConfigPage();

/**
 * Gera o HTML da pagina de sucesso
 * @return String com o HTML completo
 */
String getSuccessPage();

/**
 * Processa o captive portal no loop (DNS requests + timeout)
 * @return true se ainda em modo config, false se timeout
 */
bool processCaptivePortal();

#endif // CAPTIVE_PORTAL_H
