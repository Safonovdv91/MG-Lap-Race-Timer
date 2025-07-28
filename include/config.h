// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Preferences.h>

extern Preferences preferences;

// Объявления без инициализации
extern const char* ssid;
extern const char* password;
extern const int serverPort;


// Объявления функций
void loadWiFiSettings();
void saveWiFiSettings();

// Настройки пинов
#define SENSOR1_PIN 13
#define SENSOR2_PIN 12
#define DEBOUNCE_TIME 5 * 1000000 // задержка от срабатывания датчиков пересечения 5сек
#define MIN_LAP_TIME 3000000
#define HISTORY_SIZE 5
#define DATA_UPDATE_INTERVAL 100 // Интервал обновления данных (мс)

#endif

// Светодиоды
#define STATUS_LED_PIN 2     // Встроенный светодиод (или другой пин)
#define MEASUREMENT_LED_PIN 4 // Пин для светодиода измерений
#define LED_BLINK_INTERVAL 2000 // Интервал моргания 2 секунды
#define LED_BLINK_DURATION 500  // Длительность моргания 0.5 секунды
