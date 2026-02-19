// receiver_config.h
#ifndef RECEIVER_CONFIG_H
#define RECEIVER_CONFIG_H

#include <Arduino.h>

// Пины для ИК приемников
#define SENSOR_PIN 13


// Параметры Wi-Fi
#define WIFI_SSID "SFMTimer"
#define WIFI_PASSWORD "123456789"

// Параметры UDP
#define UDP_PORT 8888

// Светодиоды
#define STATUS_LED_PIN 2            // Встроенный светодиод (или другой пин)
#define MEASUREMENT_LED_PIN 4       // Пин для светодиода измерений
#define OPERATION_MODE_LED_PIN 33   // Пин для светодиода режима работы
#define STATUS_IR_LED_PIN 32        // Пин для отображения статуса приемника 

#define LED_BLINK_INTERVAL 3000     // Интервал моргания 2 секунды
#define LED_BLINK_DURATION 100      // Длительность моргания 0.1 секунды
#define FAST_BLINK_INTERVAL 100     // Интервал быстрого моргания


// Настройка срабатывания
#define MIN_LAP_TIME 2.0 * 1000000          // 2 c      | минимальное время проезда круга
#define TIMER_COOLDOWN_PERIOD 5000          // 5 с      |
#define DEBOUNCE_TIME 2.0 * 1000000         // 1 с      | задержка от срабатывания датчиков пересечения 1сек

#define HISTORY_SIZE 5
#define DATA_UPDATE_INTERVAL 300            // 300 мс   | Интервал обновления данных (мс)
#define BEAM_BREAK_THRESHOLD 4.0 * 1000     // 4 мс     | Порог для обнаружения прерывания луча в микросекундах 

#endif