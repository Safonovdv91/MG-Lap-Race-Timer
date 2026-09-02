#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "utils/transmitter_config.h"
#include "drivers/battery/battery.h"
#include "drivers/espnow_transmitter.h"

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК передатчиков(Включен постоянно)
  initIRTransmitters();
  // Инициализация LED 
  initLEDTransmitters();
  
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
  updateBatteryLed();
  
  // --- ESP-NOW ---
  espnow_loop();
  }
