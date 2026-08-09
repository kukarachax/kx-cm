#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <FileData.h>

// --- НАСТРОЙКИ ---
#define ENABLE_BUTTON
#define TOTAL_LEDS 86
#define LED_PIN 22
#define SOUND_R_1 35
#define SOUND_R_2 34
#define IDICATE_PIN 23
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
  uint8_t portAux = 0;
  bool powerState = true;               
  bool powerType = true;                
  bool wifi_mode = false;               
  uint8_t mode = 1;                     
  uint8_t emptyBright = 25;             
  uint8_t bright = 150;                 
  uint8_t emptyColor = 192;             
  float maxCurrent = 7.8;               
  float maxCurrentPowerbank = 2.8;      
  float EXP = 1.5;                      
  float volumeSmooth = 0.3;             
  uint8_t volumePalette = 0;            
  float SmoothFreq = 0.8;           
  float MaxCoefFreq = 1.5;          
  uint runningFreqSpeed = 1;            
  uint8_t freqColors[3] = {0, 64, 96}; 
  uint strobe1FreqSpeed = 70;           
  uint strobe5FreqSpeed = 70;           
  uint8_t hueStart = 0;                
  uint8_t hueStep = 5;                 
  uint8_t staticColorHue = 255;        
  uint8_t staticColorSat = 255;        
  uint runningRainbowSpeed = 1;      
  float runningRainbowStep = 1;     
  uint fadeRainbowSpeed = 1;         
  uint8_t fireHueGap = 50;             
  uint8_t fireStep = 15;               
  uint8_t fireHueStart = 0;            
  uint8_t fireMinBrightness = 200;     
  uint8_t fireMinSaturation = 150;     
  uint8_t fireMaxSaturation = 255;     
  uint runningRusFlagSpeed = 10;     
  uint8_t pulseColorPulseHue = 255;    
  uint8_t pulseColorPulseSat = 255;    
  uint strobeWhiteSpeed = 1000;      
  uint strobeRGBSpeed = 1000;        
  uint partyModeSideSpeed = 900;     
  int addNoiseLPVolume = 0;  
  int addNoiseLPSpectr = 0;  
};

extern Settings data;
extern FileData settings;
extern CRGB leds[NUM_LEDS];
extern char uid[13];
extern bool wifi_saved;
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