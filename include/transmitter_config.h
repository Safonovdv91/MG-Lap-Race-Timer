// transmitter_config.h
#ifndef TRANSMITTER_CONFIG_H
#define TRANSMITTER_CONFIG_H

#include <Arduino.h>

// Пины для ИК передатчиков
#define IR_TX1_PIN 14  //D14 (GPIO14) Пин для управления первым ИК передатчиком

// Частота ШИМ для модуляции ИК сигнала (38 кГц)
#define PWM_CHANNEL_1 0  // Канал ШИМ для первого передатчика

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

void initIRTransmitters();

#endif