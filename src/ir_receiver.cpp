#include "../include/ir_receiver.h"
#include <Arduino.h>

// Инициализация ИК приемников (на самом деле просто настройка пинов как входы)
void initIRReceivers() {
#ifdef RECEIVER_MODE
    // SENSOR1_PIN и SENSOR2_PIN уже настраиваются в основном коде
    // ИК приемники TSOP38238 подключаются к тем же пинам
    // Настройка как входы с подтяжкой (если используется)
    pinMode(SENSOR1_PIN, INPUT_PULLUP);
    pinMode(SENSOR2_PIN, INPUT_PULLUP);
#endif
}

// Проверка, заблокирован ли ИК луч 1
// Возвращается true, если луч пересечен (сигнал заблокирован)
bool isIRBeamBlocked1() {
#ifdef RECEIVER_MODE
    // Для TSOP38238: при наличии сигнала на выходе LOW, при отсутствии - HIGH
    // Когда луч НЕ пересечен (сигнал есть): на пине LOW
    // Когда луч пересечен (сигнал заблокирован): на пине HIGH
    // Чтобы узнать, что луч заблокирован, нужно проверить на HIGH
    return digitalRead(SENSOR1_PIN) == HIGH;
#else
    return false; // В режиме передатчика всегда возвращаем false
#endif
}

// Проверка, заблокирован ли ИК луч 2
// Возвращается true, если луч пересечен (сигнал заблокирован)
bool isIRBeamBlocked2() {
#ifdef RECEIVER_MODE
    // Для TSOP38238: при наличии сигнала на выходе LOW, при отсутствии - HIGH
    // Когда луч НЕ пересечен (сигнал есть): на пине LOW
    // Когда луч пересечен (сигнал заблокирован): на пине HIGH
    // Чтобы узнать, что луч заблокирован, нужно проверить на HIGH
    return digitalRead(SENSOR2_PIN) == HIGH;
#else
    return false; // В режиме передатчика всегда возвращаем false
#endif
}

// Получение статуса сигнала ИК луча 1 (для диагностики)
bool getIRSignalStatus1() {
#ifdef RECEIVER_MODE
    // LOW означает наличие сигнала, HIGH - отсутствие (луч заблокирован)
    return digitalRead(SENSOR1_PIN) == LOW;
#else
    return false; // В режиме передатчика всегда возвращаем false
#endif
}

// Получение статуса сигнала ИК луча 2 (для диагностики)
bool getIRSignalStatus2() {
#ifdef RECEIVER_MODE
    // LOW означает наличие сигнала, HIGH - отсутствие (луч заблокирован)
    return digitalRead(SENSOR2_PIN) == LOW;
#else
    return false; // В режиме передатчика всегда возвращаем false
#endif
}