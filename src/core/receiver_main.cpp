#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>

#include "utils/receiver_config.h"
#include "modules/web_handlers.h"
#include "modules/websocket_handlers.h"
#include "modules/measurements.h"
#include "drivers/battery/battery.h"
#include "drivers/espnow_receiver.h"
#include "drivers/espnow_broadcast.h"
#include "modules/transmitter_data.h"
#include <utils/led_config.h>

// Объявление функций
void updateOperationLed();

// Объявление сервера, определенного в web_handlers.cpp
extern WebServer server;

DNSServer dnsServer;

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    // Детектор отключения пользователя, если телефон при отключении
    // высылает пакет об отключении
    if (event == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
        Serial.println("[WiFi] Станция отключилась от AP");
    }
}

void setup() {
  Serial.begin(115200);
  
  // Настройка пинов датчиков как входы для ИК приемников
  
  pinMode(SENSOR_PIN, INPUT);
  
  // Настройка пина светодиода режима работы
    // Инициализация LED 
  initLED();

  digitalWrite(STATUS_IR_LED_PIN, LOW);
  // При нормальной работе ИК луча на пинах будет LOW (есть сигнал)
  // При пересечении луча на пинах будет HIGH (нет сигнала)
  // Поэтому используем прерывание по RISING (по положительному фронту)
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handleSensor, FALLING);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed. Formatting...");
  }

    // Загрузка сохраненных настроек wifi
    loadWiFiSettings();
    WiFi.mode(WIFI_AP);        
    WiFi.setSleep(false); 
    
    // Используем сохранённые настройки или константы по умолчанию
    Serial.printf("[WiFi] Создаём AP: SSID=%s, Password=%s\n", ssid, password);
    WiFi.softAP(ssid, password);
  
    Serial.print("Receiver MAC:");
    Serial.println(WiFi.softAPmacAddress());
    
    // Настройка DNS для перенаправления всех запросов
    dnsServer.start(53, "*", WiFi.softAPIP());
    // Установка имени хоста
    WiFi.setHostname("chrono.mg");

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // server.on("/api/v1/data", handleData); // Replaced by WebSockets
    server.on("/api/v1/reset", handleReset);
    server.on("/api/v1/mode", HTTP_GET, handleMode);
    
    server.on("/", handleRoot);
    server.on("/wifisettings", handleWiFiSettings);
    server.on("/updatewifi", HTTP_POST, handleUpdateWiFi);
    server.on("/style.css", handleCSS);
    server.on("/script.js", handleJS);
    
    // API для настроек Wi-Fi
    server.on("/api/v1/wifi/settings", HTTP_GET, handleGetWifiSettings);
    server.on("/api/v1/wifi/update", HTTP_POST, handleUpdateWifiPassword);

    server.onNotFound([]() {
        server.send(404, "text/plain", "Not found");
    });
  
    // Инициализация WebSocket
    ws_init();
    ws_start_task();   // вместо server.begin() отдельного вызова ws_loop в основном loop
    server.begin();
    WiFi.onEvent(onWiFiEvent);

    // Инициализация определения заряда батареи
    initReadBattery();
  
    Serial.println("Server is running!");
  
    //инициализция esp-now

    transmitterData_init();  // ← добавить ДО espnow_init()
    espnow_init();

    // инициализация broadcast
    espnow_broadcast_init();
    espnow_broadcast_setInterval(90); // 11 раза в секунду общая рассылка
}

void loop() {
    esp_task_wdt_reset(); 
    
    // ws_loop(); // Обработка WebSocket
    server.handleClient();
    dnsServer.processNextRequest(); // Обработка DNS запросов

    yield();
    // Обработка ESP-NOW пакетов от излучателя
    espnow_loop();

    // Обновление состояния измерений (core + side effects)
    processMeasurementsWithSideEffects();
    
    // Обновление светодиода режима работы



    // Определение заряда батареи
    readBattery();
    
    // Запрос статуса заряда у передатчика.
    handleTxReadBattery();

    // Broadcast данных
    espnow_broadcast_loop();

    // Определение цвета свтодиода
    updateBatteryLed();
    updateOperationLed();
    handleStatusLED();

    applyLedOutputs();     // световая индикация в зависимости от сценария

}

