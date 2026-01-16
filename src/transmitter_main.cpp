#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "../include/transmitter_config.h"
#include "../include/ir_transmitter.h"

WiFiUDP udp;

// Объявления функций
void sendTelemetry();
void handleUDPPackets();

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК передатчиков
  initIRTransmitters();
  
  // Подключение к Wi-Fi сети
  WiFi.mode(WIFI_STA); // Работаем в режиме станции
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Wi-Fi подключен");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
  
  // Инициализация UDP
  udp.begin(UDP_PORT);
}

unsigned long lastBeaconTime = 0;

void loop() {
  unsigned long currentTime = millis();
  
  // Отправка beacon сообщения каждые BEACON_INTERVAL миллисекунд
  if (currentTime - lastBeaconTime > BEACON_INTERVAL) {
    sendTelemetry();
    lastBeaconTime = currentTime;
  }
  
  // Обработка UDP пакетов (опционально)
  handleUDPPackets();
  
  delay(10);
}

void sendTelemetry() {
  // Получение данных о состоянии
  float battVoltage = analogRead(BATTERY_PIN) * 3.3 / 4095.0 * 2.0; // Предполагаем делитель 2:1
  int batteryLevel = map(battVoltage * 100, BATTERY_MIN_V * 100, BATTERY_MAX_V * 100, 0, 100);
  if (batteryLevel > 100) batteryLevel = 100;
  if (batteryLevel < 0) batteryLevel = 0;
  
  // Формирование и отправка UDP пакета
  IPAddress receiverIP(192, 168, 4, 1); // IP адрес по умолчанию для AP режима
  udp.beginPacket(receiverIP, UDP_PORT);
  udp.printf("TELEMETRY:%d:%.2fV", batteryLevel, battVoltage);
  udp.endPacket();
}

void handleUDPPackets() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    udp.read(incomingPacket, sizeof(incomingPacket));
    incomingPacket[packetSize] = '\0';
    
    // Обработка входящих команд (опционально)
    Serial.printf("Получено UDP сообщение: %s\n", incomingPacket);
  }
}