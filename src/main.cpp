/* for IDE https://dl.espressif.com/dl/package_esp32_index.json
esp arduino ide 2.0.11
FastLED 3.4.0

popa
*/
#include <Arduino.h>

#define ENABLE_BUTTON

#define TOTAL_LEDS 181
#define LED_PIN 22                // stripe 
#define SOUND_R_1 35              // pin 1 sound
#define SOUND_R_2 34              // pin 2 sound
#define IDICATE_PIN 23
#define BUTTON_PIN 12

#define LED_CHIP WS2812
#define LED_COLOR GRB

#define FFT_SIZE 64     // размер выборки x2
#define ADC_BITS 4095
#define EEPROM_SIZE 512

#define OTA_NAME    "Colormusic-ESP32"
#define AP_NAME     "Colormusic"
#define AP_PASSWORD ""

#define UDP_PORT_LISTEN 5395  //Receive port (device port in android app)
#define UDP_PORT_SEND 5396    //For debugging (local port in andriod app)
#define MAX_UDP_PACKET_SIZE 255
#define WIFI_CONNECTION_TRYS 20
const char NETWORK_KEY[] = "";
char uid[13];


#if TOTAL_LEDS % 2 
  const uint __numleds = TOTAL_LEDS + 1;
  #define NUM_LEDS __numleds
#else 
  #define NUM_LEDS TOTAL_LEDS
#endif

#include <FastLED.h>
#include <FFT_C.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <SPIFFS.h>
#include <FileData.h>

// НАСТРОЙКИ ПО УМОЛЧАНИЮ
#define SMOOTH_STEP 20  
#define LIGHT_SMOOTH 2

const uint8_t soundRList[2] = {SOUND_R_1, SOUND_R_2}; //массив портов

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
// НАСТРОЙКИ ПО УМОЛЧАНИЮ


int spectr[FFT_SIZE];
float averK = 0.006;
int lowPassSpectr = 40;
int lowPassVolume = 100;
bool colorMusicFlash[3], running_flag[3];
float freq_max_f, colorMusic_f[3], colorMusic_aver[3];
uint8_t thisBright[3];
int freq_min[32];
float averageLevel = 50;
int maxLevel = 100;
float RsoundLevel_f, LsoundLevel_f;
int Rlenght, Llenght, RcurrentLevel, LcurrentLevel;
bool wifi_saved = false; //флаг корректности конфигурации wifi
const uint16_t HALF_LED = NUM_LEDS / 2;
DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0,    0,    255,  0,  // green
  100,  255,  255,  0,  // yellow
  150,  255,  100,  0,  // orange
  200,  255,  50,   0,  // red
  255,  255,  0,    0   // red
};

CRGBPalette32 redtogreen_p = soundlevel_gp;
IPAddress broadcastIP, this_ip;
AsyncWebServer serverAP(80);
DNSServer dnsServer;
WiFiUDP udp;
CRGB leds[NUM_LEDS];
Settings data;
FileData settings(&SPIFFS, "/settings.dat", 'B', &data, sizeof(data), 1500);

void fill_leds(const int from, const int to, CRGB color, const bool clr = true) {
  if (clr) 
    FastLED.clear();

  for (int i = from; i < to; i++) 
    leds[i] = color;
  
  FastLED.show();
}

void getFFT() {
  int raw[FFT_SIZE];

  for (int i = 0; i < FFT_SIZE; i++)
    raw[i] = analogRead(soundRList[data.portAux]);
  
  FFT(raw, spectr);
} 

void updateLowPass() {
  int maxNoiseLevel = 0;                          
  int noiseLevel = 0;

  fill_leds(0, 5, CRGB::Pink);

  for (int i = 0; i < 500; i++) {
    noiseLevel = analogRead(soundRList[data.portAux]);        
    if (noiseLevel > maxNoiseLevel) 
      maxNoiseLevel = noiseLevel;                                             
  }
  lowPassVolume = maxNoiseLevel + data.addNoiseLPVolume;

  maxNoiseLevel = 0;
  for (int i = 0; i < 500; i++) {
    getFFT();

    for (uint8_t j = 3; j < 28; j++) {
      if (spectr[j] > maxNoiseLevel) 
        maxNoiseLevel = noiseLevel;
    }
  }
  lowPassSpectr = maxNoiseLevel + data.addNoiseLPSpectr;
  
  //debugUDP("LowPass Updated");
}

void filterFFT() {
  int spectrF[FFT_SIZE];
  int colorMusic[3] = {0,0,0};

  for (int i = 0; i < FFT_SIZE; i++)  //Нижний порог звука (подавление шума)
    if (spectr[i] < lowPassSpectr) 
      spectr[i] = 0;
    
  for (uint8_t i = 2; i < 6; i++) //выборка нижних частот
    if (spectr[i] > colorMusic[0]) 
      colorMusic[0] = spectr[i];
  
  for (uint8_t i = 6; i < 11; i++) //выборка средних частот
    if (spectr[i] > colorMusic[1])  
      colorMusic[1] = spectr[i];
    
  for (uint8_t i = 11; i < 29; i++) //выборка высоких частот
    if (spectr[i] > colorMusic[2]) 
      colorMusic[2] = spectr[i];
    
  int freq_max = 0;
  
  for (uint8_t i = 0; i < 31; i++) { 
    if (spectr[i + 1] > freq_max) freq_max = spectr[i + 1]; //максимальное значение среди всех частот
    if (freq_max < 5) freq_max = 5;
    
    if (freq_min[i] < spectr[i + 1]) freq_min[i] = spectr[i + 1]; //минимальное значение среди всех частот
    if (freq_min[i] > 0) freq_min[i] -= LIGHT_SMOOTH; //плавность вспышки
    else freq_min[i] = 0;
  }

  freq_max_f = freq_max * averK + freq_max_f * (1 - averK); 

  for (uint8_t i = 0; i < 3; i++) { //цикл на фильтрацию и проверку 3-х частот
    colorMusic_aver[i] = colorMusic[i] * averK + colorMusic_aver[i] * (1 - averK);
    colorMusic_f[i] = colorMusic[i] * data.SmoothFreq + colorMusic_f[i] * (1 - data.SmoothFreq);
    
    if (colorMusic_f[i] > ((float)colorMusic_aver[i] * data.MaxCoefFreq)) {
      thisBright[i] = 255; //яркость вспышки
      colorMusicFlash[i] = true; //разрешение на вспышку 1
      running_flag[i] = true; //разрешение на вспышку частоты 2 
    } 
    else colorMusicFlash[i] = false; //запрет на вспышку 1

    if (thisBright[i] >= 0) thisBright[i] -= SMOOTH_STEP; //если яркость вспышки больше 0 то применяется плавность вспышки

    if (thisBright[i] < data.emptyBright) { //emptyBright - яркость "подложки" (когда нет звука)
      thisBright[i] = data.emptyBright; //яркость вспышки не может быть меньше чем emptyBright
      running_flag[i] = false; //запрет на вспышку частоты 2
    }
  }
}

void filterVolume() {
  #define MAX_COEF 1.8
  int HALF_LED = NUM_LEDS / 2;
  float RsoundLevel = 0, LsoundLevel = 0;

  for (uint8_t i = 0; i < 100; i ++) { //100 измерений для большей точности                     
    RcurrentLevel = analogRead(soundRList[data.portAux]);
    if (RsoundLevel < RcurrentLevel) //ищем максимум
      RsoundLevel = RcurrentLevel;
  }

  RsoundLevel = map(RsoundLevel, lowPassVolume, ADC_BITS, 0, 500); //ограничение по диапазону 0 - 500
  RsoundLevel = constrain(RsoundLevel, 0, 500); //предотвращение выхода за диапазон
  RsoundLevel = pow(RsoundLevel, data.EXP); //возведение в степень 
  RsoundLevel_f = RsoundLevel * data.volumeSmooth + RsoundLevel_f * (1 - data.volumeSmooth);
  LsoundLevel_f = RsoundLevel_f;

  if (data.emptyBright > 5) {
    for (int i = 0; i < NUM_LEDS; i++)
      leds[i] = CHSV(data.emptyColor, 255, data.emptyBright); //тут заливка подложки светодиодов
  }

  if (RsoundLevel_f > 15 && LsoundLevel_f > 15) {
    averageLevel = (float)(RsoundLevel_f + LsoundLevel_f) / 2 * averK + averageLevel * (1 - averK);
    maxLevel = (float)averageLevel * MAX_COEF;
    Rlenght = map(RsoundLevel_f, 0, maxLevel, 0, HALF_LED);
    Llenght = map(LsoundLevel_f, 0, maxLevel, 0, HALF_LED);

    Rlenght = constrain(Rlenght, 0, HALF_LED);
    Llenght = constrain(Llenght, 0, HALF_LED);
  }

  #undef MAX_COEF
}

void computeSound() {
  if (data.mode == 0) filterVolume();
  else if (data.mode < 6) {
    getFFT();
    filterFFT();
  }
}



//--------------------------VOLUME------------------------------
void volumeAnimate() {
  const float index = (float)255 / HALF_LED;
  uint8_t counter = 0;

  if (data.emptyBright > 0) {
    for (int i = 0; i < NUM_LEDS; i++)
      leds[i] = CHSV(data.emptyColor, 255, data.emptyBright);
  }

  for (int i = HALF_LED - 1; i > HALF_LED - 1 - Rlenght; i--) {
    switch (data.volumePalette) {
      case 0: leds[i] = ColorFromPalette(redtogreen_p, (counter * index)); break;
      case 1: leds[i] = ColorFromPalette(RainbowColors_p, (counter * index)); break;
      case 2: leds[i] = ColorFromPalette(ForestColors_p, (counter * index)); break;
      case 3: leds[i] = ColorFromPalette(CloudColors_p, (counter * index)); break;
      case 4: leds[i] = ColorFromPalette(LavaColors_p, (counter * index)); break;
      case 5: leds[i] = ColorFromPalette(OceanColors_p, (counter * index)); break;
      default: break;
    }
    counter++;
  }
  counter = 0;

  for (int i = HALF_LED; i < HALF_LED + Llenght; i++) {
    switch (data.volumePalette) {
      case 0: leds[i] = ColorFromPalette(redtogreen_p, (counter * index)); break;
      case 1: leds[i] = ColorFromPalette(RainbowColors_p, (counter * index)); break;
      case 2: leds[i] = ColorFromPalette(ForestColors_p, (counter * index)); break;
      case 3: leds[i] = ColorFromPalette(CloudColors_p, (counter * index)); break;
      case 4: leds[i] = ColorFromPalette(LavaColors_p, (counter * index)); break;
      case 5: leds[i] = ColorFromPalette(OceanColors_p, (counter * index)); break;
      default: break;
    }
    counter++;
  }
  counter = 0;
}


//--------------------------RUNNING_FREQ------------------------
uint32_t runningFreq_timer;
void runningFreqAnimate() {
  if (running_flag[2]) leds[HALF_LED] = CHSV(data.freqColors[2], 255, thisBright[2]);
  else if (running_flag[1]) leds[HALF_LED] = CHSV(data.freqColors[1], 255, thisBright[1]);
  else if (running_flag[0]) leds[HALF_LED] = CHSV(data.freqColors[0], 255, thisBright[0]);
  else leds[HALF_LED] = CHSV(data.emptyColor, 255, data.emptyBright);

  leds[HALF_LED - 1] = leds[HALF_LED];

  if (micros() - runningFreq_timer > data.runningFreqSpeed) {
    runningFreq_timer = micros();
    
    for (int i = 0; i < HALF_LED - 1; i++) {
      leds[i] = leds[i + 1];
      leds[NUM_LEDS - i - 1] = leds[i];
    }
  } 
}


//--------------------------1_FREQ_STROBE-----------------------
uint32_t strobe1Freq_timer;
void strobe1FreqAnimate() {
  if (millis() - strobe1Freq_timer > data.strobe1FreqSpeed) {
    strobe1Freq_timer = millis();

    if (colorMusicFlash[2]) fill_solid(leds, NUM_LEDS, CHSV(data.freqColors[2], 255, thisBright[2]));
    else if (colorMusicFlash[1]) fill_solid(leds, NUM_LEDS, CHSV(data.freqColors[1], 255, thisBright[1]));
    else if (colorMusicFlash[0]) fill_solid(leds, NUM_LEDS, CHSV(data.freqColors[0], 255, thisBright[0]));
    else fill_solid(leds, NUM_LEDS, CHSV(data.emptyColor, 255, data.emptyBright));
  }
    
}


//--------------------------5_FREQ_STROBE-----------------------
uint32_t strobe5Freq_timer;
const int freq5Leds = NUM_LEDS / 5;
void strobe5FreqAnimate() {
  if (millis() - strobe5Freq_timer > data.strobe5FreqSpeed) {
    strobe5Freq_timer = millis();

    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < freq5Leds)          leds[i] = CHSV(data.freqColors[2], 255, thisBright[2]);
      else if (i < freq5Leds * 2) leds[i] = CHSV(data.freqColors[1], 255, thisBright[1]);
      else if (i < freq5Leds * 3) leds[i] = CHSV(data.freqColors[0], 255, thisBright[0]);
      else if (i < freq5Leds * 4) leds[i] = CHSV(data.freqColors[1], 255, thisBright[1]);
      else if (i < NUM_LEDS) leds[i] = CHSV(data.freqColors[2], 255, thisBright[2]);
    }
  }
}


//--------------------------SPECTR_STROBE-----------------------
void strobe20FreqAnimate() {
  uint8_t HUEindex = data.hueStart;

  for (int i = 0; i < HALF_LED; i++) {
    const float numLedsFreq = NUM_LEDS / (float)40;
    const int j = floor((HALF_LED - i) / numLedsFreq);
    uint8_t this_bright = map(freq_min[j], 0, freq_max_f, 0, 255);

    leds[i] = CHSV(HUEindex, 255, this_bright);
    leds[NUM_LEDS - i - 1] = leds[i];

    HUEindex += data.hueStep;
    if (HUEindex > 254) HUEindex = 0;
  } 
}


//--------------------------PARTY_MODE--------------------------
const int partyModeLeds = NUM_LEDS / 5;
void partyModeSideFlash() {
  static bool flash1State = false;
  static uint32_t flash1_timer = 0;
  bool isFlash = false;

  if (colorMusicFlash[0] || colorMusicFlash[1] || colorMusicFlash[2]) 
    isFlash = true;
  else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    return;
  }

  if (millis() - flash1_timer > data.partyModeSideSpeed) {
    flash1_timer = millis();

    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < partyModeLeds || i > partyModeLeds * 4 - 1) {
        if (flash1State) 
          leds[i] = CRGB::White;
        else 
          leds[i] = CRGB::Black;
      }
    }
    flash1State = !flash1State;
  }
}

void partyModeAnimate() {
  partyModeSideFlash();

  for (int i = 0; i < partyModeLeds; i++) {
    if (colorMusicFlash[0]) 
      leds[partyModeLeds * 2 + i] = CRGB::Purple;
    else 
      leds[partyModeLeds * 2 + i] = CRGB::Black;

    if (colorMusicFlash[1]) 
      leds[partyModeLeds + i] = CRGB::Red;
    else 
      leds[partyModeLeds + i] = CRGB::Black;

    if (colorMusicFlash[2]) 
      leds[partyModeLeds * 3 + i] = CRGB::Blue;
    else 
      leds[partyModeLeds * 3 + i] = CRGB::Black;
  }
}


//--------------------------STATIC_COLOR------------------------
void staticColorAnimate() {
  fill_solid(leds, NUM_LEDS, CHSV(data.staticColorHue, data.staticColorSat, data.bright));
}


//--------------------------RUNNING_RAINBOW---------------------
uint32_t runningRainbow_timer;
float runningRainbow_steps;
int runningRainbow_color;

void runningRainbowAnimate() {
  if (millis() - runningRainbow_timer > 30) {
    runningRainbow_timer = millis();

    runningRainbow_color += data.runningRainbowSpeed;

    if (runningRainbow_color > 255) runningRainbow_color = 0;
    if (runningRainbow_color < 0) runningRainbow_color = 255;
  }

  runningRainbow_steps = runningRainbow_color;

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(floor(runningRainbow_steps), 255, 255);

    runningRainbow_steps += data.runningRainbowStep;

    if (runningRainbow_steps > 255) runningRainbow_steps = 0;
    if (runningRainbow_steps < 0) runningRainbow_steps = 255;
  }
}


//--------------------------RAINBOW_FADE------------------------
uint32_t fadeRainbow_timer;
int rainbowFadeHue;

void fadeRainbowAnimate() {
  if (millis() - fadeRainbow_timer > data.fadeRainbowSpeed) {
    fadeRainbow_timer = millis();

    rainbowFadeHue++;
    if (rainbowFadeHue > 255) rainbowFadeHue = 0;

    fill_solid(leds, NUM_LEDS, CHSV(rainbowFadeHue, 255, data.bright));
  }
}


//--------------------------PERLIN_NOISE------------------------
uint32_t perlinNoise_timer;
int FireCounter = 0;

CHSV getFireColor(int val) {
  return CHSV(data.fireHueStart + map(val, 0, 255, 0, data.fireHueGap), constrain(map(val, 0, 255, data.fireMaxSaturation, data.fireMinSaturation), 0, 255), constrain(map(val, 0, 255, data.fireMinBrightness, 255), 0, 255));
}

void perlinNoiseAnimate() {
  #define FOR_i(from, to) for(int i = (from); i < (to); i++)

  if (millis() - perlinNoise_timer > 20) {
    perlinNoise_timer = millis();

    int thisPos = 0, lastPos = 0;

    FOR_i(0, NUM_LEDS) {
      leds[i] = getFireColor((inoise8(i * data.fireStep, FireCounter)));
    }
    
    FireCounter += 20;
  }
}


//--------------------------RUS_FLAG----------------------------
int runningRusFlag_idex = 0;
uint32_t runningRusFlag_timer;

int shift_index(int current_index, int delta, int leds_count) {
  return (current_index + delta) % leds_count;
}

void runningRusFlagAnimate() { 
  if (millis() - runningRusFlag_timer > data.runningRusFlagSpeed) {
    runningRusFlag_timer = millis();
    if (runningRusFlag_idex++ >= NUM_LEDS) runningRusFlag_idex = 0;

    int delta = NUM_LEDS / 3;
    int whiteIndex = runningRusFlag_idex;
    int blueIndex = shift_index(whiteIndex, delta, NUM_LEDS);
    int redIndex = shift_index(blueIndex, delta, NUM_LEDS);

    leds[whiteIndex].setRGB(255, 255, 255);
    leds[blueIndex].setRGB(0, 0, 255);
    leds[redIndex].setRGB(255, 0, 0);
  }
}


//--------------------------RGB_STROBE--------------------------
uint32_t strobeRGB_timer = 0;
uint8_t strobeRGB_counter = 0;

void strobeRGBAnimate() {
  if (millis() - strobeRGB_timer > data.strobeRGBSpeed) {
    strobeRGB_timer = millis();
    switch (strobeRGB_counter) {
      case 0: fill_solid(leds, NUM_LEDS, CRGB::Red); break;
      case 1: fill_solid(leds, NUM_LEDS, CRGB::Green); break;
      case 2: fill_solid(leds, NUM_LEDS, CRGB::Blue); break;
    }
    strobeRGB_counter++;
    if (strobeRGB_counter > 2) strobeRGB_counter = 0;
  }
}


//--------------------------PULSE_COLOR-------------------------
void pulseColorAnimate() {
  static bool direct = false;
  static uint8_t iBright = 0;

  if (direct) {
    iBright++;
    if (iBright >= data.bright) direct = !direct;
  }
  else {
    iBright--;
    if (iBright < 1) direct = !direct;
  }

  for (int i = 0 ; i < NUM_LEDS; i++) {
    leds[i] = CHSV(data.pulseColorPulseHue, data.pulseColorPulseSat, iBright);
  }
}


//--------------------------STROBE_WHITE------------------------
uint32_t strobeWhite_timer;
bool strobeWhite_counter;

void strobeWhiteAnimate() {
  if (millis() - strobeWhite_timer > data.strobeWhiteSpeed) {
    strobeWhite_timer = millis();

    if (strobeWhite_counter) fill_solid(leds, NUM_LEDS, CRGB::White);
    else fill_solid(leds, NUM_LEDS, CRGB::Black);
    strobeWhite_counter = !strobeWhite_counter;
  }
}



void animation() {
  if (data.powerState) {
    switch (data.mode) {
      case 0: volumeAnimate(); break;
      case 1: runningFreqAnimate(); break;
      case 2: strobe1FreqAnimate(); break;
      case 3: strobe5FreqAnimate(); break;
      case 4: strobe20FreqAnimate(); break;
      case 5: partyModeAnimate(); break;
      case 6: staticColorAnimate(); break;
      case 7: runningRainbowAnimate(); break;
      case 8: fadeRainbowAnimate(); break;
      case 9: perlinNoiseAnimate(); break;
      case 10: runningRusFlagAnimate(); break;
      case 11: strobeRGBAnimate(); break;
      case 12: pulseColorAnimate(); break;
      case 13: strobeWhiteAnimate(); break;
    }
  } 
  else {
    FastLED.clear();
    FastLED.setBrightness(0);
  }

  FastLED.show();
}

void updateBright(uint8_t newBr) {
  if (!data.powerType) newBr = constrain(newBr, 0, 100);

  if (newBr < data.bright) {
    for (uint8_t i = data.bright; i > newBr; i-=2) {
      animation();

      if (i - newBr < 2) break;

      FastLED.setBrightness(i);
    }
  }

  else {
    for (uint8_t i = data.bright; i < newBr; i+=2) {
      animation();

      if (newBr - i < 2) break;
    
      FastLED.setBrightness(i);
    }
  }

  FastLED.setBrightness(newBr);
  data.bright = newBr;
}

void updatePowerState(bool newPS) {
  if (newPS) {
    data.powerState = newPS;
    for (uint8_t i = 0; i < data.bright; i++) {
      if (data.bright - i < 2) break;

      FastLED.setBrightness(i);
      animation();
    }
    return;
  }

  for (uint8_t i = data.bright; i > 0 ; i--) {
    if (i < 2) break;

    FastLED.setBrightness(i);
    animation();

    delay(10);
  }
  data.powerState = newPS;
}

void updatePowerType(bool newPT) {
  if (newPT) FastLED.setMaxPowerInVoltsAndMilliamps(5, data.maxCurrent * 1000);
  else FastLED.setMaxPowerInVoltsAndMilliamps(5, data.maxCurrentPowerbank * 1000);
  data.powerType = newPT;
}


//KXCM:aabbccddeeff>1=0.1%
//Отправка настроек в приложение
void sendDataToApp() {
  String dataBuffer = String(NETWORK_KEY);
  #define addStr(t) dataBuffer = dataBuffer + t + ";"

  addStr(data.portAux); 
  addStr(data.powerState); 
  addStr(data.mode); 
  addStr(data.emptyBright); 
  addStr(data.bright); 
  addStr(data.emptyColor); 
  addStr(data.volumePalette); 
  addStr(data.freqColors[0]); 
  addStr(data.freqColors[1]); 
  addStr(data.freqColors[2]); 
  addStr(data.hueStart); 
  addStr(data.hueStep); 
  addStr(data.staticColorHue); 
  addStr(data.staticColorSat); 
  addStr(data.fireHueGap); 
  addStr(data.fireStep); 
  addStr(data.fireHueStart); 
  addStr(data.fireMinBrightness); 
  addStr(data.fireMinSaturation); 
  addStr(data.fireMaxSaturation); 
  addStr(data.pulseColorPulseHue); 
  addStr(data.pulseColorPulseSat); 
  addStr(data.addNoiseLPVolume); 
  addStr(data.addNoiseLPSpectr); 
  addStr(data.maxCurrent); 
  addStr(data.maxCurrentPowerbank); 
  addStr(data.volumeSmooth); 
  addStr(data.SmoothFreq); 
  addStr(data.MaxCoefFreq); 
  addStr(data.runningFreqSpeed); 
  addStr(data.strobe1FreqSpeed); 
  addStr(data.strobe5FreqSpeed); 
  addStr(data.runningRainbowSpeed); 
  addStr(data.runningRainbowStep); 
  addStr(data.fadeRainbowSpeed); 
  addStr(data.runningRusFlagSpeed); 
  addStr(data.strobeWhiteSpeed); 
  addStr(data.strobeRGBSpeed); 
  addStr(data.partyModeSideSpeed);
  addStr(data.addNoiseLPVolume);
  addStr(data.addNoiseLPSpectr);
  addStr(data.powerType);

  if (WiFi.getMode() == WIFI_STA)
    udp.beginPacket(broadcastIP, UDP_PORT_SEND);
  else 
    udp.beginPacket(this_ip, UDP_PORT_SEND);

  udp.print(dataBuffer);
  udp.endPacket();

  #undef addStr
}


const String genParsName[] = {
    "GET_DATA",
    "LowPass",
    "RestartInAP",
    "RestartInSTA",
    "Restart",
    "ResetToDefaults",
    "ResetWiFiSettings",
    "portAux=",
    "powerState=",
    "powerType=",
    "mode=",
    "emptyBright=",
    "addNoiseLPVolume=",
    "addNoiseLPSpectr=",
    "maxCurrentPowerbank=",
    "maxCurrent=",
    "SmoothFreq=",
    "MaxCoefFreq=",
    "freqColorLow=",
    "freqColorMid=",
    "freqColorHigh=",
    "volumeSmooth=",
    "volumePalette=",
    "runningFreqSpeed=",
    "strobe1FreqSpeed=",
    "strobe5FreqSpeed=",
    "hueStart=",
    "hueStep=",
    "partyModeSideSpeed=",
    "staticColorHue=",
    "staticColorSat=",
    "runningRainbowSpeed=",
    "runningRainbowStep=",
    "fadeRainbowSpeed=",
    "fireHueGap=",
    "fireStep=",
    "fireHueStart=",
    "fireMinBrightness=",
    "fireMinSaturation=",
    "fireMaxSaturation=",
    "runningRusFlagSpeed=",
    "strobeRGBSpeed=",
    "pulseColorPulseHue=",
    "pulseColorPulseSat=",
    "strobeWhiteSpeed=",
    "bright="
  };
int arraySize = sizeof(genParsName) / sizeof(genParsName[0]); 

int getParamNumber(String nData) {
  for (int i = 0; i < arraySize; i++) 
    if (nData.indexOf(genParsName[i]) != -1) {
      Serial.printf("Name: %s\n\r", genParsName[i].c_str());
      return i;
    }

  return -1;
}

float getNewVar(String nData) {
  float am = nData.substring(nData.indexOf("=") + 1).toFloat();
  return am;
}

//Обновление переменных
void updateData(String newData) {
  const int paramNum = getParamNumber(newData);

  Serial.printf("Num: %i\n\r", paramNum);
  Serial.printf("Amount: %f\n\r", getNewVar(newData));
  Serial.printf("Data: %s\n\r", newData.c_str());

  switch (paramNum) {
    case -1:
      return; 
      break;
    case 0:  //GET_DATA
      sendDataToApp(); 
      break;
    case 1: //lowpass
      updateLowPass(); 
      break;
    case 2: //RestartInAP
      ESP.restart();
      break;
    case 3: //RestartInSTA
      ESP.restart();
      break;
    case 4: //Restart
      ESP.restart();
      break;
    case 5: //ResetToDefaults
      ESP.restart();
      break;
    case 6: //ResetWiFiSettings
      ESP.restart();
      break;
    case 7: //portAux
      data.portAux = getNewVar(newData); 
      break;
    case 8: //powerState
      updatePowerState(getNewVar(newData));
      break;
    case 9: //powerType
      updatePowerType(getNewVar(newData)); 
      break;
    case 10: //mode
      data.mode = getNewVar(newData); 
      break;
    case 11: //emptyBright
      data.emptyBright = getNewVar(newData); 
      break;
    case 12: //addNoiseLPVolume
      data.addNoiseLPVolume = getNewVar(newData); 
      break;
    case 13: //addNoiseLPSpectr
      data.addNoiseLPSpectr = getNewVar(newData); 
      break;
    case 14: //maxCurrent
      data.maxCurrent = getNewVar(newData); 
      break;
    case 15: //maxCurrentPowerbank
      data.maxCurrentPowerbank = getNewVar(newData); 
      break;
    case 16: //SmoothFreq
      data.SmoothFreq = getNewVar(newData); 
      break;
    case 17: //MaxCoefFreq
      data.MaxCoefFreq = getNewVar(newData); 
      break;
    case 18: //freqColorLow
      data.freqColors[0] = getNewVar(newData); 
      break;
    case 19: //freqColorMid
      data.freqColors[1] = getNewVar(newData); 
      break;
    case 20: //freqColorHigh
      data.freqColors[2] = getNewVar(newData); 
      break;
    case 21: //volumeSmooth
      data.volumeSmooth = getNewVar(newData); 
      break;
    case 22: //volumePalette
      data.volumePalette = getNewVar(newData); 
      break;
    case 23: //runningFreqSpeed
      data.runningFreqSpeed = getNewVar(newData); 
      break;
    case 24: //strobe1FreqSpeed
      data.strobe1FreqSpeed = getNewVar(newData); 
      break;
    case 25: //strobe5FreqSpeed
      data.strobe5FreqSpeed = getNewVar(newData); 
      break;
    case 26: //hueStart
      data.hueStart = getNewVar(newData); 
      break;
    case 27: //hueStep
      data.hueStep = getNewVar(newData); 
      break;
    case 28: //partyModeSideSpeed
      data.partyModeSideSpeed = getNewVar(newData); 
      break;
    case 29: //staticColorHue
      data.staticColorHue = getNewVar(newData); 
      break;
    case 30: //staticColorSat
      data.staticColorSat = getNewVar(newData); 
      break;
    case 31: //runningRainbowSpeed
      data.runningRainbowSpeed = getNewVar(newData); 
      break;
    case 32: //runningRainbowStep
      data.runningRainbowStep = getNewVar(newData); 
      break;
    case 33: //fadeRainbowSpeed
      data.fadeRainbowSpeed = getNewVar(newData); 
      break;
    case 34: //fireHueGap
      data.fireHueGap = getNewVar(newData); 
      break;
    case 35: //fireStep
      data.fireStep = getNewVar(newData); 
      break;
    case 36: //fireHueStart
      data.fireHueStart = getNewVar(newData); 
      break;
    case 37: //fireMinBrightness
      data.fireMinBrightness = getNewVar(newData); 
      break;
    case 38: //fireMinSaturation
      data.fireMinSaturation = getNewVar(newData); 
      break;
    case 39: //fireMaxSaturation
      data.fireMaxSaturation = getNewVar(newData); 
      break;
    case 40: //runningRusFlagSpeed
      data.runningRusFlagSpeed = getNewVar(newData); 
      break;
    case 41: //strobeRGBSpeed
      data.strobeRGBSpeed = getNewVar(newData); 
      break;
    case 42: //pulseColorPulseHue
      data.pulseColorPulseHue = getNewVar(newData); 
      break;
    case 43: //pulseColorPulseSat
      data.pulseColorPulseSat = getNewVar(newData); 
      break;
    case 44: //strobeWhiteSpeed
      data.strobeWhiteSpeed = getNewVar(newData); 
      break;
    case 45: //bright
      updateBright(getNewVar(newData)); 
      break;
  }

  settings.update();
}

//Проверка ключа сети
int udpCheckNetKey(char buff[MAX_UDP_PACKET_SIZE]) {
  for (int i = 0; i < sizeof(NETWORK_KEY) - 1; i++) {
    if (buff[i] != NETWORK_KEY[i]) {
      Serial.printf("B: %c\n\rK: %c\n\rI: %i", buff[i], NETWORK_KEY[i]);   
      return -1;
    }
  }
  
  if (buff[sizeof(NETWORK_KEY) - 1] != ':') {
    return -1;
  }

  for (int i = sizeof(NETWORK_KEY); i < sizeof(NETWORK_KEY) + 12; i++) {
    if (buff[i] != uid[i - sizeof(NETWORK_KEY)]) {
      Serial.printf("B: %c\n\rU: %c\n\rI: %i", buff[i], uid[i], i); 
      return -1;
    }
  }
  
  if (buff[sizeof(NETWORK_KEY) + 12] != '>') {
    return -1;
  }
  
  return sizeof(NETWORK_KEY) + 12;
}

//Получение широковещательного адреса
void getBroadcastIp() {
  broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;
}

//Хандлер юдп
void udpListen() {
  if (!udp.parsePacket()) 
    return;

  char udpBuffer[MAX_UDP_PACKET_SIZE];
  int udpBufferSize = udp.read(udpBuffer, MAX_UDP_PACKET_SIZE);

  if (udpCheckNetKey(udpBuffer) > 0) 
    return;

  String parsedBuffer = "";

  for (int i = sizeof(NETWORK_KEY) - 1; i < udpBufferSize; i++) {
    if ((char)udpBuffer[i] == '%') break;
    parsedBuffer += (char)udpBuffer[i];
  }

  this_ip = udp.remoteIP();
  updateData(udpBuffer);
}



//Запуск перехвата запросов
void setupCaptivePortal(const IPAddress locIP) {
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.setTTL(3600);
	dnsServer.start(53, "*", locIP);
}

//Запуск сервера на точке доступа
void setupAPServer() {
  serverAP.onNotFound([](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  serverAP.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  serverAP.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css");
  });

  serverAP.on("/continue", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/submit.html");
    delay(100);
    return;
  });

  serverAP.on("/done", HTTP_POST, [](AsyncWebServerRequest *request) {
    String receivedSsid, receivedPassword;

    if (request->hasParam("ssid", true)) 
      receivedSsid = request->getParam("ssid", true)->value();
    
    if (request->hasParam("password", true)) 
      receivedPassword = request->getParam("password", true)->value();
    
    if (receivedSsid == "") {
      request->send(SPIFFS, "/index.html");
      return;
    }
    
    data.WIFI_SSID = receivedSsid;
    data.WIFI_PASS = receivedPassword;
    
    settings.updateNow();

    request->send(SPIFFS, "/submit.html");

    delay(100);

    ESP.restart();
  });

  serverAP.begin();
}

//Запуск точки доступа
void beginAP() {
  IPAddress localIP(192,168,1,1);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);
  
  broadcastIP = (localIP[0], localIP[1], localIP[2], 255);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(localIP, gateway, subnet);
  delay(200);
  WiFi.softAP(AP_NAME, AP_PASSWORD);

  fill_leds(0, (NUM_LEDS / 10), CRGB::Blue);
  delay(1000);

  Serial.println(WiFi.softAPIP());

  if (!wifi_saved || !data.wifi_mode) 
    setupCaptivePortal(localIP);

  setupAPServer();
  
  digitalWrite(IDICATE_PIN, HIGH);
}

//Подключение к сети
void connectToWiFi() {
  uint8_t counter = 0;
  uint32_t wifi_timer = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(data.WIFI_SSID, data.WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - wifi_timer > 1000) {
      wifi_timer = millis();
      
      fill_leds(0, map(counter, 0, WIFI_CONNECTION_TRYS, 1, NUM_LEDS), CRGB::Yellow, false);
      counter++;
      
      if (counter == WIFI_CONNECTION_TRYS) {
        fill_leds(0, (NUM_LEDS / 10), CRGB::Red);

        delay(1000);

        wifi_saved = false; //установка флага, что конфигурация wifi sta некорректная.
        settings.updateNow();
        
        beginAP();
        return;
      } 
    }
  }

  fill_leds(0, (NUM_LEDS / 10), CRGB::Green);

  wifi_saved = true; //установка флага, что конфигурация wifi sta корректная.
  settings.updateNow();
}

//Запуск обновления по воздуху
void otaInit() {
  ArduinoOTA.setHostname(OTA_NAME);
  ArduinoOTA.onEnd([]() {
    delay(100);
    ESP.restart();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });
  ArduinoOTA.begin();
}

//Инициализация сети
void networkInit() {
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  ArduinoOTA.end();
  dnsServer.stop();

  const uint64_t chipId = ESP.getEfuseMac();
  snprintf(uid, sizeof(uid), "%02X%02X%02X%02X%02X%02X",
    (uint8_t)(chipId),
    (uint8_t)(chipId >> 8),
    (uint8_t)(chipId >> 16),
    (uint8_t)(chipId >> 24),
    (uint8_t)(chipId >> 32),
    (uint8_t)(chipId >> 40)
  );
  Serial.println(uid);

  if (data.wifi_mode) beginAP();
  else connectToWiFi();

  getBroadcastIp();
  udp.begin(UDP_PORT_LISTEN);
  otaInit();

  delay(500);
}



void ledStripeInit() {
  FastLED.addLeds<LED_CHIP, LED_PIN, LED_COLOR>(leds, NUM_LEDS);

  updatePowerType(data.powerType);

  if (!data.powerType) {
    data.mode = 6;
    data.powerState = true;
    data.bright = constrain(data.bright, 0, 150);
  }

  FastLED.setBrightness(data.bright);
}

void spiffsBegin() {
  if (!SPIFFS.begin(true)){
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }

  switch (settings.read()) {
    case FD_FS_ERR: Serial.println("FS Error");
      break;
    case FD_FILE_ERR: Serial.println("Error");
      break;
    case FD_WRITE: Serial.println("Data Write");
      break;
    case FD_ADD: Serial.println("Data Add");
      break;
    case FD_READ: Serial.println("Data Read");
      break;
    case FD_NO_DIF: Serial.println("FD_NO_DIF");
      break;
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

  spiffsBegin();

  #ifdef ENABLE_BUTTON 
  buttonCheck();
  #endif

  ledStripeInit();
  networkInit();
  updateLowPass();

  Serial.println("Setup end");
}

void loop() { 
  computeSound();
  animation();

  if (settings.tick() == FD_WRITE) Serial.println("Data updated!");

  dnsServer.processNextRequest();
  ArduinoOTA.handle();
  udpListen();
}
