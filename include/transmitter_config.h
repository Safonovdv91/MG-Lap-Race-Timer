// transmitter_config.h
#ifndef TRANSMITTER_CONFIG_H
#define TRANSMITTER_CONFIG_H

#include <Arduino.h>

// Пины для ИК передатчиков
#define IR_TX1_PIN 13  // Пин для управления первым ИК передатчиком
#define IR_TX2_PIN 12  // Пин для управления вторым ИК передатчиком

// Частота ШИМ для модуляции ИК сигнала (38 кГц)
#define IR_CARRIER_FREQ 38000
#define PWM_CHANNEL_1 0  // Канал ШИМ для первого передатчика
#define PWM_CHANNEL_2 1  // Канал ШИМ для второго передатчика
#define PWM_RESOLUTION 10 // Разрешение ШИМ (10 бит = 0-1023)

// Параметры Wi-Fi
#define WIFI_SSID "SFMTimer"
#define WIFI_PASSWORD "123456789"

// Параметры UDP
#define UDP_PORT 8888
#define BEACON_INTERVAL 5000 // Интервал отправки beacon сообщений (мс)

// Пины для измерения напряжения батареи
#define BATTERY_PIN 34
#define BATTERY_MIN_V 3.0  // Минимальное напряжение (0%)
#define BATTERY_MAX_V 4.2  // Максимальное напряжение (100%)

#endif