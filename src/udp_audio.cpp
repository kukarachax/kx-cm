#include "udp_audio.h"

static WiFiUDP audioUdp;
static uint16_t audioBuffer[AUDIO_BUF_SIZE];
static volatile uint16_t head = 0;
static volatile uint16_t tail = 0;
static uint32_t lastPacketTime = 0;

void initUDPAudio() {
    audioUdp.begin(AUDIO_UDP_PORT);
}

void handleUDPAudio() {
    int packetSize = audioUdp.parsePacket();
    if (packetSize > 0) {
        
        Serial.printf("New packet %i", packetSize);
        // Ограничиваем размер буфера чтения
        uint8_t tempBuf[512];
        int bytesRead = audioUdp.read(tempBuf, sizeof(tempBuf));
        
        // Предполагается формат 16-бит unsigned / signed PCM или 8-бит PCM.
        // Перевод поступающих данных в формат 12-битного АЦП ESP32 (0..4095)
        for (int i = 0; i < bytesRead; i += 2) {
            if (i + 1 < bytesRead) {
                // Прием 16-битного PCM (Little Endian)
                int16_t rawSample = (int16_t)(tempBuf[i] | (tempBuf[i + 1] << 8));
                
                // Масштабирование 16-бит signed (-32768..32767) в 12-бит ADC (0..4095)
                uint16_t adcSample = (uint16_t)map(rawSample, -32768, 32767, 0, 4095);

                uint16_t nextHead = (head + 1) % AUDIO_BUF_SIZE;
                if (nextHead != tail) { // Проверка на переполнение буфера
                    audioBuffer[head] = adcSample;
                    head = nextHead;
                }
            }
        }
        lastPacketTime = millis();
    }
}

// Получение очередного сэмпла из кольцевого буфера
uint16_t getNextAudioSample() {
    if (head == tail) {
        return 2048; // Возврат среднего значения (тишина), если буфер пуст
    }
    uint16_t sample = audioBuffer[tail];
    tail = (tail + 1) % AUDIO_BUF_SIZE;
    return sample;
}

// Проверка активности потока (таймаут 1 сек)
bool isAudioStreamActive() {
    return (millis() - lastPacketTime < 1000);
}