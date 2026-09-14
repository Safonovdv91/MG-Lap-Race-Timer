/**
 * 
 * Обработчики побочных эффектов (Side Effects):
 * - WebSocket broadcast
 * - Serial output
 * - LED индикация
 * 
 */

#include "modules/measurements.h"
#include "modules/websocket_handlers.h"

#include <Arduino.h>


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
