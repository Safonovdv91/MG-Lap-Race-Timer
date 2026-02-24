/**
 * measurement_core.h
 * 
 * Заголовочный файл ядра логики измерений.
 */

#ifndef MEASUREMENT_CORE_H
#define MEASUREMENT_CORE_H

#include <Arduino.h>
#include "config.h"
#include "receiver_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ============================================================================
// Типы данных
// ============================================================================

enum Mode { LAP_TIMER, RACE_TIMER, SPEEDOMETER };
enum TimerStatus { STATUS_READY, STATUS_RUNNING, STATUS_DISPLAY };

struct Measurement {
    float value;
    unsigned long timestamp;
};

// ============================================================================
// Глобальные переменные состояния (extern)
// ============================================================================

extern Mode currentMode;
extern volatile TimerStatus timerStatus;
extern volatile unsigned long displayStartTime;

extern Measurement speedHistory[HISTORY_SIZE];
extern Measurement lapHistory[HISTORY_SIZE];
extern int historyIndex;

extern volatile unsigned long long startTime;
extern volatile unsigned long long endTime;
extern volatile bool measurementReady;
extern volatile bool measurementInProgress;

extern volatile unsigned long long currentRaceTime;
extern volatile float currentValue;

extern volatile unsigned long lastSensorPulseTime;
extern volatile bool sensorTriggered;

// ============================================================================
// ISR Handler
// ============================================================================

void IRAM_ATTR handleSensor();

// ============================================================================
// Core Processing
// ============================================================================

/**
 * Основная функция обработки измерений.
 * Вызывается из loop() приемника.
 * 
 * Разделено на core (этот файл) и side effects (measurements.cpp)
 */
void processMeasurements();

/**
 * Сброс всех измерений.
 * Безопасно для вызова из веб-обработчиков.
 */
void resetMeasurementsCore();

// ============================================================================
// FSM Functions (internal use via processMeasurements)
// ============================================================================

void handleCooldown(unsigned long nowMs);
void startTimer(unsigned long nowUs);
void updateLiveTimer(unsigned long nowUs);
void handleBeamState(bool beamBrokenNow, unsigned long nowUs);

// ============================================================================
// History Management
// ============================================================================

void addToHistory(Measurement history[], float value);

// ============================================================================
// Safe Accessor Functions (thread-safe)
// ============================================================================

unsigned long long getStartTimeSafe();
unsigned long long getCurrentRaceTimeSafe();
float getCurrentValueSafe();
bool getMeasurementReadySafe();
bool getMeasurementInProgressSafe();
TimerStatus getTimerStatus();
unsigned long getDisplayStartTimeSafe();

// Функция для проверки и сброса флага срабатывания датчика
bool checkAndClearSensorTriggered();

// Проверка — луч потерян (не восстанавливается > 10 секунд)
bool isBeamLost();

// Critical section management
void lockMeasurements();
void unlockMeasurements();

#endif // MEASUREMENT_CORE_H
