#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>

#include "../include/receiver_config.h"
#include "../include/ir_receiver.h"
#include "../include/web_handlers.h"

// Объявление функций
void handleUDPPackets();

// Объявление сервера, определенного в web_handlers.cpp
extern WebServer server;

WiFiUDP udp;
DNSServer dnsServer;

// Переменные для хранения данных излучателя определены в web_handlers.cpp
extern struct TransmitterTelemetry {
  int batteryLevel;
  float batteryVoltage;
  unsigned long lastUpdate;
} transmitterData;

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК приемников
  initIRReceivers();
  
  // Настройка пинов датчиков как входы для ИК приемников
  pinMode(SENSOR1_PIN, INPUT_PULLUP);
  pinMode(SENSOR2_PIN, INPUT_PULLUP);
  
  // При нормальной работе ИК луча на пинах будет LOW (есть сигнал)
  // При пересечении луча на пинах будет HIGH (нет сигнала)
  // Поэтому используем прерывание по RISING (по положительному фронту)
  attachInterrupt(digitalPinToInterrupt(SENSOR1_PIN), handleSensor1, RISING);
  attachInterrupt(digitalPinToInterrupt(SENSOR2_PIN), handleSensor2, RISING);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed. Formatting...");
  }

  // Загрузка сохраненных настроек wifi
  loadWiFiSettings();

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  // Настройка DNS для перенаправления всех запросов
  dnsServer.start(53, "*", WiFi.softAPIP());
  // Установка имени хоста
  WiFi.setHostname("chrono.mg");

  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/api/v1/data", handleData);
  server.on("/api/v1/reset", handleReset);
  server.on("/api/v1/mode", HTTP_GET, handleMode);
  server.on("/api/v1/distance", HTTP_GET, handleDistance);
  
  server.on("/", handleRoot);
  server.on("/wifisettings", handleWiFiSettings);
  server.on("/updatewifi", HTTP_POST, handleUpdateWiFi);
  server.on("/style.css", handleCSS);
  server.on("/script.js", handleJS);

  server.begin();
  
  // Инициализация UDP
  udp.begin(UDP_PORT);
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest(); // Обработка DNS запросов
  
  // Обработка UDP пакетов от излучателя
  handleUDPPackets();
  
  // Обновление состояния измерений
  processMeasurements();
  updateSensorDisplay(); // состояние датчиков
}

void handleUDPPackets() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    udp.read(incomingPacket, sizeof(incomingPacket));
    incomingPacket[packetSize] = '\0';
    
    // Парсинг телеметрии излучателя
    if (strncmp(incomingPacket, "TELEMETRY:", 10) == 0) {
      sscanf(incomingPacket, "TELEMETRY:%d:%fV", &transmitterData.batteryLevel, &transmitterData.batteryVoltage);
      transmitterData.lastUpdate = millis();
      Serial.printf("Получена телеметрия излучателя: %d%% (%.2fV)\n",
                    transmitterData.batteryLevel, transmitterData.batteryVoltage);
    }
  }
}

// Функции для получения телеметрии излучателя определены в web_handlers.cpp