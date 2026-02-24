/**
 * 
 * Обработчики побочных эффектов (Side Effects):
 * - WebSocket broadcast
 * - Serial output
 * - LED индикация
 * 
 */

#include "measurements.h"
#include "websocket_handlers.h"

#include <Arduino.h>

// ============================================================================
// LED Indicators
// ============================================================================

/**
 * Управление статусным светодиодом датчика.
 * Показывает текущее состояние луча:
 * - HIGH (включен)   : луч есть (нормальное состояние)
 * - LOW (выключен)   : луч прерван
 * - Мигание (1 Гц)   : луч потерян > 10 секунд (ошибка)
 */
void handleStatusLED() {
    static unsigned long lastBlinkTime = 0;
    static bool blinkState = false;

    // Проверка на "луч потерян"
    if (isBeamLost()) {
        // Мигание 1 Гц (1000 мс)
        unsigned long nowMs = millis();
        if (nowMs - lastBlinkTime >= 500) {
            blinkState = !blinkState;
            digitalWrite(STATUS_IR_LED_PIN, blinkState ? HIGH : LOW);
            lastBlinkTime = nowMs;
        }
        return;
    }

    // Нормальный режим: показываем текущее состояние датчика
    int sensorState = digitalRead(SENSOR_PIN);
    // sensorState == HIGH → луч прерван → LED выключен
    // sensorState == LOW → луч есть → LED включен
    digitalWrite(STATUS_IR_LED_PIN, sensorState == LOW ? HIGH : LOW);
}

// ============================================================================
// WebSocket Broadcast
// ============================================================================

void handleWebsocketBroadcast(unsigned long nowMs, bool lapFinished) {
    static unsigned long lastWsBroadcastTime = 0;

    if (lapFinished ||
        timerStatus == STATUS_DISPLAY ||
        (timerStatus == STATUS_RUNNING &&
         nowMs - lastWsBroadcastTime > 100))
    {
        ws_broadcast_data();
        lastWsBroadcastTime = nowMs;
    }
}

// ============================================================================
// Serial Output (Debug)
// ============================================================================

void handleSerialOutput(unsigned long nowMs, bool lapFinished) {
    static unsigned long lastSerialPrintTime = 0;

    if (lapFinished) {
        Serial.print("Время круга: ");
        Serial.print(currentValue, 3);
        Serial.println(" с");
    }

    if (measurementInProgress &&
        (nowMs - lastSerialPrintTime > 1000))
    {
        Serial.print("Текущее время: ");
        Serial.print(currentRaceTime / 1000000.0, 3);
        Serial.println(" с");
        lastSerialPrintTime = nowMs;
    }
}

// ============================================================================
// Main Processing Wrapper
// ============================================================================

/**
 * Публичная функция обработки измерений с side effects.
 * Вызывается из loop() приемника.
 * 
 * Обёрка над processMeasurements() + side effects
 */
void processMeasurementsWithSideEffects() {
    const unsigned long nowMs = millis();

    // 1. Core logic (FSM, timer state)
    processMeasurements();

    // 2. Side effects (WebSocket, Serial)
    // Проверяем статус таймера для отправки данных
    TimerStatus status = getTimerStatus();
    bool lapFinished = (status == STATUS_DISPLAY);
    handleWebsocketBroadcast(nowMs, lapFinished);
    // handleSerialOutput(nowMs, lapFinished);
}
