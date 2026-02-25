/** 
 * Обработчики веб-запросов.
 */

#include "web_handlers.h"
#include "config.h"
#include "core/measurement_core.h"
#include "web_content.h"

#include "battery/battery.h"

#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

WebServer server(80);

// Внешние переменные (определены в других файлах)
extern Mode currentMode;

// Переменные для хранения данных излучателя (только для режима приемника)
#ifdef RECEIVER_MODE
struct TransmitterTelemetry {
  int batteryLevel = -1;
  float batteryVoltage = 0.0;
  unsigned long lastUpdate = 0;
} transmitterData;

int getTransmitterBatteryLevel() {
  if (millis() - transmitterData.lastUpdate > 10000) {
    return -1;
  }
  return transmitterData.batteryLevel;
}

float getTransmitterBatteryVoltage() {
  return transmitterData.batteryVoltage;
}
#endif

// ============================================================================
// * Все данные обновляются через WebSocket (JavaScript)
// ============================================================================

void sendHtmlFile(const char* path) {
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(500, "text/plain", "File open failed");
    }
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

// ============================================================================
// Web Handlers
// ============================================================================

void handleRoot() {
  sendHtmlFile("/index.html");
}

void handleReset() {
  resetMeasurementsCore();
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
    server.send(200, "text/css", "");
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
    server.send(200, "application/javascript", "");
  }
}

void handleWiFiSettings() {
  sendHtmlFile("/wifi_settings.html");
}

void handleUpdateWiFi() {
  String newSSID = server.arg("ssid");
  String newPassword = server.arg("password");

  if(newSSID.length() > 0 && newSSID.length() < 32) {
    strncpy(ssid, newSSID.c_str(), sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
  }

  if(newPassword.length() < 64) {
    strncpy(password, newPassword.c_str(), sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';
  }

  saveWiFiSettings();

  server.sendHeader("Location", "/");
  server.send(303);
}
