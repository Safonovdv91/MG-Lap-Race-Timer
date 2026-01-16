// receiver_config.h
#ifndef RECEIVER_CONFIG_H
#define RECEIVER_CONFIG_H

#include <Arduino.h>

// Пины для ИК приемников
#define SENSOR1_PIN 14
#define SENSOR2_PIN 27
#define DEBOUNCE_TIME 1.0 * 1000000 // задержка от срабатывания датчиков пересечения 1сек
#define MIN_LAP_TIME 1.0 * 1000000

// Параметры Wi-Fi
#define WIFI_SSID "SFMTimer"
#define WIFI_PASSWORD "123456789"

// Параметры UDP
#define UDP_PORT 8888

// Светодиоды
#define STATUS_LED_PIN 2     // Встроенный светодиод (или другой пин)
#define MEASUREMENT_LED_PIN 4 // Пин для светодиода измерений
#define LED_BLINK_INTERVAL 2000 // Интервал моргания 2 секунды
#define LED_BLINK_DURATION 500  // Длительность моргания 0.5 секунды

#endif