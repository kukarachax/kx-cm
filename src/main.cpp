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
bool pair_enable = false;
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

  newBright = data.bright;
  newPowerState = data.powerState; 
  newPowerType = data.powerType;
}

void buttonCheckOnStart() {
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
  buttonCheckOnStart();
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


const uint DEBOUNCE_DELAY = 50;  // Защита от дребезга контактов
const uint CLICK_DELAY = 250;     // Время ожидания второго клика
const uint HOLD_DELAY = 800;      // Время до фиксации удержания
uint32_t pairflag_tmr; 
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
uint32_t lastDebounceTime = 0;
uint32_t buttonPressedTime = 0;
bool isWaitingForClick = false;
bool isHolding = false;
int clickCount = 0;
void checkButton() {
  // Чтение текущего физического состояния пина
  bool reading = digitalRead(BUTTON_PIN);
  uint32_t currentTime = millis();

  // 1. Фильтрация дребезга контактов
  if (reading != lastButtonState) 
    lastDebounceTime = currentTime;

  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Если состояние стабилизировалось
    if (reading != currentButtonState) {
      currentButtonState = reading;

      // Кнопка нажата
      if (currentButtonState == LOW) {
        buttonPressedTime = currentTime;
        isHolding = false;
        clickCount++;
        isWaitingForClick = true;
      } 
      
      else {
        if (isHolding) {
          isHolding = false;
          clickCount = 0; 
          isWaitingForClick = false;
        }
      }
    }
  }

  // Логика удержания (Hold)
  if (currentButtonState == LOW && !isHolding) {
    if ((currentTime - buttonPressedTime) > HOLD_DELAY) {
      isHolding = true;
      pair_enable = true;
      pairflag_tmr = millis();
    }
  }

  // Логика одиночного и двойного клика
  if (isWaitingForClick && currentButtonState == HIGH) {
    if ((currentTime - buttonPressedTime) > CLICK_DELAY) {
      if (clickCount == 1) {  // Вызов события одиночного клика
        newPowerState = !newPowerState; 
      } 
      else if (clickCount == 2) { // Вызов события двойного клика
        if (data.mode >= TOTAL_MODES) 
          data.mode = 0;  
        else 
          data.mode++;          
      }
      settings.update();
      clickCount = 0;
      isWaitingForClick = false;
    }
  }
  lastButtonState = reading;
}

void handlePairing() {
  if (millis() - pairflag_tmr > 60000 && pair_enable) 
    pair_enable = false;
  
}


void loop() { 
  computeSound();
  animation();

  handlePairing();
  checkButton();

  handlePowerType();
  handlePowerState();
  handleBright();
}