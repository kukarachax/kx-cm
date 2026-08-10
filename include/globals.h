#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <FileData.h>

// --- НАСТРОЙКИ ---
#define TOTAL_MODES 13

#define ENABLE_BUTTON
#define TOTAL_LEDS 86
#define LED_PIN 22
#define SOUND_R_1 35
#define SOUND_R_2 34
#define IDICATE_PIN 2
#define BUTTON_PIN 12
#define IR_PIN 4

#define LED_CHIP WS2812
#define LED_COLOR GRB

#define FFT_SIZE 64
#define ADC_BITS 4095
#define EEPROM_SIZE 512

#define OTA_NAME    "Colormusic-ESP32"
#define AP_NAME     "Colormusic"
#define AP_PASSWORD ""

#define UDP_PORT_LISTEN 5395
#define UDP_PORT_SEND 5396
#define MAX_UDP_PACKET_SIZE 255
#define WIFI_CONNECTION_TRYS 20

#if TOTAL_LEDS % 2 
  const uint __numleds = TOTAL_LEDS + 1;
  #define NUM_LEDS __numleds
#else 
  #define NUM_LEDS TOTAL_LEDS
#endif

#define SMOOTH_STEP 20  
#define LIGHT_SMOOTH 2

extern const uint8_t soundRList[2];
extern const char NETWORK_KEY[];
extern const char PAIR_KEY[];

// --- СТРУКТУРЫ И ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
struct Settings {
  String WIFI_SSID, WIFI_PASS;
  uint8_t portAux = 0;  //номер порта для чтения звука

  bool powerState = true;               // Вкл/Выкл | false - Off; true - On
  bool powerType = true;                // Максимальный ток | false - powerbank; true - power adapter
  bool wifi_mode = false;               // false - Запуск в режиме STA; true - в режиме AP
  uint8_t mode = 1;                     // Режимы
  uint8_t emptyBright = 25;             // Яркость пустая
  uint8_t bright = 150;                 // Яркость
  uint8_t emptyColor = 192;             // Цвет пустой
  float maxCurrent = 7.8;               // Макс ток от сети
  float maxCurrentPowerbank = 2.8;      // Макс ток от повербанка
  float EXP = 1.5;                      // expon

  float volumeSmooth = 0.3;             // Плавность анимации
  uint8_t volumePalette = 0;            // Палитра

  float SmoothFreq = 0.8;           // Плавность частот
  float MaxCoefFreq = 1.5;          // Порог вспышки частот

  uint runningFreqSpeed = 1;            // Скорость бегущих частот
  uint8_t freqColors[3] = {0, 64, 96}; // Цвета [Низкие, Средние, Высокие] частот
  uint strobe1FreqSpeed = 70;           // Частота вспышек (1)
  uint strobe5FreqSpeed = 70;           // Частота вспышек (5)

  uint8_t hueStart = 0;                // Цвет спектр
  uint8_t hueStep = 5;                 // Шаг цвета спектр

  uint8_t staticColorHue = 255;        // Цвет статичный цвет
  uint8_t staticColorSat = 255;        // Насыщенность статичный цвет

  uint runningRainbowSpeed = 1;      // Скорость бегущей радуги
  float runningRainbowStep = 1;     // Шаг бегущей радуги

  uint fadeRainbowSpeed = 1;         // Скорость смены цвета (радуга)

  uint8_t fireHueGap = 50;             // Заброс по hue
  uint8_t fireStep = 15;               // Шаг огня
  uint8_t fireHueStart = 0;            // Начальный цвет огня (0 красный, 80 зелёный, 140 молния, 190 розовый)
  uint8_t fireMinBrightness = 200;     // Мин. яркость огня
  uint8_t fireMinSaturation = 150;     // Мин. насыщенность
  uint8_t fireMaxSaturation = 255;     // Макс. насыщенность

  uint runningRusFlagSpeed = 10;     // Скорость флага РФ

  uint8_t pulseColorPulseHue = 255;    // Цвет пульсации
  uint8_t pulseColorPulseSat = 255;    // Насыщенность пульсации

  uint strobeWhiteSpeed = 1000;      // Частота стробоскопа

  uint strobeRGBSpeed = 1000;        // Частота вспышек цветами

  uint partyModeSideSpeed = 900;     // Частота вспышек крайних прожекторов [WHITE, RED, WHITE, BLUE, WHITE]

  int addNoiseLPVolume = 0;  // Добавочная величина к порогу шума Volume
  int addNoiseLPSpectr = 0;  // Добавочная величина к порогу шума Spectr
};

extern Settings data;
extern FileData settings;
extern CRGB leds[NUM_LEDS];
extern char uid[13];
extern bool wifi_saved;
extern bool pair_enable;
extern uint8_t newBright;
extern bool newPowerState;
extern bool newPowerType; 

// --- ОБЪЯВЛЕНИЯ ФУНКЦИЙ ---
// Из audio_leds.cpp
void fill_leds(const int from, const int to, CRGB color, const bool clr = true);
void updateLowPass();
void computeSound();
void animation();
void handlePowerType();
void handlePowerState();
void handleBright();

// Из network.cpp
void networkInit();
void Core0Handler(void *pvParameters);