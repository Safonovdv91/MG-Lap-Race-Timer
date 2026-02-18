#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>

#include "receiver_config.h"
#include "ir_receiver.h"
#include "web_handlers.h"
#include "websocket_handlers.h"
#include "measurements.h"

// Объявление функций
void handleUDPPackets();
void updateOperationLed();

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
  
  // Настройка пинов датчиков как входы для ИК приемников
  pinMode(SENSOR1_PIN, INPUT);
  
  // Настройка пина светодиода режима работы
  pinMode(OPERATION_MODE_LED_PIN, OUTPUT);
  pinMode(STATUS_IR_LED_PIN, OUTPUT);

  digitalWrite(STATUS_IR_LED_PIN, LOW);
  // При нормальной работе ИК луча на пинах будет LOW (есть сигнал)
  // При пересечении луча на пинах будет HIGH (нет сигнала)
  // Поэтому используем прерывание по RISING (по положительному фронту)
  attachInterrupt(digitalPinToInterrupt(SENSOR1_PIN), handleSensor1, RISING);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed. Formatting...");
  }

  // Загрузка сохраненных настроек wifi
  loadWiFiSettings();

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
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

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
  
  // Инициализация WebSocket
  ws_init();

  // Инициализация UDP
  udp.begin(UDP_PORT);
}

void loop() {
  ws_loop(); // Обработка WebSocket
  server.handleClient();
  dnsServer.processNextRequest(); // Обработка DNS запросов
  
  // Обработка UDP пакетов от излучателя
  handleUDPPackets();
  
  // Обновление состояния измерений
  processMeasurements();

  // Обновление светодиода режима работы
  updateOperationLed();

  handleStatusLED();
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

void updateOperationLed() {
    static unsigned long lastBlinkTime = 0;
    static bool ledState = false; // false = OFF, true = ON
    static TimerStatus lastStatus = STATUS_READY;

    TimerStatus currentStatus = getTimerStatus();

    // Reset state machine on status change
    if (currentStatus != lastStatus) {
        lastBlinkTime = 0; // Reset blink timer
        ledState = false; // Default to OFF
        digitalWrite(OPERATION_MODE_LED_PIN, LOW);
        lastStatus = currentStatus;
    }

    unsigned long currentTime = millis();

    switch (currentStatus) {
        case STATUS_READY:
            // Normal blink: 0.3s on, 1.7s off
            if (ledState && (currentTime - lastBlinkTime >= LED_BLINK_DURATION)) {
                ledState = false;
                digitalWrite(OPERATION_MODE_LED_PIN, LOW);
                lastBlinkTime = currentTime;
            } else if (!ledState && (currentTime - lastBlinkTime >= (unsigned long)LED_BLINK_INTERVAL - LED_BLINK_DURATION)) {
                ledState = true;
                digitalWrite(OPERATION_MODE_LED_PIN, HIGH);
                lastBlinkTime = currentTime;
            }
            break;

        case STATUS_RUNNING:
            // Fast blink for the first second (MIN_LAP_TIME is in microseconds)
            if (getCurrentRaceTimeSafe() < MIN_LAP_TIME) {
                if (currentTime - lastBlinkTime >= FAST_BLINK_INTERVAL) {
                    ledState = !ledState;
                    digitalWrite(OPERATION_MODE_LED_PIN, ledState ? HIGH : LOW);
                    lastBlinkTime = currentTime;
                }
            } else {
                // After the first second, keep the LED solid ON to indicate the timer is running.
                if (!ledState) {
                    digitalWrite(OPERATION_MODE_LED_PIN, HIGH);
                    ledState = true;
                }
            }
            break;

        case STATUS_DISPLAY:
                 // Rapid blink for MIN_LAP_TIME duration, then solid ON for the rest of TIMER_COOLDOWN_PERIOD
        if (currentTime - getDisplayStartTimeSafe() < (MIN_LAP_TIME / 1000)) { // MIN_LAP_TIME is in microseconds, convert to milliseconds 
          if (currentTime - lastBlinkTime >= FAST_BLINK_INTERVAL) {
            ledState = !ledState;
            digitalWrite(OPERATION_MODE_LED_PIN, ledState ? HIGH : LOW);
                lastBlinkTime = currentTime;
            }
          } else { 
            // After MIN_LAP_TIME duration, keep the LED solid ON.                                                                                │
            if (!ledState) {
            digitalWrite(OPERATION_MODE_LED_PIN, HIGH);
            ledState = true;
          }
        }
        break;
    }

}