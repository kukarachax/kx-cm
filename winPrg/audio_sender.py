#!/usr/bin/env python3
"""
ESP32 Audio Stream Sender - Отправка аудиопотока на ESP32 через UDP

Этот скрипт принимает аудио с микрофона и отправляет его на ESP32 через UDP.
Поддерживает:
- Mono (1 канал) или Stereo (2 канала)
- 48000 Hz sample rate
- 16-bit PCM формат
"""

import socket
import struct
import sys
import wave
import pyaudio
import time
from collections import deque

# Настройки
UDP_HOST = '0.0.0.0'  # Слушать все интерфейсы
UDP_PORT = 5004       # Порт ESP32 (соответствует AUDIO_UDP_PORT в globals.h)
BUFFER_SIZE = 1024    # Размер буфера для отправки

# Формат пакета: "AUDI" + uint32_t length + raw PCM data
AUDIO_MAGIC = b'AUDI'


def create_audio_packet(samples):
    """Создаёт UDP пакет с аудио данными"""
    # Преобразуем сэмплы в bytes (little-endian int16)
    sample_bytes = b''.join(struct.pack('<h', s) for s in samples)
    
    # Создаём заголовок пакета
    packet = AUDIO_MAGIC + struct.pack('<I', len(sample_bytes)) + sample_bytes
    
    return packet


def send_audio_stream():
    """Основная функция отправки аудио потока"""
    print(f"ESP32 Audio Stream Sender")
    print(f"UDP Host: {UDP_HOST}")
    print(f"UDP Port: {UDP_PORT}")
    print()
    
    # Инициализация PyAudio
    p = pyaudio.PyAudio()
    
    try:
        # Настройки микрофона
        FORMAT = pyaudio.paInt16  # 16-bit
        CHANNELS = 1              # Stereo (можно изменить на 1 для mono)
        RATE = 48000              # Sample rate
        CHUNK = BUFFER_SIZE // 2  # Количество сэмплов за пакет
        
        print(f"Opening microphone...")
        stream = p.open(
            format=FORMAT,
            channels=CHANNELS,
            rate=RATE,
            input=True,
            output=False,
            frames_per_buffer=CHUNK
        )
        
        print("Microphone opened successfully!")
        print(f"Sample Rate: {RATE} Hz")
        print(f"Channels: {CHANNELS}")
        print()
        
        # Создание UDP сокета
        udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        udp_socket.bind((UDP_HOST, UDP_PORT))
        
        print(f"UDP socket created and bound to {UDP_HOST}:{UDP_PORT}")
        print()
        
        # Буфер для плавной обработки
        audio_buffer = deque(maxlen=BUFFER_SIZE * 2)
        
        # Задержка перед началом отправки (чтобы ESP32 успел инициализироваться)
        time.sleep(2)
        
        print("Starting audio stream...")
        print("(Press Ctrl+C to stop)")
        print()
        
        start_time = time.time()
        packets_sent = 0
        
        while True:
            try:
                # Чтение аудио данных
                data = stream.read(CHUNK, exception_on_overflow=False)
                
                # Преобразование из bytes в сэмплы int16
                samples = struct.unpack(f'<{CHUNK}h', data)
                
                # Добавляем в буфер (для плавной обработки)
                audio_buffer.extend(samples)
                
                # Создаём пакет и отправляем
                packet = create_audio_packet(list(audio_buffer))
                udp_socket.sendto(packet, ('255.255.255.255', UDP_PORT))  # Broadcast
                
                packets_sent += 1
                
            except Exception as e:
                print(f"Error reading audio: {e}")
            
            # Периодическая статистика
            if packets_sent % 100 == 0:
                elapsed = time.time() - start_time
                rate = packets_sent / elapsed if elapsed > 0 else 0
                print(f"Packets sent: {packets_sent} | Rate: {rate:.1f}/s")
        
    except KeyboardInterrupt:
        print("\nStopping audio stream...")
    finally:
        # Очистка ресурсов
        stream.stop_stream()
        stream.close()
        p.terminate()
        udp_socket.close()
        
        print("Audio stream stopped.")


def send_test_packet():
    """Отправка тестового пакета для проверки связи"""
    import random
    
    print("Sending test packet...")
    
    # Создаём тестовые сэмплы (синусоида)
    samples = []
    for i in range(BUFFER_SIZE):
        freq = 440  # А-мажор
        sample = int(32767 * 0.5 * (1 + 2.718**((i / RATE - int(i / RATE)) * 2 * 3.14159 * freq)))
        samples.append(sample)
    
    packet = create_audio_packet(samples)
    
    # Отправка на broadcast
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    udp_socket.bind((UDP_HOST, UDP_PORT))
    
    try:
        udp_socket.sendto(packet, ('255.255.255.255', UDP_PORT))
        print("Test packet sent successfully!")
        
        # Попробуем получить ответ от ESP32 (если он поддерживает)
        udp_socket.settimeout(1)
        try:
            response, addr = udp_socket.recvfrom(1024)
            print(f"Response from {addr}: {response[:50]}")
        except socket.timeout:
            print("No response received (ESP32 may not be listening)")
            
    finally:
        udp_socket.close()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--test':
        send_test_packet()
    else:
        send_audio_stream()