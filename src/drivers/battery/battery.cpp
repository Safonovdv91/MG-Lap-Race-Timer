// Для тестов: если определено UNIT_TEST, используем моки вместо Arduino.h
#ifdef UNIT_TEST
#include "fixtures/mocks/arduino_mocks.h"
#else
#include <Arduino.h>
#endif

#include "utils/config.h"
#include "drivers/battery/battery.h"
#include <utils/transmitter_config.h>

#ifndef UNIT_TEST
#include "drivers/espnow_receiver.h"
#endif

float batteryVoltage = 0.0f;
int batteryPercentage = 0;

static unsigned long lastSampleTime = 0;

// EMA
static float emaVoltage = 0.0f;
const float EMA_ALPHA = 0.1f;   // 0.05 плавнее, 0.1 оптимально, 0.2 быстрее

// ===== Настройки для реализации моргания =====

unsigned long lastBlinkChange = 0;
bool ledIsOn = false;

// ===== Настройки heartbeat =====

unsigned long lastHeartbeatChange = 0;
bool heartbeatIsOn = false;

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

// ===== Обновление индикации батареи + heartbeat (вызывать каждый loop()) =====
void updateBatteryLed() {
    unsigned long now = millis();

    enum BlinkColor { NONE, RED, YELLOW, BLUE,VIOLET, ORANGE };
    BlinkColor color;

    if (batteryPercentage < 10) {
        color = RED;
    } else if (batteryPercentage < 20) {
        color = YELLOW;
    } else if (batteryPercentage < 30) {
        color = VIOLET;
    } else if (batteryPercentage < 50) {
        color = ORANGE;
    } else {
        color = NONE;
    }

    if (color == NONE) {
        // ===== Батарея в норме — гасим RED/GREEN, синий отдаём под heartbeat =====
        analogWrite(RED_LED_PIN, 0);
        analogWrite(GREEN_LED_PIN, 0);

        // сбрасываем состояние батарейного моргания, чтобы при разрядке цикл начинался заново
        ledIsOn = false;
        lastBlinkChange = now;

        // --- Heartbeat на BLUE_LED_PIN ---
        unsigned long hbElapsed = now - lastHeartbeatChange;
        if (heartbeatIsOn) {
            if (hbElapsed >= HEARTBEAT_ON_TIME) {
                analogWrite(BLUE_LED_PIN, LOW);
                heartbeatIsOn = false;
                lastHeartbeatChange = now;
            }
        } else {
            if (hbElapsed >= HEARTBEAT_OFF_TIME) {
                analogWrite(BLUE_LED_PIN, 255);
                heartbeatIsOn = true;
                lastHeartbeatChange = now;
            }
        }
        return;
    }

    // ===== Батарея разряжена — приоритет у предупреждения, heartbeat отключаем =====
    heartbeatIsOn = false;
    lastHeartbeatChange = now; // чтобы heartbeat начинался заново после возврата в норму

    unsigned long elapsed = now - lastBlinkChange;

    if (ledIsOn) {
        if (elapsed >= BLINK_ON_TIME) {
            analogWrite(RED_LED_PIN, 0);
            analogWrite(GREEN_LED_PIN, 0);
            analogWrite(BLUE_LED_PIN, 0);
            ledIsOn = false;
            lastBlinkChange = now;
        }
    } else {
        if (elapsed >= BLINK_OFF_TIME) {
            switch (color) {
                case RED:
                    analogWrite(RED_LED_PIN, 255);
                    break;
                case YELLOW:
                    analogWrite(RED_LED_PIN, 255);
                    analogWrite(GREEN_LED_PIN, 255);
                    break;
                case VIOLET:
                    analogWrite(BLUE_LED_PIN, 255);
                    analogWrite(RED_LED_PIN, 255);
                    break;
                case ORANGE:
                      analogWrite(RED_LED_PIN, 255);
                    analogWrite(GREEN_LED_PIN, 140);
                    break;
                default:
                    break;
            }
            ledIsOn = true;
            lastBlinkChange = now;
        }
    }
}
// ===== Геттеры =====
float getBatteryVoltage() {
    return batteryVoltage;
}

int getBatteryPercentage() {
    return batteryPercentage;
}