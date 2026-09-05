/**
 * measurement_core.cpp
 *
 * Ядро логики измерений — без побочных эффектов (Serial, WebSocket, LED).
 * Содержит только FSM, обработку датчиков и управление состоянием таймера.
 *
 * P0.4: Single Responsibility Principle — только бизнес-логика
 */

// Для тестов: если определено UNIT_TEST, используем моки вместо Arduino.h
#ifdef UNIT_TEST
#include "fixtures/mocks/arduino_mocks.h"
#else
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

#include "core/measurement_core.h"
#include "utils/config.h"
#include "utils/receiver_config.h"

// Критическая секция для защиты общих данных
static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ============================================================================
// Глобальное состояние (защищено timerMux)
// ============================================================================

Mode currentMode = LAP_TIMER;
volatile TimerStatus timerStatus = STATUS_READY;
volatile unsigned long displayStartTime = 0;

Measurement speedHistory[HISTORY_SIZE];
Measurement lapHistory[HISTORY_SIZE];
int historyIndex = 0;

volatile unsigned long long startTime = 0;
volatile unsigned long long endTime = 0;
volatile bool measurementReady = false;
volatile bool measurementInProgress = false;

volatile unsigned long long currentRaceTime = 0;
volatile float currentValue = 0.0;
volatile float lastBeamBrokenTime;
volatile float lastBeamRestoredTime;

// Состояние луча
static bool beamBrokenStable = false;      // Луч стабильно прерван (>BEAM_BREAK_THRESHOLD)
static bool beamRestoredStable = true;     // Луч стабильно восстановлен (>RESTORE_STABLE_TIME_US)
static unsigned long beamRestoreStartTime = 0;
static unsigned long beamLostStartTime = 0; // Время когда луч пропал и не возвращается
static unsigned long beamRestoredAt = 0;    // Время когда луч стабильно восстановился

#define RESTORE_STABLE_TIME_US 200000   // 200 мс стабильного восстановления (защита от дребезга)
#define BEAM_LOST_TIMEOUT_US 10000000   // 10 секунд - индикатор "луч потерян"
#define MIN_BEAM_RESTORED_TIME_US 2000000  // 2 секунды - минимальное время восстановления перед следующим пересечением

// Переменная времени последнего импульса датчика (мкс)
volatile unsigned long lastSensorPulseTime = 0;

// Флаг нового пересечения луча (устанавливается в ISR)
volatile bool sensorTriggered = false;

// ============================================================================
// ISR Handler
// ============================================================================

void IRAM_ATTR handleSensor() {
  // Минимальная логика в ISR — только установка флага и времени
  portENTER_CRITICAL_ISR(&timerMux);
  lastSensorPulseTime = micros();
  sensorTriggered = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

// ============================================================================
// Core FSM Functions
// ============================================================================

void handleCooldown(unsigned long nowMs) {
    // Куллдаун на запуск нового таймера
    // Переход в READY только если луч восстановлен
    if (timerStatus == STATUS_DISPLAY &&
        (nowMs - displayStartTime > TIMER_COOLDOWN_PERIOD) &&
        beamRestoredStable)
    {
        Serial.printf("[%7lu] [COOLDOWN] Ready. beamRestoredStable=%d\n", nowMs, beamRestoredStable);
        timerStatus = STATUS_READY;
        measurementInProgress = false;
    }
}

void startTimer(unsigned long nowUs) {
    startTime = nowUs;
    measurementInProgress = true;
    timerStatus = STATUS_RUNNING;
    currentRaceTime = 0;
    beamRestoredAt = 0;           // Сбрасываем время восстановления
    displayStartTime = 0;         // Сбрасываем время начала отображения
    beamLostStartTime = 0;        // Сбрасываем таймер потери луча
    
    Serial.printf("[%7lu] [START] Timer started. nowUs=%lu\n", millis(), nowUs);
}

void updateLiveTimer(unsigned long nowUs) {
    if (timerStatus == STATUS_RUNNING) {
        currentRaceTime = nowUs - startTime;
    } else if (timerStatus == STATUS_DISPLAY) {
        currentRaceTime = endTime - startTime;
    } else {
        currentRaceTime = 0;
    }
}

void handleBeamState(bool beamBrokenNow, unsigned long nowUs) {
    // ========================================================================
    // FSM логика:
    // 1. Луч прерван (первое пересечение) → СТАРТ таймера
    // 2. Луч есть (восстановлен > 2 с и есть луч) → ждём повторного пересечения
    // 3. Луч прерван (второе пересечение) → СТОП таймера (если > MIN_LAP_TIME)
    // 4. Луч есть → переход в DISPLAY (показ результата)
    // todo: в данный момент луч считается восстановленным практически сразу,
    // на работоспособность не влияет, но можно логику доделать.
    // ========================================================================

    // ---- Луч сейчас прерван (ISR срабатывает, разница < BEAM_BREAK_THRESHOLD) ----
    if (beamBrokenNow) {
        // Отслеживаем время когда луч пропал и не возвращается
        if (beamLostStartTime == 0) {
            beamLostStartTime = nowUs;
        }

        // Если луч был стабильно восстановлен — это ПЕРВОЕ или ПОВТОРНОЕ пересечение
        if (beamRestoredStable) {
            // ПРОВЕРКА: луч должен быть восстановлен хотя бы MIN_BEAM_RESTORED_TIME_US
            // перед тем как следующее пересечение считается валидным
            bool canStopTimer = (beamRestoredAt > 0) &&
                                ((nowUs - beamRestoredAt) > MIN_BEAM_RESTORED_TIME_US);

            beamBrokenStable = true;
            beamRestoredStable = false;
            beamRestoreStartTime = 0;

            if (timerStatus == STATUS_READY) {
                // ПЕРВОЕ пересечение → ЗАПУСК таймера
                startTimer(nowUs);
            } else if (timerStatus == STATUS_RUNNING && canStopTimer) {
                // ПОВТОРНОЕ пересечение → ПРОВЕРКА на остановку
                if (nowUs - startTime > MIN_LAP_TIME) {
                    // Валидное пересечение → СРАЗУ останавливаем таймер
                    endTime = nowUs;
                    measurementReady = true;

                    // Сохраняем результат в историю
                    const unsigned long long duration = endTime - startTime;
                    currentValue = duration / 1000000.0;
                    addToHistory(lapHistory, currentValue);

                    // Переход в режим отображения
                    timerStatus = STATUS_DISPLAY;
                    displayStartTime = millis();
                    measurementReady = false;
                }
                // Если < MIN_LAP_TIME — игнорируем (ложное срабатывание), таймер продолжает идти
            }
            // Если STATUS_DISPLAY — игнорируем (режим отображения)
        }
        // Если beamRestoredStable == false — луч ещё не восстановился после предыдущего пересечения,
        // таймер продолжает идти (ничего не делаем)
    }
    // ---- Луч восстановился (ISR не срабатывает, разница > BEAM_BREAK_THRESHOLD) ----
    else {
        // Сбрасываем таймер "луч потерян"
        beamLostStartTime = 0;

        // Начинаем отсчёт времени восстановления ТОЛЬКО если луч был прерван
        if (beamBrokenStable && !beamRestoredStable) {
            if (beamRestoreStartTime == 0) {
                beamRestoreStartTime = nowUs;
            }

            // Если луч стабильно восстановлен > RESTORE_STABLE_TIME_US (200 мс)
            if (nowUs - beamRestoreStartTime > RESTORE_STABLE_TIME_US) {
                beamRestoredStable = true;
                beamBrokenStable = false;
                beamRestoreStartTime = 0;
                beamRestoredAt = nowUs;  // Запоминаем время восстановления
            }
        }
        // Если beamBrokenStable == false — луч уже восстановлен, ничего не делаем
    }
}

// ============================================================================
// History Management
// ============================================================================

void addToHistory(Measurement history[], float value) {
    // Эта функция должна вызываться внутри critical section
    if (historyIndex < HISTORY_SIZE) {
        history[historyIndex++] = {value, millis()};
    } else {
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            history[i] = history[i + 1];
        }
        history[HISTORY_SIZE - 1] = {value, millis()};
    }
}

void resetMeasurementsCore() {
    // Сброс в критической секции
    portENTER_CRITICAL(&timerMux);
    
    currentValue = 0.0;
    startTime = 0;
    endTime = 0;
    measurementReady = false;
    measurementInProgress = false;
    
    portEXIT_CRITICAL(&timerMux);

    // Сброс истории (не требует critical section)
    memset(speedHistory, 0, sizeof(speedHistory));
    memset(lapHistory, 0, sizeof(lapHistory));
    historyIndex = 0;
}

// ============================================================================
// Main Processing Function
// ============================================================================

void processMeasurements() {
    // =========================
    // 1 — Атомарное чтение из ISR
    // =========================

    const unsigned long nowMs = millis();
    const unsigned long nowUs = micros();

    unsigned long lastPulseTime;
    bool sensorTriggeredNow;

    portENTER_CRITICAL(&timerMux);
    lastPulseTime = lastSensorPulseTime;
    sensorTriggeredNow = sensorTriggered;
    sensorTriggered = false;  // Сброс флага
    portEXIT_CRITICAL(&timerMux);

    // Определение состояния луча (debounce ~4ms)
    // ВАЖНО: Логика инвертирована!
    // - Луч ПРЕРВАН → ISR срабатывает → lastPulseTime свежий → разница < BEAM_BREAK_THRESHOLD
    // - Луч ЕСТЬ → ISR не срабатывает → lastPulseTime старый → разница > BEAM_BREAK_THRESHOLD
    const bool beamBrokenNow = (nowUs - lastPulseTime) <= BEAM_BREAK_THRESHOLD;

    // =========================
    // 2 — FSM
    // Вызываем handleBeamState ВНЕ critical section для избежания WDT
    // =========================
    
    // Сохраняем старые значения для отладки
    TimerStatus oldStatus = timerStatus;
    bool oldRestoredStable = beamRestoredStable;
    unsigned long oldRestoredAt = beamRestoredAt;
    
    handleCooldown(nowMs);
    handleBeamState(beamBrokenNow, nowUs);

    // =========================
    // 2a — Защита переменных в critical section
    // =========================
    portENTER_CRITICAL(&timerMux);
    updateLiveTimer(nowUs);
    portEXIT_CRITICAL(&timerMux);

    // =========================
    // 3 — Отладочный вывод (ВНЕ критической секции)
    // =========================

    // Отслеживаем изменение статуса таймера
    if (timerStatus != oldStatus) {
        Serial.printf("[%7lu] [FSM] Status: %d → %d\n", nowMs, oldStatus, timerStatus);
    }
    
    // Отслеживаем запуск таймера
    if (timerStatus == STATUS_RUNNING && oldStatus == STATUS_READY) {
        Serial.printf("[%7lu] [FSM] >>> START TIMER <<<\n", nowMs);
    }
    
    // Отслеживаем остановку таймера
    if (timerStatus == STATUS_DISPLAY && oldStatus == STATUS_RUNNING) {
        Serial.printf("[%7lu] [FSM] >>> STOP TIMER (%.3f s) <<<\n", nowMs, currentValue);
    }

    // Отслеживаем когда луч стал стабильно восстановлен
    if (beamRestoredStable && !oldRestoredStable) {
        unsigned long timeSinceRestoredUs = micros() - oldRestoredAt;
        bool canStop = timeSinceRestoredUs > MIN_BEAM_RESTORED_TIME_US;
        Serial.printf("[%7lu] [BEAM] Stable RESTORED (%lu ms, canStop after %lu ms)\n",
                      nowMs, timeSinceRestoredUs / 1000, (oldRestoredAt + MIN_BEAM_RESTORED_TIME_US) / 1000);
    }
    
    // Вывод времени таймера каждую секунду
    if (timerStatus == STATUS_RUNNING && measurementInProgress) {
        static unsigned long lastPrintTime = 0;
        if (nowMs - lastPrintTime >= 1000) {
            Serial.printf("[%7lu] [TIMER] %.3f s\n", nowMs, currentRaceTime / 1000000.0);
            lastPrintTime = nowMs;
        }
    }
}

// ============================================================================
// Safe Accessor Functions
// ============================================================================

unsigned long long getStartTimeSafe() {
    unsigned long long value;
    portENTER_CRITICAL(&timerMux);
    value = startTime;
    portEXIT_CRITICAL(&timerMux);
    return value;
}

unsigned long long getCurrentRaceTimeSafe() {
    unsigned long long value;
    portENTER_CRITICAL(&timerMux);
    value = currentRaceTime;
    portEXIT_CRITICAL(&timerMux);
    return value;
}

float getCurrentValueSafe() {
    float value;
    portENTER_CRITICAL(&timerMux);
    value = currentValue;
    portEXIT_CRITICAL(&timerMux);
    return value;
}

bool getMeasurementReadySafe() {
    bool value;
    portENTER_CRITICAL(&timerMux);
    value = measurementReady;
    portEXIT_CRITICAL(&timerMux);
    return value;
}

bool getMeasurementInProgressSafe() {
    bool value;
    portENTER_CRITICAL(&timerMux);
    value = measurementInProgress;
    portEXIT_CRITICAL(&timerMux);
    return value;
}

TimerStatus getTimerStatus() {
    TimerStatus status;
    portENTER_CRITICAL(&timerMux);
    status = timerStatus;
    portEXIT_CRITICAL(&timerMux);
    return status;
}

unsigned long getDisplayStartTimeSafe() {
    unsigned long value;
    portENTER_CRITICAL(&timerMux);
    value = displayStartTime;
    portEXIT_CRITICAL(&timerMux);
    return value;
}

bool checkAndClearSensorTriggered() {
    bool triggered;
    portENTER_CRITICAL(&timerMux);
    triggered = sensorTriggered;
    sensorTriggered = false;
    portEXIT_CRITICAL(&timerMux);
    return triggered;
}

// Проверка — луч потерян (не восстанавливается > 10 секунд)
bool isBeamLost() {
    const unsigned long nowUs = micros();
    // Луч потерян если:
    // 1. Луч сейчас прерван (beamBrokenStable == true)
    // 2. Луч НЕ восстановился (beamRestoredStable == false)
    // 3. Прошло > 10 секунд с момента прерывания
    if (beamBrokenStable && !beamRestoredStable && beamLostStartTime > 0) {
        return (nowUs - beamLostStartTime) > BEAM_LOST_TIMEOUT_US;
    }
    return false;
}

void lockMeasurements() {
    portENTER_CRITICAL(&timerMux);
}

void unlockMeasurements() {
    portEXIT_CRITICAL(&timerMux);
}
