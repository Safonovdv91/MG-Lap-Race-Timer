// config.h
#ifndef CONFIG_H
#define CONFIG_H

// Объявления без инициализации
extern const char* ssid;
extern const char* password;
extern const int serverPort;

// Настройки пинов
#define SENSOR1_PIN 13
#define SENSOR2_PIN 12
#define DEBOUNCE_TIME 50
#define MIN_LAP_TIME 3000000
#define HISTORY_SIZE 5


// Светодиоды
#define STATUS_LED_PIN 2     // Встроенный светодиод (или другой пин)
#define MEASUREMENT_LED_PIN 4 // Пин для светодиода измерений
#define LED_BLINK_INTERVAL 2000 // Интервал моргания 2 секунды
#define LED_BLINK_DURATION 500  // Длительность моргания 0.5 секунды
#endif
