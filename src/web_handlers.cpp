#include "../include/web_handlers.h"
#include "../include/config.h"
#include "../include/measurements.h"
#include "../include/web_content.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

WebServer server(80);

// Внешние переменные (определены в других файлах)
extern Mode currentMode;
extern float distance;
extern Measurement speedHistory[HISTORY_SIZE];
extern Measurement lapHistory[HISTORY_SIZE];
extern int historyIndex;
extern float batteryVoltage;
extern int batteryPercentage;

// Переменные для хранения данных излучателя (только для режима приемника)
#ifdef RECEIVER_MODE
struct TransmitterTelemetry {
  int batteryLevel = -1;
  float batteryVoltage = 0.0;
  unsigned long lastUpdate = 0;
} transmitterData;

int getTransmitterBatteryLevel() {
  // Если данных нет более 10 секунд, считаем что излучатель отключен
  if (millis() - transmitterData.lastUpdate > 10000) {
    return -1; // Сигнал о том, что излучатель недоступен
  }
  return transmitterData.batteryLevel;
}

float getTransmitterBatteryVoltage() {
  return transmitterData.batteryVoltage;
}
#endif

// Объявление внешних функций из measurements.cpp
extern unsigned long long getStartTimeSafe();
extern unsigned long long getCurrentRaceTimeSafe();
extern bool getSensor1TriggeredSafe();
extern bool getSensor2TriggeredSafe();
extern bool getMeasurementReadySafe();

void handleRoot() {
  String html = generateHTMLContent();
  server.send(200, "text/html", html);
}

// Global JSON document to avoid heap fragmentation
StaticJsonDocument<1024> doc;

void handleData() {
  // Clear the document before use
  doc.clear();

  // Добавляем базовые данные
  doc["mode"] = currentMode;
  doc["distance"] = distance;
  doc["value"] = currentValue;
  doc["battery"] = batteryPercentage;
  doc["voltage"] = batteryVoltage;
  
  // Добавляем информацию о телеметрии излучателя (только в режиме приемника)
  #ifdef RECEIVER_MODE
  int txBatteryLevel = getTransmitterBatteryLevel();
  doc["tx_battery"] = txBatteryLevel;
  doc["tx_voltage"] = getTransmitterBatteryVoltage();
  #endif
  
  // Определяем единицы измерения в зависимости от режима
  if(currentMode == SPEEDOMETER) {
    doc["unit"] = "km/h";
  } else {
    doc["unit"] = "s";
  }
  
  // Добавляем статус датчиков
  doc["sensor1_active"] = sensor1Active;
  doc["sensor2_active"] = sensor2Active;
  
  // Добавляем историю измерений
  JsonArray history = doc.createNestedArray("history");
  
  if(currentMode == SPEEDOMETER) {
    for(int i = 0; i < HISTORY_SIZE && i < historyIndex; i++) {
      if(speedHistory[i].value > 0) {
        JsonObject histObj = history.createNestedObject();
        histObj["value"] = speedHistory[i].value;
        histObj["timestamp"] = formatTimestamp(speedHistory[i].timestamp);
      }
    }
  } else {
    for(int i = 0; i < HISTORY_SIZE && i < historyIndex; i++) {
      if(lapHistory[i].value > 0) {
        JsonObject histObj = history.createNestedObject();
        histObj["value"] = lapHistory[i].value;
        histObj["timestamp"] = formatTimestamp(lapHistory[i].timestamp);
      }
    }
  }
  
  // Добавляем время гонки, если в соответствующем режиме
  if(currentMode == RACE_TIMER || currentMode == LAP_TIMER) {
    unsigned long long raceTime = getCurrentRaceTimeSafe();
    unsigned long long startTimeVal = getStartTimeSafe();
    bool sensor1Trig = getSensor1TriggeredSafe();
    
    if(sensor1Trig && startTimeVal > 0) {
      unsigned long long currentTime = micros();
      double elapsed = (currentTime - startTimeVal) / 1000000.0;
      doc["race_time"] = elapsed;
    }
  }
  
  // Serialize to a string and send
  String jsonString;
  serializeJson(doc, jsonString);
  server.send(200, "application/json", jsonString);
}

void handleReset() {
  resetMeasurements();
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  String modeParam = server.arg("m");
  if(modeParam != "") {
    int mode = modeParam.toInt();
    if(mode >= 0 && mode <= 2) {
      currentMode = (Mode)mode;
    }
  }
  server.send(200, "text/plain", String(currentMode));
}

void handleDistance() {
  String distanceParam = server.arg("d");
  if(distanceParam != "") {
    float newDistance = distanceParam.toFloat();
    if(newDistance > 0) {
      distance = newDistance;
    }
  }
  server.send(200, "text/plain", String(distance, 2));
}

void handleCSS() {
  if(SPIFFS.exists("/style.css")) {
    File file = SPIFFS.open("/style.css", "r");
    if(file) {
      server.streamFile(file, "text/css");
      file.close();
    } else {
      server.send(404, "text/plain", "File not found");
    }
  } else {
    // Отправляем стандартный CSS, если файл не найден
    String css = "";
    server.send(200, "text/css", css);
  }
}

void handleJS() {
  if(SPIFFS.exists("/script.js")) {
    File file = SPIFFS.open("/script.js", "r");
    if(file) {
      server.streamFile(file, "application/javascript");
      file.close();
    } else {
      server.send(404, "text/plain", "File not found");
    }
  } else {
    // Отправляем стандартный JS, если файл не найден
    String js = "";
    server.send(200, "application/javascript", js);
  }
}

void resetMeasurements() {
  // Сбрасываем текущие измерения
  currentValue = 0.0;
  startTime = 0;
  endTime = 0;
  measurementReady = false;
  measurementInProgress = false;
  
  // Сбрасываем историю
  memset(speedHistory, 0, sizeof(speedHistory));
  memset(lapHistory, 0, sizeof(lapHistory));
  historyIndex = 0;
}

void handleWiFiSettings() {
  String html = "<!DOCTYPE html><html><head><title>Настройки Wi-Fi</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<link rel=\"stylesheet\" href=\"/style.css\"></head><body>";
  html += "<div class=\"container\">";
  html += "<h2>Настройки Wi-Fi</h2>";
  html += "<form action=\"/updatewifi\" method=\"post\">";
  html += "<div class=\"form-group\"><label for=\"ssid\">SSID:</label>";
  html += "<input type=\"text\" id=\"ssid\" name=\"ssid\" value=\"" + String(ssid) + "\"></div>";
  html += "<div class=\"form-group\"><label for=\"password\">Пароль:</label>";
  html += "<input type=\"password\" id=\"password\" name=\"password\" value=\"" + String(password) + "\"></div>";
  html += "<button type=\"submit\">Сохранить</button></form>";
  html += "<a href=\"/\">Назад</a></div></body></html>";
  server.send(200, "text/html", html);
}

void handleUpdateWiFi() {
  String newSSID = server.arg("ssid");
  String newPassword = server.arg("password");
  
  if(newSSID.length() > 0 && newSSID.length() < 32) {
    strcpy(ssid, newSSID.c_str());
  }
  
  if(newPassword.length() < 64) {
    strcpy(password, newPassword.c_str());
  }
  
  saveWiFiSettings();
  
  server.sendHeader("Location", "/");
  server.send(303);
}