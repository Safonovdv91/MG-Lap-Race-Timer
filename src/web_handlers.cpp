// web_handlers.cpp
#include "web_handlers.h"
#include <WebServer.h>
#include "measurements.h"
#include "config.h"

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
    unsigned long long startTimeVal = getStartTimeSafe();
    unsigned long long currentTimeVal = getCurrentRaceTimeSafe();
    float raceDuration = 0.0;
    if (currentTimeVal >= startTimeVal && startTimeVal != 0) {
      raceDuration = (currentTimeVal - startTimeVal) / 1000000.0;
    }
    json += "\"currentTime\":" + String(raceDuration, 3) + ",";
  }

  json += "\"mode\":" + String(currentMode) + ","; // Режим работы
  json += "\"currentValue\":" + String(currentValue) + ","; // текущее значение
  json += "\"distance\":" + String(distance) + ","; // данные о дистанции
  json += "\"sensor1Active\":" + String(sensor1Active ? "true" : "false") + ","; // Состояние датчика 1
  json += "\"sensor2Active\":" + String(sensor2Active ? "true" : "false") + ","; // Состояние датчика 2
  json += "\"measurementInProgress\":" + String(getSensor1TriggeredSafe() ? "true" : "false") + ","; // Идет ли измерение
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
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);
  startTime = 0;
 endTime = 0;
  currentRaceTime = 0; // Добавляем сброс таймера гонки
  sensor1Triggered = false;
  sensor2Triggered = false;
  taskEXIT_CRITICAL(&mux);
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
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);
  startTime = 0;
  endTime = 0;
  currentRaceTime = 0; // Добавляем сброс таймера гонки
  sensor1Triggered = false;
  sensor2Triggered = false;
  measurementReady = false;
  measurementInProgress = false;
  taskEXIT_CRITICAL(&mux);
  currentValue = 0.0;
}

void handleUpdateWiFi() {
  if (server.method() == HTTP_POST) {
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    
    // Обновляем конфигурацию
    strncpy(ssid, newSSID.c_str(), sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';  // Гарантируем терминатор
    strncpy(password, newPassword.c_str(), sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';  // Гарантируем терминатор
    
    // Сохраняем настройки
    saveWiFiSettings();
    // Здесь можно добавить сохранение в EEPROM или файл
    
    // Перезапускаем точку доступа с новыми параметрами
    WiFi.softAPdisconnect(true);
    WiFi.softAP(ssid, password);
    
    server.send(200, "text/plain", "WiFi settings updated. AP restarted with new credentials.");
  }
}