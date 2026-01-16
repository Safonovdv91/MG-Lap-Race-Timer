// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Preferences.h>

extern Preferences preferences;

// Объявления без инициализации
extern char ssid[32];
extern char password[64];
extern const int serverPort;


// Объявления функций
void loadWiFiSettings();
void saveWiFiSettings();

// Настройки пинов

// Безопасные GPIO для датчиков (рекомендуемые):
//     GPIO14, GPIO27, GPIO26, GPIO25, GPIO33, GPIO32, GPIO15, GPIO2, GPIO4, GPIO5, GPIO18, GPIO19, GPIO21, GPIO22, GPIO23
//     Эти пины не используются критически при загрузке (кроме GPIO15, но он менее проблематичен).
// Пины, которых стоит избегать:
//     GPIO0, GPIO2, GPIO12, GPIO15 — участвуют в определении режима загрузки.
//     GPIO6-11 — подключены к внутренней flash-памяти (их использование может "убить" плату).
//     GPIO34-39 — только входные пины (без подтяжки), но безопасны для датчиков.

#define SENSOR1_PIN 14
#define SENSOR2_PIN 27

#define DEBOUNCE_TIME 1.0 * 1000000 // задержка от срабатывания датчиков пересечения 1сек
#define MIN_LAP_TIME 1.0 * 1000000
#define HISTORY_SIZE 5
#define DATA_UPDATE_INTERVAL 100 // Интервал обновления данных (мс)

#endif

// Светодиоды
#define STATUS_LED_PIN 2     // Встроенный светодиод (или другой пин)
#define MEASUREMENT_LED_PIN 4 // Пин для светодиода измерений
#define LED_BLINK_INTERVAL 2000 // Интервал моргания 2 секунды
#define LED_BLINK_DURATION 500  // Длительность моргания 0.5 секунды

// измерение напряжения
#define BATTERY_PIN 34
#define BATTERY_MIN_V 3.0  // Минимальное напряжение (0%)
#define BATTERY_MAX_V 4.2  // Максимальное напряжение (100%)

// Добавляем определения для ИК передатчиков
#ifdef USE_IR_SENSORS
#define IR_TX1_PIN 13  // Пин для управления первым ИК передатчиком
#define IR_TX2_PIN 12  // Пин для управления вторым ИК передатчиком
#endif
