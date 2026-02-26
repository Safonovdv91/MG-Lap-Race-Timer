#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "transmitter_config.h"
#include "battery/battery.h"
#include "espnow_transmitter.h"

WiFiUDP udp;

// Объявления функций
void sendTelemetry();
void handleUDPPackets();

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК передатчиков(Включен постоянно)
  initIRTransmitters();
  
  // Подключение к Wi-Fi сети
  espnow_init();   // WiFi.mode(WIFI_STA) вызывается внутри

  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());
  
  // Инициализация определения заряда батареи
  initReadBattery();
  
  
}

unsigned long lastBeaconTime = 0;
bool isIRPulseOn = false;


void loop() {
  unsigned long currentTime = millis();

  // --- Battery Reading ---
  readBattery();
  
  espnow_loop();
  
  // --- UDP Handling ---
}

void sendTelemetry() {
  // Получение данных о состоянии из модуля батареи
  float battVoltage = getBatteryVoltage();
  int batteryLevel = getBatteryPercentage();
  
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