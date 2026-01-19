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
extern unsigned long long getCurrentRaceTimeSafe();
extern bool getMeasurementReadySafe();
extern bool getMeasurementInProgressSafe();

// Helper function to send HTML file with placeholder replacement
void sendHtml(String path, std::function<void(String&)> replacer) {
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    if (file) {
      String html = file.readString();
      file.close();
      replacer(html);
      server.send(200, "text/html", html);
    } else {
      server.send(500, "text/plain", "File open failed");
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

void handleRoot() {
  sendHtml("/index.html", [](String& html) {
    String options = "";
    options += "<option value=\"0\"" + String(currentMode == SPEEDOMETER ? " selected" : "") + ">Speedometer</option>";
    options += "<option value=\"1\"" + String(currentMode == LAP_TIMER ? " selected" : "") + ">Lap Timer</option>";
    options += "<option value=\"2\"" + String(currentMode == RACE_TIMER ? " selected" : "") + ">Race Timer</option>";
    html.replace("{{MODE_OPTIONS}}", options);

    String battery_status = "";
    #ifdef RECEIVER_MODE
      int txBatteryLevel = getTransmitterBatteryLevel();
      battery_status += "<div class=\"battery-info tx\">TX: " + (txBatteryLevel >= 0 ? String(txBatteryLevel) + "%" : "---") + "</div>";
    #endif
    battery_status += "<div class=\"battery-info rx\">RX: " + String(batteryPercentage) + "%</div>";
    html.replace("{{BATTERY_STATUS}}", battery_status);
  });
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
  if (currentMode == RACE_TIMER || currentMode == LAP_TIMER) {
    if (getMeasurementInProgressSafe()) {
      doc["race_time"] = getCurrentRaceTimeSafe() / 1000000.0;
    } else {
      doc["race_time"] = 0;
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
  sendHtml("/wifi_settings.html", [](String& html) {
    html.replace("{{WIFI_SSID}}", String(ssid));
    html.replace("{{WIFI_PASSWORD}}", String(password));
  });
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