#include "web_handlers.h"
#include "config.h"
#include "measurements.h"
#include "web_content.h"

#include "battery/battery.h" 

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

WebServer server(80);

// Внешние переменные (определены в других файлах)
extern Mode currentMode;
extern Measurement speedHistory[HISTORY_SIZE];
extern Measurement lapHistory[HISTORY_SIZE];
extern int historyIndex;

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
extern TimerStatus getTimerStatus();

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
    options += "<option value=\"1\"" + String(currentMode == LAP_TIMER ? " selected" : "") + ">Lap Timer</option>";
    options += "<option value=\"2\"" + String(currentMode == RACE_TIMER ? " selected" : "") + ">Race Timer</option>";
    html.replace("{{MODE_OPTIONS}}", options);

    String battery_status = "";
    #ifdef RECEIVER_MODE
      int txBatteryLevel = getTransmitterBatteryLevel();
      battery_status += "<div class=\"battery-info tx\">TX: " + (txBatteryLevel >= 0 ? String(txBatteryLevel) + "%" : "---") + "</div>";
    #endif
    battery_status += "<div class=\"battery-info rx\">RX: " + String(getBatteryPercentage()) + "%</div>";

    html.replace("{{BATTERY_STATUS}}", battery_status);
  });
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