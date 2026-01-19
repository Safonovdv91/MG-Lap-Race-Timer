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
unsigned long lastIRPulseTime = 0;
bool isIRPulseOn = false;

#define IR_PULSE_ON_DURATION 10  // ms
#define IR_PULSE_OFF_DURATION 40 // ms

void loop() {
  unsigned long currentTime = millis();

  // --- IR Signal Modulation ---
  if (isIRPulseOn) {
    if (currentTime - lastIRPulseTime >= IR_PULSE_ON_DURATION) {
      setIRPower(0); // Turn IR off
      isIRPulseOn = false;
      lastIRPulseTime = currentTime;
    }
  } else {
    if (currentTime - lastIRPulseTime >= IR_PULSE_OFF_DURATION) {
      setIRPower(50); // Turn IR on (50% duty cycle)
      isIRPulseOn = true;
      lastIRPulseTime = currentTime;
    }
  }
  
  // --- Telemetry ---
  if (currentTime - lastBeaconTime > BEACON_INTERVAL) {
    sendTelemetry();
    lastBeaconTime = currentTime;
  }
  
  // --- UDP Handling ---
  handleUDPPackets();
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