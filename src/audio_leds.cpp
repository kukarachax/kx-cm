#include "globals.h"
#include <FFT_C.h>

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
const uint16_t HALF_LED = NUM_LEDS / 2;

DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0, 0, 255, 0,  100, 255, 255, 0,  150, 255, 100, 0,  200, 255, 50, 0,  255, 255, 0, 0
};
CRGBPalette32 redtogreen_p = soundlevel_gp;

void fill_leds(const int from, const int to, CRGB color, const bool clr) {
  if (clr) FastLED.clear();
  for (int i = from; i < to; i++) leds[i] = color;
  FastLED.show();
}



void getFFT() {
  int raw[FFT_SIZE];
  for (int i = 0; i < FFT_SIZE; i++) raw[i] = analogRead(soundRList[data.portAux]);
  FFT(raw, spectr);
} 

void updateLowPass(const bool force_trigger) {
  if (force_trigger || lowpass_trigger) {
    int maxNoiseLevel = 0, noiseLevel = 0;
    fill_leds(0, 5, CRGB::Pink);

    for (int i = 0; i < 1500; i++) {
      noiseLevel = analogRead(soundRList[data.portAux]);        
      if (noiseLevel > maxNoiseLevel) maxNoiseLevel = noiseLevel;                                             
    }
    lowPassVolume = maxNoiseLevel + data.addNoiseLPVolume;
    maxNoiseLevel = 0;

    for (int i = 0; i < 1500; i++) {
      getFFT();
      for (uint8_t j = 3; j < 28; j++) 
        if (spectr[j] > maxNoiseLevel) maxNoiseLevel = spectr[j];
    }
    lowPassSpectr = maxNoiseLevel + data.addNoiseLPSpectr;

    lowpass_trigger = false;
  }
}

void filterFFT() {
  int colorMusic[3] = {0,0,0};
  for (int i = 0; i < FFT_SIZE; i++) if (spectr[i] < lowPassSpectr) spectr[i] = 0;
  for (uint8_t i = 1; i < 3; i++) if (spectr[i] > colorMusic[0]) colorMusic[0] = spectr[i];
  for (uint8_t i = 4; i < 12; i++) if (spectr[i] > colorMusic[1]) colorMusic[1] = spectr[i];
  for (uint8_t i = 13; i < 30; i++) if (spectr[i] > colorMusic[2]) colorMusic[2] = spectr[i];
    
  int freq_max = 0;
  for (uint8_t i = 0; i < 31; i++) { 
    if (spectr[i + 1] > freq_max) freq_max = spectr[i + 1]; 
    if (freq_max < 5) freq_max = 5;
    if (freq_min[i] < spectr[i + 1]) freq_min[i] = spectr[i + 1]; 
    if (freq_min[i] > 0) freq_min[i] -= LIGHT_SMOOTH; 
    else freq_min[i] = 0;
  }
  freq_max_f = freq_max * averK + freq_max_f * (1 - averK); 

  for (uint8_t i = 0; i < 3; i++) {
    colorMusic_aver[i] = colorMusic[i] * averK + colorMusic_aver[i] * (1 - averK);
    colorMusic_f[i] = colorMusic[i] * data.SmoothFreq + colorMusic_f[i] * (1 - data.SmoothFreq);
    
    if (colorMusic_f[i] > ((float)colorMusic_aver[i] * data.MaxCoefFreq)) {
      thisBright[i] = 255; colorMusicFlash[i] = true; running_flag[i] = true; 
    } else colorMusicFlash[i] = false; 

    if (thisBright[i] >= 0) thisBright[i] -= SMOOTH_STEP; 
    if (thisBright[i] < data.emptyBright) { 
      thisBright[i] = data.emptyBright; running_flag[i] = false; 
    }
  }
}

void filterVolume() {
  #define MAX_COEF 1.8
  float RsoundLevel = 0, LsoundLevel = 0;

  for (uint8_t i = 0; i < 100; i ++) {                     
    RcurrentLevel = analogRead(soundRList[data.portAux]);
    if (RsoundLevel < RcurrentLevel) RsoundLevel = RcurrentLevel;
  }
  RsoundLevel = constrain(map(RsoundLevel, lowPassVolume, ADC_BITS, 0, 500), 0, 500); 
  RsoundLevel = pow(RsoundLevel, data.EXP); 
  RsoundLevel_f = RsoundLevel * data.volumeSmooth + RsoundLevel_f * (1 - data.volumeSmooth);
  LsoundLevel_f = RsoundLevel_f;

  if (data.emptyBright > 5) {
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(data.emptyColor, 255, data.emptyBright); 
  }

  if (RsoundLevel_f > 15 && LsoundLevel_f > 15) {
    averageLevel = (float)(RsoundLevel_f + LsoundLevel_f) / 2 * averK + averageLevel * (1 - averK);
    maxLevel = (float)averageLevel * MAX_COEF;
    Rlenght = constrain(map(RsoundLevel_f, 0, maxLevel, 0, HALF_LED), 0, HALF_LED);
    Llenght = constrain(map(LsoundLevel_f, 0, maxLevel, 0, HALF_LED), 0, HALF_LED);
  }
  #undef MAX_COEF
}

void computeSound() {
  if (data.mode == 0) filterVolume();
  else if (data.mode < 6) { getFFT(); filterFFT(); }
}




void volumeAnimate() {
  const float index = (float)255 / HALF_LED;
  uint8_t counter = 0;
  if (data.emptyBright > 0) 
    for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(data.emptyColor, 255, data.emptyBright);

  auto applyPalette = [&](int i, uint8_t c) {
    switch (data.volumePalette) {
      case 0: leds[i] = ColorFromPalette(redtogreen_p, (c * index)); break;
      case 1: leds[i] = ColorFromPalette(RainbowColors_p, (c * index)); break;
      case 2: leds[i] = ColorFromPalette(ForestColors_p, (c * index)); break;
      case 3: leds[i] = ColorFromPalette(CloudColors_p, (c * index)); break;
      case 4: leds[i] = ColorFromPalette(LavaColors_p, (c * index)); break;
      case 5: leds[i] = ColorFromPalette(OceanColors_p, (c * index)); break;
    }
  };

  for (int i = HALF_LED - 1; i > HALF_LED - 1 - Rlenght; i--) applyPalette(i, counter++);
  counter = 0;
  for (int i = HALF_LED; i < HALF_LED + Llenght; i++) applyPalette(i, counter++);
}

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
      leds[i] = leds[i + 1]; leds[NUM_LEDS - i - 1] = leds[i];
    }
  } 
}

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
      else if (i < NUM_LEDS)      leds[i] = CHSV(data.freqColors[2], 255, thisBright[2]);
    }
  }
}

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

const int partyModeLeds = NUM_LEDS / 5;
void partyModeSideFlash() {
  static bool flash1State = false;
  static uint32_t flash1_timer = 0;
  if (!(colorMusicFlash[0] || colorMusicFlash[1] || colorMusicFlash[2])) {
    fill_solid(leds, NUM_LEDS, CRGB::Black); return;
  }
  if (millis() - flash1_timer > data.partyModeSideSpeed) {
    flash1_timer = millis();
    for (int i = 0; i < NUM_LEDS; i++) {
      if (i < partyModeLeds || i > partyModeLeds * 4 - 1) 
        leds[i] = flash1State ? CRGB::White : CRGB::Black;
    }
    flash1State = !flash1State;
  }
}

void partyModeAnimate() {
  partyModeSideFlash();
  for (int i = 0; i < partyModeLeds; i++) {
    leds[partyModeLeds * 2 + i] = colorMusicFlash[0] ? CRGB::Purple : CRGB::Black;
    leds[partyModeLeds + i]     = colorMusicFlash[1] ? CRGB::Red : CRGB::Black;
    leds[partyModeLeds * 3 + i] = colorMusicFlash[2] ? CRGB::Blue : CRGB::Black;
  }
}

void staticColorAnimate() {
  fill_solid(leds, NUM_LEDS, CHSV(data.staticColorHue, data.staticColorSat, data.bright));
}

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

uint32_t fadeRainbow_timer;
int rainbowFadeHue;
void fadeRainbowAnimate() {
  if (millis() - fadeRainbow_timer > data.fadeRainbowSpeed) {
    fadeRainbow_timer = millis();
    if (++rainbowFadeHue > 255) rainbowFadeHue = 0;
    fill_solid(leds, NUM_LEDS, CHSV(rainbowFadeHue, 255, data.bright));
  }
}

uint32_t perlinNoise_timer;
int FireCounter = 0;
CHSV getFireColor(int val) {
  return CHSV(data.fireHueStart + map(val, 0, 255, 0, data.fireHueGap), 
              constrain(map(val, 0, 255, data.fireMaxSaturation, data.fireMinSaturation), 0, 255), 
              constrain(map(val, 0, 255, data.fireMinBrightness, 255), 0, 255));
}
void perlinNoiseAnimate() {
  if (millis() - perlinNoise_timer > 20) {
    perlinNoise_timer = millis();
    for(int i = 0; i < NUM_LEDS; i++) leds[i] = getFireColor((inoise8(i * data.fireStep, FireCounter)));
    FireCounter += 20;
  }
}

int runningRusFlag_idex = 0;
uint32_t runningRusFlag_timer;
int shift_index(int current_index, int delta, int leds_count) { return (current_index + delta) % leds_count; }
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
    if (++strobeRGB_counter > 2) strobeRGB_counter = 0;
  }
}

void pulseColorAnimate() {
  static bool direct = false;
  static uint8_t iBright = 0;
  if (direct) { if (++iBright >= data.bright) direct = !direct; }
  else { if (--iBright < 1) direct = !direct; }
  for (int i = 0 ; i < NUM_LEDS; i++) leds[i] = CHSV(data.pulseColorPulseHue, data.pulseColorPulseSat, iBright);
}

uint32_t strobeWhite_timer;
bool strobeWhite_counter;
void strobeWhiteAnimate() {
  if (millis() - strobeWhite_timer > data.strobeWhiteSpeed) {
    strobeWhite_timer = millis();
    
    if (strobeWhite_counter) 
      fill_solid(leds, NUM_LEDS, CRGB::White);
    else 
      fill_solid(leds, NUM_LEDS, CRGB::Black);

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
  } else {
    FastLED.clear(); FastLED.setBrightness(0);
  }

  if (pair_enable) 
    fill_leds(0, 5, CRGB::White, false);
  

  FastLED.show();
}




void handleBright() {
  if (newBright == data.bright) return;

  if (!data.powerType) newBright = constrain(newBright, 0, 100);

  if (newBright < data.bright) {
    for (uint8_t i = data.bright; i > newBright; i-=2) {
      animation(); 
      if (i - newBright < 2) break; 
      FastLED.setBrightness(i);
    }
  } 
  else {
    for (uint8_t i = data.bright; i < newBright; i+=2) {
      animation(); 
      if (newBright - i < 2) break; 
      FastLED.setBrightness(i);
    }
  }

  FastLED.setBrightness(newBright); 
  data.bright = newBright;
}

void handlePowerState() {
  if (newPowerState == data.powerState) return;

  if (newPowerState) {
    data.powerState = newPowerState;
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
  }
  data.powerState = newPowerState;
}

void handlePowerType() {
  if (newPowerType == data.powerType) return;

  if (newPowerType) FastLED.setMaxPowerInVoltsAndMilliamps(5, data.maxCurrent * 1000);
  else FastLED.setMaxPowerInVoltsAndMilliamps(5, data.maxCurrentPowerbank * 1000);
  data.powerType = newPowerType;
}