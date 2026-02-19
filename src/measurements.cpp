#include "measurements.h"
#include "config.h"
#include "receiver_config.h"

#include "websocket_handlers.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
// Define a critical section spinlock
static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// --- Global Variables ---

Mode currentMode = LAP_TIMER;
// Timer State Machine
volatile TimerStatus timerStatus = STATUS_READY;
volatile unsigned long displayStartTime = 0;

Measurement speedHistory[HISTORY_SIZE];
Measurement lapHistory[HISTORY_SIZE];
int historyIndex = 0;

// Переменные времения для основного таймера
volatile unsigned long long startTime = 0;
volatile unsigned long long endTime = 0;
volatile bool measurementReady = false;
volatile bool measurementInProgress = false;

// Переменные состояния луча
static bool beamBrokenStable = false;
static bool beamRestoredStable = true;
static unsigned long beamRestoreStartTime = 0;

#define RESTORE_STABLE_TIME_US 20000  // 20 мс стабильного восстановления


// Переменная времени когда луч разорван в (мкс)
volatile unsigned long lastSensorPulseTime = 0;

// State tracking for beam break detection
bool beamBroken = false;

// Отображение & UI переменных
volatile unsigned long long currentRaceTime = 0;
volatile float currentValue = 0.0;
bool sensorActive = false;


void IRAM_ATTR handleSensor() {
  // функция определения времени пересечения импульса
  portENTER_CRITICAL_ISR(&timerMux);
  lastSensorPulseTime = micros();
  portEXIT_CRITICAL_ISR(&timerMux);
}

// Функция для управления статусным светодиодом
void handleStatusLED() {
  int sensorState = digitalRead(SENSOR_PIN);
  if (sensorState == HIGH) {
    digitalWrite(STATUS_IR_LED_PIN, LOW);
    
  } else {
    digitalWrite(STATUS_IR_LED_PIN, HIGH);
  }  
}



void addToHistory(Measurement history[], float value) {
  // This function assumes the caller has already acquired the timerMux lock
  if (historyIndex < HISTORY_SIZE) {
    history[historyIndex++] = {value, millis()};
  } else {
    for (int i = 0; i < HISTORY_SIZE - 1; i++) {
      history[i] = history[i + 1];
    }
    history[HISTORY_SIZE - 1] = {value, millis()};
  }
}

void handleCooldown(unsigned long nowMs)
// Куллдаун на запуск нового таймера,
// чтобы не было мгновенной сработки после пересечения
{
    if (timerStatus == STATUS_DISPLAY &&
        (nowMs - displayStartTime > TIMER_COOLDOWN_PERIOD))
    {
        timerStatus = STATUS_READY;
        measurementInProgress = false;
    }
}

void startTimer(unsigned long nowUs)
// Запуск таймера хронографа
{
    startTime = nowUs;
    measurementInProgress = true;
    timerStatus = STATUS_RUNNING;
    currentRaceTime = 0;
}
void stopTimerIfValid(unsigned long nowUs)
//  Остановка таймера в случае если не был 
//  пересечен раньше времени отображения на дисплее ~ 5c
// 
{
    if (nowUs - startTime > MIN_LAP_TIME)
    {
        endTime = nowUs;
        measurementReady = true;
    }
}
bool processFinishedMeasurement(unsigned long nowMs)
{
    if (!measurementReady)
        return false;

    const unsigned long long duration = endTime - startTime;

    currentValue = duration / 1000000.0;
    addToHistory(lapHistory, currentValue);

    timerStatus = STATUS_DISPLAY;
    displayStartTime = nowMs;

    measurementReady = false;

    return true;
}
void updateLiveTimer(unsigned long nowUs)
{
    if (timerStatus == STATUS_RUNNING)
    {
        currentRaceTime = nowUs - startTime;
    }
    else if (timerStatus == STATUS_DISPLAY)
    {
        currentRaceTime = endTime - startTime;
    }
    else
    {
        currentRaceTime = 0;
    }
}
void handleWebsocketBroadcast(unsigned long nowMs, bool lapFinished)
{
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

void handleSerialOutput(unsigned long nowMs, bool lapFinished)
{
  
    static unsigned long lastSerialPrintTime = 0;

    if (lapFinished)
    {
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

void handleBeamState(bool beamBrokenNow, unsigned long nowUs)
{
    // ---- Луч сейчас прерван ----
    if (beamBrokenNow)
    {
        // Если луч был стабильно восстановлен — это новое пересечение
        if (beamRestoredStable)
        {
            beamBrokenStable = true;
            beamRestoredStable = false;
            beamRestoreStartTime = 0;

            if (timerStatus == STATUS_READY)
            {
                startTimer(nowUs);
            }
            else if (timerStatus == STATUS_RUNNING)
            {
                stopTimerIfValid(nowUs);
            }
        }
    }
    // ---- Луч восстановился ----
    else
    {
        if (!beamRestoredStable)
        {
            if (beamRestoreStartTime == 0)
                beamRestoreStartTime = nowUs;

            if (nowUs - beamRestoreStartTime > RESTORE_STABLE_TIME_US)
            {
                beamRestoredStable = true;
                beamBrokenStable = false;
                beamRestoreStartTime = 0;
            }
        }
    }
}

// --- Core Logic ---
void processMeasurements()
// Основная логика работы при таймере
{
    // =========================
    // 1 — Атомарное чтение 
    // Копирует timestamp из ISR в локальную переменную.
    // Чтобы работать с переменной, а не ISR
    // Избегаем RACE CONDITION
    // =========================

    const unsigned long nowMs = millis();
    const unsigned long nowUs = micros();

    unsigned long lastPulseTime;
    
    portENTER_CRITICAL(&timerMux);
    lastPulseTime = lastSensorPulseTime;
    portEXIT_CRITICAL(&timerMux);

    // Определение состояния луча. т.к. возможны всевозможные помехи,
    // то вводим переменную на эффект дрожания входа ~ 4 ms
    const bool beamBrokenNow =
        (nowUs - lastPulseTime) > BEAM_BREAK_THRESHOLD;

    sensorActive = beamBrokenNow;

    // =========================
    // 2 — FSM
    // =========================

    bool lapFinished = false;

    lockMeasurements(); // Переходим в безопасный режим

    handleCooldown(nowMs);
    handleBeamState(beamBrokenNow, nowUs);
    lapFinished = processFinishedMeasurement(nowMs);
    updateLiveTimer(nowUs);

    unlockMeasurements();

    // =========================
    // Phase 3 — Side Effects
    // =========================

    handleWebsocketBroadcast(nowMs,lapFinished);
    handleSerialOutput(nowMs, lapFinished);
}


// --- Safe Accessor Functions ---
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

bool getSensor1TriggeredSafe() {
  bool value;
  portENTER_CRITICAL(&timerMux);
  value = beamBroken;
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

void lockMeasurements() {
  portENTER_CRITICAL(&timerMux);
}

void unlockMeasurements() {
  portEXIT_CRITICAL(&timerMux);
}
