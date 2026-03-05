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
// POWER MANAGEMENT - Brightness Cap
// ============================================================
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - PSU integrada de 25W
  // Painel consome 28W @ 100%, ESP32-S3 consome ~1-2W
  // Cap a 80% para garantir consumo < 25W
  //
  // Cálculo:
  //   - Disponível para painel: 25W - 2W = 23W
  //   - Cap: 23W / 28W = 82% → usar 80% (204/255)
  //
  #ifndef MAX_BRIGHTNESS_CAP
    #define MAX_BRIGHTNESS_CAP 204  // 80% de 255 (~23W)
  #endif
  #define PANEL_MAX_WATTS     28
  #define PSU_WATTS           25
  #define ESP32_RESERVE_WATTS 2
#else
  // ESP32 Dev Module - Sem limite (PSU externa adequada)
  #ifndef MAX_BRIGHTNESS_CAP
    #define MAX_BRIGHTNESS_CAP 255  // 100%
  #endif
#endif

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
// CLICK WHEEL (Encoder Quadratura + Botão Central)
// ============================================================
// H1: Encoder quadratura EC11/EC12 (2 canais A+B, 90° desfasados)
// H2: Hall Effect + anel magnético (mesmo pinout, lógica idêntica)
// H3: Encoder óptico IR (mesmo pinout, lógica idêntica)
//
// Rotação CW  → brilho aumenta
// Rotação CCW → brilho diminui
// Click central → cicla modo: AUTO_SOLAR → THERAPY_RED → OFF
//
#if BOARD_MATRIXPORTAL_S3
  // Matrix Portal S3 - GPIOs livres no conector de expansão
  #define ENCODER_A_PIN    9   // Canal A do encoder (interrupt)
  #define ENCODER_B_PIN   10   // Canal B do encoder (interrupt)
  #define CENTER_BTN_PIN  11   // Botão central click wheel (interrupt)
#else
  // ESP32 Dev Module - ADC1 input-only, todos suportam interrupts
  #define ENCODER_A_PIN   32   // Canal A (ADC1_CH4, interrupt)
  #define ENCODER_B_PIN   35   // Canal B (input-only, interrupt ok)
  #define CENTER_BTN_PIN  34   // Botão central (input-only, interrupt ok)
#endif

// Parâmetros da click wheel
#define ENCODER_TICKS_PER_CLICK   1    // 1 tick = 1 detent do encoder
#define BRIGHTNESS_STEP           10   // Variação de brilho por tick (0-255)
#define BRIGHTNESS_MIN            10   // Mínimo para painel não apagar totalmente
#define CENTER_DEBOUNCE_MS        200  // Debounce do botão central (ms)

// Sensor óptico IR (H3): comportamento do sinal
//   O fototransístor com pull-up produz:
//     LOW  quando dente reflecte a luz (fototransístor conduz)
//     HIGH quando gap absorve a luz   (fototransístor ao corte)
//   Se o disco tiver as cores invertidas (dente escuro, gap reflectivo),
//   ou se os sensores estiverem trocados, a direcção fica invertida.
//   Basta mudar este define de 0 para 1 — sem re-soldar nada.
#define ENCODER_INVERT_DIR        0    // 1 = inverter CW/CCW (ex: disco montado pelo lado oposto)

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
