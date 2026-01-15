// web_handlers.cpp
#include "web_handlers.h"
#include <WebServer.h>
#include "measurements.h"
#include "config.h"
#include <atomic>

WebServer server(serverPort);

extern Mode currentMode;
extern float distance;

void handleRoot();
void handleData();
void handleReset();
void handleMode();
void handleDistance();
void handleCSS();
void handleJS();


void handleRoot() {
  String html = getHTMLContent();
  server.send(200, "text/html", html);
}

void handleWiFiSettings() {
  String html = getWifiSettingsContent();

  // Замена плейсхолдера текущим SSID
  html.replace("%SSID%", ssid);
  server.send(200, "text/html", html);
}

void handleData() {

  if (currentMode == RACE_TIMER || currentMode == LAP_TIMER) {
    updateRaceTimer();
  }

  String json = "{";

  // Добавляем текущее время для RACE_TIMER
  if (currentMode == RACE_TIMER || currentMode == LAP_TIMER) {
    float raceDuration = (currentRaceTime - startTime.load()) / 1000000.0;
    json += "\"currentTime\":" + String(raceDuration, 3) + ",";
  }

  json += "\"mode\":" + String(currentMode) + ","; // Режим работы
  json += "\"currentValue\":" + String(currentValue) + ","; // текущее значение
  json += "\"distance\":" + String(distance) + ","; // данные о дистанции
  json += "\"sensor1Active\":" + String(sensor1Active ? "true" : "false") + ","; // Состояние датчика 1
  json += "\"sensor2Active\":" + String(sensor2Active ? "true" : "false") + ","; // Состояние датчика 2
  json += "\"measurementInProgress\":" + String(sensor1Triggered.load() ? "true" : "false") + ","; // Идет ли измерение
  json += "\"historyIndex\":" + String(historyIndex) + ","; // Индекс измерения
  
  json += "\"speedHistory\":[";
  for (int i = 0; i < min(historyIndex, HISTORY_SIZE); i++) {
    if (i > 0) json += ",";
    json += "{\"value\":" + String(speedHistory[i].value) + ",\"time\":" + String(speedHistory[i].timestamp) + "}";
  }
  json += "],";
  
  json += "\"lapHistory\":[";
  for (int i = 0; i < min(historyIndex, HISTORY_SIZE); i++) {
    if (i > 0) json += ",";
    json += "{\"value\":" + String(lapHistory[i].value) + ",\"time\":" + String(lapHistory[i].timestamp) + "}";
  }
  json += "],";
  json += "\"batteryVoltage\":" + String(batteryVoltage, 2) + ",";
  json += "\"batteryPercentage\":" + String(batteryPercentage);
  json += "}";
  server.send(200, "application/json", json);
}

void handleReset() {
  startTime.store(0);
  endTime.store(0);
  sensor1Triggered.store(false);
  sensor2Triggered.store(false);
  currentValue = 0.0;
  server.send(200, "text/plain", "OK");
}

void handleMode() {
  if (server.hasArg("value")) {
    currentMode = (Mode)server.arg("value").toInt();
    resetMeasurements();
  }
  server.send(200, "text/plain", "OK");
}

void handleDistance() {
  if (server.hasArg("value")) {
    distance = server.arg("value").toFloat();
  }
  server.send(200, "text/plain", "OK");
}

void handleCSS() {
  if (SPIFFS.exists("/style.css")) {
    File file = SPIFFS.open("/style.css", "r");
    if (file) {
      server.streamFile(file, "text/css");
      file.close();
      return;
    }
  }
  server.send(404, "text/plain", "CSS not found");
}

void handleJS() {
  if (SPIFFS.exists("/script.js")) {
    File file = SPIFFS.open("/script.js", "r");
    if (file) {
      server.streamFile(file, "application/javascript");
      file.close();
      return;
    }
  }
  server.send(404, "text/plain","JS not found");
}

void resetMeasurements() {
  startTime.store(0);
  endTime.store(0);
  sensor1Triggered.store(false);
  sensor2Triggered.store(false);
  measurementReady.store(false);
  currentValue = 0.0;
}

void handleUpdateWiFi() {
  if (server.method() == HTTP_POST) {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    
    // Обновляем конфигурацию
    ssid = newSSID.c_str();
    password = newPassword.c_str();
    
    // Сохраняем настройки
    saveWiFiSettings();
    // Здесь можно добавить сохранение в EEPROM или файл
    
    // Перезапускаем точку доступа с новыми параметрами
    WiFi.softAPdisconnect(true);
    WiFi.softAP(ssid, password);
    
    server.send(200, "text/plain", "WiFi settings updated. AP restarted with new credentials.");
  }
}