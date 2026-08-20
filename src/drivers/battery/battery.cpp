// Для тестов: если определено UNIT_TEST, используем моки вместо Arduino.h
#ifdef UNIT_TEST
#include "fixtures/mocks/arduino_mocks.h"
#else
#include <Arduino.h>
#endif

#include "utils/config.h"
#include "drivers/battery/battery.h"
#ifndef UNIT_TEST
#include "drivers/espnow_receiver.h"
#endif

float batteryVoltage = 0.0f;
int batteryPercentage = 0;

static unsigned long lastSampleTime = 0;

// EMA
static float emaVoltage = 0.0f;
const float EMA_ALPHA = 0.1f;   // 0.05 плавнее, 0.1 оптимально, 0.2 быстрее

// ===== Инициализация аккумуляторометра =====
void initReadBattery(){
    Serial.printf("Инициализация определения заряда батареи [Battery] [PIN]:= %d ",BATTERY_PIN);
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
    analogReadResolution(12); // фиксируем 12 бит 0..4095
}

// ===== Расчёт процентов =====
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


// ===== Чтение батареи =====
void readBattery() {

    unsigned long now = millis();

    // раз в 2 секунды
    if (now - lastSampleTime >= 2000) {
        lastSampleTime = now;

        // ESP32 ADC особенность( выбрасываем первое значение)
        analogRead(BATTERY_PIN);
        int raw = analogRead(BATTERY_PIN);

        float adcVoltage = (raw / ADC_MAX_READING) * ADC_REFERENCE_VOLTAGE;
        float newVoltage = adcVoltage * VOLTAGE_DIVIDER_MULTIPLIER;

        // ===== EMA =====
        if (emaVoltage == 0.0f) {
            emaVoltage = newVoltage;      // первый запуск
        } else {
            emaVoltage = emaVoltage + EMA_ALPHA * (newVoltage - emaVoltage);
        }

        batteryVoltage = emaVoltage * 3.7;
        batteryPercentage = calculateBatteryPercentage(batteryVoltage);
    }
}


// ===== Геттеры =====
float getBatteryVoltage() {
    return batteryVoltage;
}

int getBatteryPercentage() {
    return batteryPercentage;
}