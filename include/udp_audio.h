#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>

#define AUDIO_UDP_PORT 5390       // Порт для приема аудиоданных
#define AUDIO_BUF_SIZE 1024        // Размер кольцевого буфера сэмплов

void initUDPAudio();
void handleUDPAudio();
uint16_t getNextAudioSample();
bool isAudioStreamActive();