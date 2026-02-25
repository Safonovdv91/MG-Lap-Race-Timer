#include "config.h" 
#include "battery/battery.h"
#include <espnow_receiver.h>

float batteryVoltage = 0.0f;
int batteryPercentage = 0;
unsigned long lastBatteryRead = 0;

int calculateBatteryPercentage(float voltage) {
  // 4.1В = 100%, 3.4В = 0%
  // Li-Ion: 4.2В=100%, 3.0В=0%
  
  // На на 3.4В как 0% ESP начинает некорректно работать, из-за
  // падения напряжения  на DC-DC преобразователе.
  
  if (voltage >= 4.10) return 100;                    // 100%
  if (voltage >= 3.95) return 90 + (voltage - 3.95) * 100;  // 90-100%
  if (voltage >= 3.85) return 75 + (voltage - 3.85) * 150;  // 75-90%
  if (voltage >= 3.75) return 50 + (voltage - 3.75) * 250;  // 50-75%
  if (voltage >= 3.65) return 25 + (voltage - 3.65) * 250;  // 25-50%
  if (voltage >= 3.55) return 10 + (voltage - 3.55) * 150;  // 10-25%
  if (voltage >= 3.45) return 5 + (voltage - 3.45) * 50;    // 5-10%
  if (voltage >= 3.35) return (voltage - 3.35) * 50;        // 0-5%
  
  return 0;
}

void readBattery() {

    if (millis() - lastBatteryRead > 10000) {
    espnow_requestBattery();
    lastBatteryRead = millis();
}
    // if (millis() - lastBatteryRead > 500) {
    //     int raw = analogRead(BATTERY_PIN);
    //     float adcVoltage = (raw / ADC_MAX_READING) * ADC_REFERENCE_VOLTAGE;

    //     batteryVoltage = adcVoltage * VOLTAGE_DIVIDER_MULTIPLIER;
    //     batteryPercentage = calculateBatteryPercentage(batteryVoltage);
        
    //     lastBatteryRead = millis();
    // }
}

float getBatteryVoltage() {
    return batteryVoltage;
}

int getBatteryPercentage() {
    return batteryPercentage;
}
