#include "globals.h"

// --- ОПРЕДЕЛЕНИЕ ГЛОБАЛЬНЫХ ПЕРЕМЕННЫХ ИЗ GLOBALS.H ---
const uint8_t soundRList[2] = {SOUND_R_1, SOUND_R_2}; 
const char NETWORK_KEY[] = "KXCM";
const char PAIR_KEY[] = "KXPD";

Settings data;
FileData settings(&LittleFS, "/settings.dat", 'B', &data, sizeof(data), 1500);
CRGB leds[NUM_LEDS];
TaskHandle_t Core0TaskHandle;
char uid[13];
bool wifi_saved = false;
uint8_t newBright = data.bright;
bool newPowerState = data.powerState;
bool newPowerType = data.powerType;
// ------------------------------------------------------

void ledStripeInit() {
  FastLED.addLeds<LED_CHIP, LED_PIN, LED_COLOR>(leds, NUM_LEDS);

  if (!data.powerType) {
    FastLED.setMaxPowerInVoltsAndMilliamps(5, data.maxCurrentPowerbank * 1000);
    data.mode = 6;
    data.powerState = true;
    data.bright = constrain(data.bright, 0, 150);
  }
  else FastLED.setMaxPowerInVoltsAndMilliamps(5, data.maxCurrent * 1000);
  FastLED.setBrightness(data.bright);
}

void LittleFSBegin() {
  if (!LittleFS.begin(true)) {
    Serial.println("Ошибка монтирования LittleFS!");
  }
  
  switch (settings.read()) {
    case FD_FS_ERR: Serial.println("FS Error"); break;
    case FD_FILE_ERR: Serial.println("Error"); break;
    case FD_WRITE: Serial.println("Data Write"); break;
    case FD_ADD: Serial.println("Data Add"); break;
    case FD_READ: Serial.println("Data Read"); break;
    case FD_NO_DIF: Serial.println("FD_NO_DIF"); break;
  }
}

void buttonCheck() {
  if (digitalRead(BUTTON_PIN)) return;
  data.powerType = false;
  settings.updateNow();
}



void setup() {
  Serial.begin(115200);
  esp_log_level_set("WiFiUdp", ESP_LOG_NONE); 

  pinMode(SOUND_R_1, INPUT);
  pinMode(SOUND_R_2, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(IDICATE_PIN, OUTPUT);
  digitalWrite(IDICATE_PIN, LOW);

  LittleFSBegin();

  #ifdef ENABLE_BUTTON 
  buttonCheck();
  #endif

  xTaskCreatePinnedToCore(
    Core0Handler,         /* Функция задачи */
    "Network_Task", /* Имя задачи */
    10000,             /* Размер стека */
    NULL,              /* Параметры */
    1,                 /* Приоритет */
    &Core0TaskHandle,  /* Хэндл задачи */
    0);

  ledStripeInit();
  networkInit();
  updateLowPass();

  Serial.println("Setup end");
}

void loop() { 
  computeSound();
  animation();

  handlePowerType();
  handlePowerState();
  handleBright();
}