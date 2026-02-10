#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "transmitter_config.h"
#include "battery/battery.h"

WiFiUDP udp;

// Объявления функций
void sendTelemetry();
void handleUDPPackets();

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК передатчиков(Включен постоянно)
  initIRTransmitters();
  
  // Подключение к Wi-Fi сети
  WiFi.mode(WIFI_STA); // Работаем в режиме станции
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("Wi-Fi подключен");
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Не удалось подключиться к Wi-Fi. Работа в автономном режиме.");
  }
  
  // Инициализация UDP
  udp.begin(UDP_PORT);
}

unsigned long lastBeaconTime = 0;
bool isIRPulseOn = false;


void loop() {
  unsigned long currentTime = millis();

  // --- Battery Reading ---
  readBattery();
  
  // --- Telemetry ---
  if (currentTime - lastBeaconTime > BEACON_INTERVAL) {
    sendTelemetry();
    lastBeaconTime = currentTime;
  }
  
  // --- UDP Handling ---
  handleUDPPackets();
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