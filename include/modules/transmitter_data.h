#pragma once
#include <Arduino.h>

// ============================================================
//  transmitter_data.h — единственное место определения структуры
//  и потокобезопасных геттеров телеметрии transmitter'а.
//
//  Подключать везде где нужны данные transmitter'а:
//    - espnow_receiver.cpp  (запись)
//    - web_handlers.cpp     (чтение)
//    - websocket_handlers.cpp (чтение)
// ============================================================

struct TransmitterTelemetry {
    int           batteryLevel;    // % (0-100), -1 если нет свежих данных
    float         batteryVoltage;  // В
    unsigned long lastUpdate;      // millis() последнего обновления
    uint8_t       sourceMac[6];    // MAC отправителя
};

// ---- Инициализация — вызвать в setup() ДО espnow_init() ----
void transmitterData_init();

// ---- Запись (из ESP-NOW callback) ----
void transmitterData_set(int batteryLevel, float batteryVoltage,
                         const uint8_t *sourceMac);

// ---- Атомарный снапшот — использовать в HTTP/WebSocket handlers ----
// Возвращает копию структуры за один lock.
// snap.batteryLevel == -1 означает нет данных или данные устарели.
TransmitterTelemetry transmitterData_getSnapshot();

// ---- Геттеры — совместимы с твоим существующим кодом ----
// Используй их если не хочешь менять websocket_handlers.cpp
int   getTransmitterBatteryLevel();    // -1 если нет/устарели (> 10 сек)
float getTransmitterBatteryVoltage();
bool  transmitterData_isStale(uint32_t maxAgeMs = 10000);