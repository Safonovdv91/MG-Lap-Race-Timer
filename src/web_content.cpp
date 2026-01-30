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

// Функции для получения телеметрии излучателя (только для режима приемника)
#ifdef RECEIVER_MODE
extern int getTransmitterBatteryLevel();
extern float getTransmitterBatteryVoltage();
#endif



void formatAndSetTimestamp(JsonObject obj, const char* key, unsigned long timestamp_ms) {
  unsigned long seconds = timestamp_ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  seconds %= 60;
  minutes %= 60;

  char buffer[9]; // HH:MM:SS + null terminator
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  
  obj[key] = buffer; // ArduinoJson will copy the char array, no String object created
}