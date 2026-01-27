// receiver_config.h
#ifndef RECEIVER_CONFIG_H
#define RECEIVER_CONFIG_H

#include <Arduino.h>

// Пины для ИК приемников
#define SENSOR1_PIN 14
#define SENSOR2_PIN 27
#define DEBOUNCE_TIME 1.0 * 1000000 // задержка от срабатывания датчиков пересечения 1сек
#define MIN_LAP_TIME 1.0 * 1000000
#define TIMER_COOLDOWN_PERIOD 5000 // 5 seconds cooldown

// Параметры Wi-Fi
#define WIFI_SSID "SFMTimer"
#define WIFI_PASSWORD "123456789"

// Параметры UDP
#define UDP_PORT 8888

// Светодиоды
#define STATUS_LED_PIN 2     // Встроенный светодиод (или другой пин)
#define MEASUREMENT_LED_PIN 4 // Пин для светодиода измерений
#define OPERATION_MODE_LED_PIN 33 // Пин для светодиода режима работы
#define LED_BLINK_INTERVAL 3000 // Интервал моргания 2 секунды
#define LED_BLINK_DURATION 100  // Длительность моргания 0.1 секунды
#define FAST_BLINK_INTERVAL 100 // Интервал быстрого моргания

#endif