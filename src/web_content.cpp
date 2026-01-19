#include "../include/web_content.h"
#include "../include/config.h"
#include "../include/measurements.h"
#include <FS.h>
#include <SPIFFS.h>
#include <time.h>

// Внешние переменные (определены в других файлах)
extern Mode currentMode;
extern float distance;
extern Measurement speedHistory[HISTORY_SIZE];
extern Measurement lapHistory[HISTORY_SIZE];
extern int historyIndex;
extern float batteryVoltage;
extern int batteryPercentage;

// Функции для получения телеметрии излучателя (только для режима приемника)
#ifdef RECEIVER_MODE
extern int getTransmitterBatteryLevel();
extern float getTransmitterBatteryVoltage();
#endif

#include "../include/web_content.h"
#include "../include/config.h"
#include "../include/measurements.h"
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <time.h>

// Make the server object available here
extern WebServer server;

// Внешние переменные (определены в других файлах)
extern Mode currentMode;
extern float distance;
extern Measurement speedHistory[HISTORY_SIZE];
extern Measurement lapHistory[HISTORY_SIZE];
extern int historyIndex;
extern float batteryVoltage;
extern int batteryPercentage;

// Функции для получения телеметрии излучателя (только для режима приемника)
#ifdef RECEIVER_MODE
extern int getTransmitterBatteryLevel();
extern float getTransmitterBatteryVoltage();
#endif



String formatTimestamp(unsigned long timestamp) {
  time_t rawtime = timestamp / 1000; // Преобразуем миллисекунды в секунды
  struct tm *timeinfo = localtime(&rawtime);
  
  char buffer[9]; // HH:MM:SS + null terminator
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
  
  return String(buffer);
}