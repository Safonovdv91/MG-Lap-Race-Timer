#ifndef WEB_CONTENT_H
#define WEB_CONTENT_H

#include <Arduino.h>

#include <ArduinoJson.h>

void formatAndSetTimestamp(JsonObject obj, const char* key, unsigned long timestamp_ms);

#endif