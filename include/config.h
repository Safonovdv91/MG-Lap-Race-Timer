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


#define MIN_LAP_TIME 1.0 * 1000000
#define HISTORY_SIZE 5
#define DATA_UPDATE_INTERVAL 300 // Интервал обновления данных (мс)

// измерение напряжения
#define BATTERY_PIN 34
#define BATTERY_MIN_V 3.0  // Минимальное напряжение (0%)
#define BATTERY_MAX_V 4.2  // Максимальное напряжение (100%)
 // Настройки делителя напряжения для измерения батареи, используем подстроечный делитель. 
 // Calibrate: REAL_V(на батарее) / ADC_PIN_V (на АЦП) - формула рассчета  
 //         |   Vin    |  Vcc  | ADC_PIN_V 32| coeff
 // (USB)   |   5.00   | 4.72  |   2.49   |  1.895582
 //         |   4.00   | 3.927 |   2.256  |  1.740691
 //         |   3.92   | 3.812 |   2.172  |  1.740691 
 //         |   3.67   | 3.572 |   2.038  | 
 //   3.7   |   3.60   | 3.572 |   2.000  | 
 //   3.65  |   3.55   | 3.549 |   1.971  | 
 //   3.62  |   3.53   | 3.533 |   1.965  | 
 
#define VOLTAGE_DIVIDER_MULTIPLIER 1.740691
// Характеристики АЦП ESP32
#define ADC_REFERENCE_VOLTAGE 3.3 // Опорное напряжение АЦП ESP32
#define ADC_MAX_READING 4095.0    // Максимальное значение 12-битного АЦП   
#endif
