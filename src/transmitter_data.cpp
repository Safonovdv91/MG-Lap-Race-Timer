// ============================================================
//  transmitter_data.cpp — потокобезопасное хранилище телеметрии
// ============================================================

#include "transmitter_data.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static TransmitterTelemetry s_data  = {-1, 0.0f, 0, {0}};
static SemaphoreHandle_t    s_mutex = nullptr;

static inline bool lock()   { return xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) == pdTRUE; }
static inline void unlock() { xSemaphoreGive(s_mutex); }

// ============================================================

void transmitterData_init() {
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) Serial.println("[TxData] ОШИБКА: не удалось создать мьютекс!");
}

void transmitterData_set(int batteryLevel, float batteryVoltage,
                         const uint8_t *sourceMac) {
    if (!lock()) { Serial.println("[TxData] Таймаут мьютекса при записи!"); return; }
    s_data.batteryLevel   = batteryLevel;
    s_data.batteryVoltage = batteryVoltage;
    s_data.lastUpdate     = millis();
    if (sourceMac) memcpy(s_data.sourceMac, sourceMac, 6);
    unlock();
}

bool transmitterData_isStale(uint32_t maxAgeMs) {
    // Вызывать только внутри lock или через геттеры
    if (s_data.lastUpdate == 0) return true;
    return (millis() - s_data.lastUpdate) > maxAgeMs;
}

// Атомарный снапшот — читаем всю структуру за один lock
TransmitterTelemetry transmitterData_getSnapshot() {
    TransmitterTelemetry snap = {-1, 0.0f, 0, {0}};
    if (!lock()) return snap;
    snap = s_data;
    if (transmitterData_isStale()) snap.batteryLevel = -1;
    unlock();
    return snap;
}

// ---- Геттеры — совместимы с существующим кодом ----

int getTransmitterBatteryLevel() {
    if (!lock()) return -1;
    int val = transmitterData_isStale() ? -1 : s_data.batteryLevel;
    unlock();
    return val;
}

float getTransmitterBatteryVoltage() {
    if (!lock()) return 0.0f;
    // Напряжение возвращаем даже если данные устарели — 
    // последнее известное значение лучше чем 0.0
    float val = s_data.batteryVoltage;
    unlock();
    return val;
}