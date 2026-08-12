#include "globals.h"
#include "udp_audio.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

IPAddress broadcastIP, this_ip;
AsyncWebServer serverAP(80);
DNSServer dnsServer;
WiFiUDP udp;

void getBroadcastIp() {
  broadcastIP = WiFi.localIP();
  broadcastIP[3] = 255;
}


void udpSend(char *buf, bool broadcast = false) {
  if (broadcast) udp.beginPacket(broadcastIP, UDP_PORT_SEND);
  else udp.beginPacket(this_ip, UDP_PORT_SEND);
  udp.print(buf);
  udp.endPacket();
}

void sendDataToApp() {
  String dataBuffer = String(NETWORK_KEY);
  #define addStr(t) dataBuffer = dataBuffer + t + ";"
    addStr(data.portAux); addStr(data.powerState); addStr(data.mode); addStr(data.emptyBright); 
    addStr(data.bright); addStr(data.emptyColor); addStr(data.volumePalette); 
    addStr(data.freqColors[0]); addStr(data.freqColors[1]); addStr(data.freqColors[2]); 
    addStr(data.hueStart); addStr(data.hueStep); addStr(data.staticColorHue); 
    addStr(data.staticColorSat); addStr(data.fireHueGap); addStr(data.fireStep); 
    addStr(data.fireHueStart); addStr(data.fireMinBrightness); addStr(data.fireMinSaturation); 
    addStr(data.fireMaxSaturation); addStr(data.pulseColorPulseHue); addStr(data.pulseColorPulseSat); 
    addStr(data.addNoiseLPVolume); addStr(data.addNoiseLPSpectr); addStr(data.maxCurrent); 
    addStr(data.maxCurrentPowerbank); addStr(data.volumeSmooth); addStr(data.SmoothFreq); 
    addStr(data.MaxCoefFreq); addStr(data.runningFreqSpeed); addStr(data.strobe1FreqSpeed); 
    addStr(data.strobe5FreqSpeed); addStr(data.runningRainbowSpeed); addStr(data.runningRainbowStep); 
    addStr(data.fadeRainbowSpeed); addStr(data.runningRusFlagSpeed); addStr(data.strobeWhiteSpeed); 
    addStr(data.strobeRGBSpeed); addStr(data.partyModeSideSpeed); addStr(data.addNoiseLPVolume);
    addStr(data.addNoiseLPSpectr); addStr(data.powerType);
  #undef addStr

  udp.beginPacket(this_ip, UDP_PORT_SEND);

  udp.print(dataBuffer);
  udp.endPacket();
  
}

void startPairing() { 
  char str_buf[strlen(NETWORK_KEY) + 13 + 5];
  snprintf(str_buf, sizeof(str_buf), "CM/%s/%s", NETWORK_KEY, uid);
  
  udpSend(str_buf, true);
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
    if (nData.indexOf(genParsName[i]) != -1) return i;
  return -1;
}

float getNewVar(String nData) { 
  return nData.substring(nData.indexOf("=") + 1).toFloat(); 
}

void restartESP(uint8_t nMode = 0) {
  switch (nMode) {
  case 0: ESP.restart();
    break;
  case 1: 
    data.wifi_mode = true;
    settings.updateNow();
    settings.tick();
    delay(50);
    ESP.restart();
    break;
  case 2: 
    data.wifi_mode = false;
    settings.updateNow();
    settings.tick();
    delay(50);
    ESP.restart();
    break;
  case 3:
    settings.reset();
    settings.tick();
    delay(50);
    ESP.restart();
    break;
  case 4:
    data.WIFI_SSID = "";
    data.WIFI_PASS = "";
    settings.updateNow();
    settings.tick();
    delay(50);
    ESP.restart();
    break;
  
  default:
    break;
  }
}

void updateData(String newData) {
  const int paramNum = getParamNumber(newData);
  switch (paramNum) {
    case -1: return; 
    case 0: sendDataToApp(); break;
    case 1: lowpass_trigger = true; break;
    case 2: restartESP(1); break;
    case 3: restartESP(2); break;
    case 4: restartESP(); break;
    case 5: restartESP(3); break;
    case 6: restartESP(4); break;
    case 7: data.portAux = getNewVar(newData); break;
    case 8: newPowerState = getNewVar(newData); break;
    case 9: newPowerType = getNewVar(newData); break;
    case 10: data.mode = getNewVar(newData); break;
    case 11: data.emptyBright = getNewVar(newData); break;
    case 12: data.addNoiseLPVolume = getNewVar(newData); break;
    case 13: data.addNoiseLPSpectr = getNewVar(newData); break;
    case 14: data.maxCurrent = getNewVar(newData); break;
    case 15: data.maxCurrentPowerbank = getNewVar(newData); break;
    case 16: data.SmoothFreq = getNewVar(newData); break;
    case 17: data.MaxCoefFreq = getNewVar(newData); break;
    case 18: data.freqColors[0] = getNewVar(newData); break;
    case 19: data.freqColors[1] = getNewVar(newData); break;
    case 20: data.freqColors[2] = getNewVar(newData); break;
    case 21: data.volumeSmooth = getNewVar(newData); break;
    case 22: data.volumePalette = getNewVar(newData); break;
    case 23: data.runningFreqSpeed = getNewVar(newData); break;
    case 24: data.strobe1FreqSpeed = getNewVar(newData); break;
    case 25: data.strobe5FreqSpeed = getNewVar(newData); break;
    case 26: data.hueStart = getNewVar(newData); break;
    case 27: data.hueStep = getNewVar(newData); break;
    case 28: data.partyModeSideSpeed = getNewVar(newData); break;
    case 29: data.staticColorHue = getNewVar(newData); break;
    case 30: data.staticColorSat = getNewVar(newData); break;
    case 31: data.runningRainbowSpeed = getNewVar(newData); break;
    case 32: data.runningRainbowStep = getNewVar(newData); break;
    case 33: data.fadeRainbowSpeed = getNewVar(newData); break;
    case 34: data.fireHueGap = getNewVar(newData); break;
    case 35: data.fireStep = getNewVar(newData); break;
    case 36: data.fireHueStart = getNewVar(newData); break;
    case 37: data.fireMinBrightness = getNewVar(newData); break;
    case 38: data.fireMinSaturation = getNewVar(newData); break;
    case 39: data.fireMaxSaturation = getNewVar(newData); break;
    case 40: data.runningRusFlagSpeed = getNewVar(newData); break;
    case 41: data.strobeRGBSpeed = getNewVar(newData); break;
    case 42: data.pulseColorPulseHue = getNewVar(newData); break;
    case 43: data.pulseColorPulseSat = getNewVar(newData); break;
    case 44: data.strobeWhiteSpeed = getNewVar(newData); break;
    case 45: newBright = getNewVar(newData); break;
  }
  settings.update();
}

bool udpCheckNetKey(char buff[MAX_UDP_PACKET_SIZE]) {
  for (int i = 0; i < strlen(NETWORK_KEY); i++) {
    if (buff[i] != NETWORK_KEY[i]) {
      Serial.printf("B: %c\n\rK: %c\n\rI: %i", buff[i], NETWORK_KEY[i]);   
      return true;
    }
  }
  
  if (buff[strlen(NETWORK_KEY)] != ':') {
    Serial.printf("Error : char [%c]", buff[strlen(NETWORK_KEY) + 1]);
    return true;
  }
    
  

  for (int i = strlen(NETWORK_KEY); i < strlen(NETWORK_KEY) + 12; i++) {
    if (buff[i + 1] != uid[i - strlen(NETWORK_KEY)]) {
      Serial.printf("B: %c\n\rU: %c\n\rI: %i", buff[i], uid[i], i); 
      return true;
    }
  }
  
  if (buff[strlen(NETWORK_KEY) + 13] != '>') {
    Serial.printf("Error > char [%c]", buff[strlen(NETWORK_KEY) + 13]);
    return true;
  }
  
  return false;
}

bool checkPairRequest(char buff[MAX_UDP_PACKET_SIZE]) {
  for (int i = 0; i < strlen(PAIR_KEY) - 1; i++) 
    if (buff[i] != PAIR_KEY[i]) return false;
  return true;
}

void udpListen() {
  if (!udp.parsePacket()) return;
  char udpBuffer[MAX_UDP_PACKET_SIZE];
  int udpBufferSize = udp.read(udpBuffer, MAX_UDP_PACKET_SIZE);
  
  if (checkPairRequest(udpBuffer) && pair_enable) { 
    startPairing(); 
    return; 
  }
  if (udpCheckNetKey(udpBuffer)) return;

  String parsedBuffer = "";
  for (int i = strlen(NETWORK_KEY) - 1; i < udpBufferSize; i++) {
    if ((char)udpBuffer[i] == '%') break;
    parsedBuffer += (char)udpBuffer[i];
  }
  this_ip = udp.remoteIP();
  updateData(udpBuffer);
}



void setupCaptivePortal(const IPAddress locIP) {
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.setTTL(3600);
  dnsServer.start(53, "*", locIP);
}

void setupAPServer() {
  serverAP.onNotFound([](AsyncWebServerRequest *request) { 
    request->send(LittleFS, "/index.html"); 
  });
  serverAP.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(LittleFS, "/style.css"); 
  });
  serverAP.on("/continue", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/submit.html"); 
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
      request->send(LittleFS, "/index.html"); 
      return; 
    }
    
    data.WIFI_SSID = receivedSsid;
    data.WIFI_PASS = receivedPassword;
    data.wifi_saved_flag = true;
    settings.updateNow();
    request->send(LittleFS, "/submit.html");
    delay(100);

    ESP.restart();
  });
  serverAP.begin();
}

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

  if (!data.wifi_saved_flag || !data.wifi_mode) setupCaptivePortal(localIP);
  setupAPServer();
  digitalWrite(IDICATE_PIN, HIGH);
}

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
        data.wifi_saved_flag = false; 
        settings.updateNow();
        delay(1000);

        beginAP();
        return;
      } 
    }
  }
  fill_leds(0, (NUM_LEDS / 10), CRGB::Green);
  data.wifi_saved_flag = true; 
  settings.updateNow();
}

void otaInit() {
  ArduinoOTA.setHostname(OTA_NAME);
  ArduinoOTA.onEnd([]() { delay(100); ESP.restart(); });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("Error[%u]: ", error); });
  ArduinoOTA.begin();
}

void getLocalUID() {
  const uint64_t chipId = ESP.getEfuseMac();
  snprintf(uid, sizeof(uid), "%02X%02X%02X%02X%02X%02X",
    (uint8_t)(chipId), (uint8_t)(chipId >> 8), (uint8_t)(chipId >> 16),
    (uint8_t)(chipId >> 24), (uint8_t)(chipId >> 32), (uint8_t)(chipId >> 40)
  );
}

void networkInit() {
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  ArduinoOTA.end();
  dnsServer.stop();
  getLocalUID();

  if (data.wifi_mode || !data.wifi_saved_flag) beginAP();
  else connectToWiFi();

  getBroadcastIp();
  initUDPAudio();
  udp.begin(UDP_PORT_LISTEN);
  otaInit();
  delay(500);
}


void Core0Handler(void *pvParameters) {
  while (true) {
    if (settings.tick() == FD_WRITE) Serial.println("Data updated!");

    dnsServer.processNextRequest();
    ArduinoOTA.handle();
    udpListen();
    handleUDPAudio();
    handleIR();

    vTaskDelay(pdMS_TO_TICKS(2));
  }
  
}