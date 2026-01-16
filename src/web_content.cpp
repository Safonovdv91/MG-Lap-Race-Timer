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
  String html = "<!DOCTYPE html><html lang=\"ru\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><title>SFMgTimer</title>";
  html += "<link rel=\"stylesheet\" href=\"/style.css\"></head><body>";
  
  // Верхняя панель с информацией
  html += "<div class=\"header\">";
  html += "<h1>SFMgTimer</h1>";
  html += "<div class=\"battery-info\">ESP32: ";
  html += batteryPercentage;
  html += "%</div>";
  
  // Добавляем информацию о телеметрии излучателя (только в режиме приемника)
  #ifdef RECEIVER_MODE
  int txBatteryLevel = getTransmitterBatteryLevel();
  if(txBatteryLevel >= 0) {
    html += "<div class=\"battery-info\">TX: ";
    html += txBatteryLevel;
    html += "%</div>";
  } else {
    html += "<div class=\"battery-info\">TX: ---</div>";
  }
  #endif
  
  html += "</div>";

  // Основной контент
  html += "<div class=\"container\">";
  
  // Блок выбора режима
  html += "<div class=\"control-section\">";
  html += "<label for=\"mode-select\">Режим:</label>";
  html += "<select id=\"mode-select\" onchange=\"changeMode(this.value)\">";
  html += "<option value=\"0\"" + String(currentMode == SPEEDOMETER ? " selected" : "") + ">Спидометр</option>";
  html += "<option value=\"1\"" + String(currentMode == LAP_TIMER ? " selected" : "") + ">Секундомер круга</option>";
 html += "<option value=\"2\"" + String(currentMode == RACE_TIMER ? " selected" : "") + ">Гоночный таймер</option>";
  html += "</select>";
  
  // Блок настройки дистанции
 html += "<label for=\"distance-input\">Дистанция (м):</label>";
  html += "<input type=\"number\" id=\"distance-input\" value=\"" + String(distance, 2) + "\" step=\"0.01\" min=\"0.01\" onchange=\"changeDistance(this.value)\">";
  html += "</div>";

  // Блок отображения измерений
  html += "<div class=\"display-section\">";
  html += "<div class=\"measurement-display\" id=\"measurement-display\">";
  html += "<div class=\"value\">--.-</div>";
  html += "<div class=\"unit\">км/ч</div>"; // по умолчанию для спидометра
  html += "</div>";
  
  // Блок индикаторов датчиков
  html += "<div class=\"sensor-indicators\">";
  html += "<div class=\"sensor-indicator\" id=\"sensor1-indicator\">Датчик 1: <span class=\"status\">-</span></div>";
  html += "<div class=\"sensor-indicator\" id=\"sensor2-indicator\">Датчик 2: <span class=\"status\">-</span></div>";
  html += "</div>";
  html += "</div>";

  // Блок истории
  html += "<div class=\"history-section\">";
  html += "<h3>История измерений</h3>";
  html += "<table class=\"history-table\">";
  html += "<thead><tr><th>№</th><th>Значение</th><th>Время</th></tr></thead>";
  html += "<tbody id=\"history-body\">";
  
  // Добавляем историю в зависимости от режима
  if(currentMode == SPEEDOMETER) {
    for(int i = 0; i < HISTORY_SIZE; i++) {
      if(i < historyIndex && speedHistory[i].value > 0) {
        html += "<tr><td>" + String(i+1) + "</td><td>" + String(speedHistory[i].value, 1) + "</td><td>" + formatTimestamp(speedHistory[i].timestamp) + "</td></tr>";
      }
    }
  } else {
    for(int i = 0; i < HISTORY_SIZE; i++) {
      if(i < historyIndex && lapHistory[i].value > 0) {
        html += "<tr><td>" + String(i+1) + "</td><td>" + String(lapHistory[i].value, 3) + "</td><td>" + formatTimestamp(lapHistory[i].timestamp) + "</td></tr>";
      }
    }
  }
  
  html += "</tbody>";
  html += "</table>";
  html += "</div>";

  html += "<button onclick=\"resetMeasurements()\">Сбросить</button>";
  
  html += "<div class=\"wifisettings-link\"><a href=\"/wifisettings\">Настройки Wi-Fi</a></div>";
  
  html += "</div>";

  // Скрипт для обновления данных
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