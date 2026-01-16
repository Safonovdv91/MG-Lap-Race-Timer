#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include <SPIFFS.h>
#include "measurements.h"
#include "web_content.h"

extern WebServer server;

void handleRoot();
void handleData();
void handleReset();
void handleMode();
void handleDistance();
void handleCSS();
void handleJS();
void resetMeasurements();
void handleWiFiSettings();
void handleUpdateWiFi();

// Функции для получения телеметрии излучателя (только для режима приемника)
#ifdef RECEIVER_MODE
int getTransmitterBatteryLevel();
float getTransmitterBatteryVoltage();
#endif

#endif