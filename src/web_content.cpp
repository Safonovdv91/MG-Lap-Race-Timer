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

String generateHTMLContent() {
  String html = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"><title>SFMgTimer</title>";
  html += "<link rel=\"stylesheet\" href=\"/style.css\"></head><body>";

  // Header for battery info
  html += "<div class=\"header\">";
  html += "<div class=\"battery-status\">";
  #ifdef RECEIVER_MODE
    int txBatteryLevel = getTransmitterBatteryLevel();
    html += "<div class=\"battery-info tx\">TX: " + (txBatteryLevel >= 0 ? String(txBatteryLevel) + "%" : "---") + "</div>";
  #endif
  html += "<div class=\"battery-info rx\">RX: " + String(batteryPercentage) + "%</div>";
  html += "</div>";

  html += "<div class=\"mode-selector-container\">";
  html += "<select id=\"mode-select\" onchange=\"changeMode(this.value)\">";
  html += "<option value=\"0\"" + String(currentMode == SPEEDOMETER ? " selected" : "") + ">Speedometer</option>";
  html += "<option value=\"1\"" + String(currentMode == LAP_TIMER ? " selected" : "") + ">Lap Timer</option>";
  html += "<option value=\"2\"" + String(currentMode == RACE_TIMER ? " selected" : "") + ">Race Timer</option>";
  html += "</select>";
  html += "</div>";

  html += "</div>";

  // Main timer container
  html += "<div class=\"timer-container\">";
  html += "<h1 class=\"timer-display\" id=\"timer-display\">00:00.000</h1>";
  html += "<p class=\"timer-subtitle\" id=\"timer-subtitle\">ready to go</p>";
  html += "</div>";

  // Controls
  html += "<div class=\"controls\">";
  html += "<button class=\"button\" onclick=\"resetMeasurements()\">Reset</button>";
  html += "</div>";

  // History
  html += "<div class=\"history\">";
  html += "<table class=\"history-table\">";
  html += "<thead><tr><th>Lap</th><th>Time</th></tr></thead>";
  html += "<tbody id=\"history-body\"></tbody>";
  html += "</table>";
  html += "</div>";

  // Footer
  html += "<div class=\"footer\">";
  html += "<a href=\"/wifisettings\">Wi-Fi Settings</a>";
  html += "</div>";

  html += "<script src=\"/script.js\"></script>";
  html += "</body></html>";

  return html;
}

String formatTimestamp(unsigned long timestamp) {
  time_t rawtime = timestamp / 1000; // Преобразуем миллисекунды в секунды
  struct tm *timeinfo = localtime(&rawtime);
  
  char buffer[9]; // HH:MM:SS + null terminator
  strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
  
  return String(buffer);
}