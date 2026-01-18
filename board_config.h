/**
 * ============================================================
 * BOARD CONFIGURATION - Compilação Condicional
 * ============================================================
 *
 * Define a placa alvo antes de incluir este ficheiro:
 *
 *   #define BOARD_MATRIXPORTAL_S3 1   // Adafruit Matrix Portal S3
 *   #define BOARD_MATRIXPORTAL_S3 0   // ESP32 Dev Module genérico
 *
 * Ou define via platformio.ini / Arduino IDE:
 *   -DBOARD_MATRIXPORTAL_S3=1
 *
 * ============================================================
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================
// SELEÇÃO DE PLACA (alterar aqui ou via build flags)
// ============================================================
#ifndef BOARD_MATRIXPORTAL_S3
  #define BOARD_MATRIXPORTAL_S3 1  // 0 = ESP32 Dev Module, 1 = Matrix Portal S3
#endif

// ============================================================
// VALIDAÇÃO E INFO
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  #define BOARD_NAME "Adafruit Matrix Portal S3"
  #define BOARD_MCU  "ESP32-S3"
#else
  #define BOARD_NAME "ESP32 Dev Module"
  #define BOARD_MCU  "ESP32"
#endif

// ============================================================
// PINOUT HUB75 (Display LED Matrix)
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - Pinos fixos na PCB
  // Ref: https://learn.adafruit.com/adafruit-matrixportal-s3/pinouts
  #define R1_PIN  42
  #define G1_PIN  41
  #define B1_PIN  40
  #define R2_PIN  38
  #define G2_PIN  39
  #define B2_PIN  37
  #define A_PIN   45
  #define B_PIN   36
  #define C_PIN   48
  #define D_PIN   35
  #define E_PIN   21   // Para matrizes 64x64 (jumper na PCB)
  #define LAT_PIN 47
  #define OE_PIN  14
  #define CLK_PIN 2
#else
  // ESP32 Dev Module - Pinout original do projeto
  #define R1_PIN  25
  #define G1_PIN  26
  #define B1_PIN  27
  #define R2_PIN  14
  #define G2_PIN  12
  #define B2_PIN  13
  #define A_PIN   23
  #define B_PIN   19
  #define C_PIN   5
  #define D_PIN   17
  #define E_PIN   -1   // Não usado em painéis 32x16
  #define LAT_PIN 4
  #define OE_PIN  15
  #define CLK_PIN 16
#endif

// ============================================================
// PINOUT I2C (RTC DS3231 e sensores)
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - STEMMA QT connector
  // Ref: pins_arduino.h do ESP32-S3
  // NOTA: O acelerómetro LIS3DH integrado usa endereço 0x19
  #define I2C_SDA_PIN 16
  #define I2C_SCL_PIN 17

  // Endereço do acelerómetro integrado (reservado)
  #define LIS3DH_I2C_ADDR 0x19
#else
  // ESP32 Dev Module - Pinos I2C padrão
  #define I2C_SDA_PIN 21
  #define I2C_SCL_PIN 22
#endif

// ============================================================
// BOTÕES
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - 2 botões dedicados (sem pull-up interno!)
  #define BUTTON_PIN      6   // Botão UP (principal para mudar modo)
  #define BUTTON_UP_PIN   6   // Alias
  #define BUTTON_DOWN_PIN 7   // Botão DOWN (funcionalidade extra)
  #define BUTTON_COUNT    2
  #define BUTTON_ACTIVE_LOW true
  #define BUTTON_NEEDS_PULLUP true  // Sem resistência de pull-up na PCB
#else
  // ESP32 Dev Module - Botão BOOT
  #define BUTTON_PIN      0   // GPIO0 (BOOT button)
  #define BUTTON_COUNT    1
  #define BUTTON_ACTIVE_LOW true
  #define BUTTON_NEEDS_PULLUP true
#endif

// ============================================================
// NEOPIXEL / LED DE STATUS
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - NeoPixel RGB integrado
  #define HAS_NEOPIXEL      true
  #define NEOPIXEL_PIN      4
  #define NEOPIXEL_COUNT    1
  #define NEOPIXEL_TYPE     (NEO_GRB + NEO_KHZ800)  // WS2812B

  // LED D13 também disponível
  #define LED_BUILTIN_PIN   13
  #define HAS_LED_BUILTIN   true
#else
  // ESP32 Dev Module - Apenas LED azul (se disponível)
  #define HAS_NEOPIXEL      false
  #define NEOPIXEL_PIN      -1

  // LED integrado (varia por modelo de placa)
  #define LED_BUILTIN_PIN   2   // Comum em ESP32 DevKit
  #define HAS_LED_BUILTIN   true
#endif

// ============================================================
// SENSOR DE LUZ AMBIENTE
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - Sensor de luz analógico
  #define HAS_LIGHT_SENSOR    true
  #define LIGHT_SENSOR_PIN    5   // A5
#else
  #define HAS_LIGHT_SENSOR    false
  #define LIGHT_SENSOR_PIN    -1
#endif

// ============================================================
// ACELERÓMETRO (LIS3DH)
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - LIS3DH integrado no barramento I2C
  #define HAS_ACCELEROMETER   true
  #define ACCEL_I2C_ADDR      0x19  // Endereço não-padrão!
#else
  #define HAS_ACCELEROMETER   false
#endif

// ============================================================
// CONFIGURAÇÕES DE DISPLAY
// ============================================================
// Estas são comuns mas podem ser ajustadas por placa
#define DISPLAY_WIDTH   32
#define DISPLAY_HEIGHT  16
#define DISPLAY_CHAIN   1   // Número de painéis em cadeia

// Tipo de painel (P10 quarter-scan)
#define DISPLAY_DRIVER  HUB75_I2S_CFG::SHIFTREG

// ============================================================
// CONFIGURAÇÕES DE WIFI
// ============================================================
// O ESP32-S3 suporta WiFi 802.11 b/g/n (2.4 GHz)
// Ambas as placas têm WiFi integrado, sem diferenças de config

// ============================================================
// DEBUG / SERIAL
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - Debug via USB-C nativo ou pinos TX/RX
  #define DEBUG_TX_PIN  18
  #define DEBUG_RX_PIN  8
#else
  // ESP32 Dev Module - Debug via USB-UART ou pinos padrão
  #define DEBUG_TX_PIN  1
  #define DEBUG_RX_PIN  3
#endif

// ============================================================
// MACROS ÚTEIS
// ============================================================

// Verifica se um pino está definido e válido
#define PIN_VALID(pin) ((pin) >= 0)

// Inicialização condicional do I2C
#define INIT_I2C() Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN)

// Inicialização condicional dos botões
#if BUTTON_NEEDS_PULLUP
  #define INIT_BUTTON(pin) pinMode(pin, INPUT_PULLUP)
#else
  #define INIT_BUTTON(pin) pinMode(pin, INPUT)
#endif

// ============================================================
// INFORMAÇÃO DE COMPILAÇÃO
// ============================================================
#define BOARD_INFO_STRING BOARD_NAME " (" BOARD_MCU ")"

#endif // BOARD_CONFIG_H
