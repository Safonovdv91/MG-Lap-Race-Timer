#include <Arduino.h>

#include "../include/config.h"
#include "../include/measurements.h"
#include "../include/web_handlers.h"

#include <WiFi.h>
#include <DNSServer.h>

DNSServer dnsServer;

void setup() {
  Serial.begin(115200);
  
  // В зависимости от типа датчика необходимо коммутировать +3.3V
  // в большинстве случаем коммутация  происходит через + в ином расскоментировать.
  // pinMode(SENSOR1_PIN, INPUT_PULLUP);
  // pinMode(SENSOR2_PIN, INPUT_PULLUP);
  pinMode(SENSOR1_PIN, INPUT_PULLDOWN);
  pinMode(SENSOR2_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(SENSOR1_PIN), handleSensor1, FALLING);
  attachInterrupt(digitalPinToInterrupt(SENSOR2_PIN), handleSensor2, FALLING);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed. Formatting...");
  }

  // Загрузка сохраненных настроек wifi
  loadWiFiSettings();

  WiFi.softAP(ssid, password);
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

}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest(); // Обработка DNS запросов
  processMeasurements();
  updateSensorDisplay(); // состояние датчиков
}
