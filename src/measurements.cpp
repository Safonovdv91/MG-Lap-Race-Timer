#include "measurements.h"
#include "config.h"
#include "receiver_config.h"
#ifdef RECEIVER_MODE
#include "websocket_handlers.h"
#endif
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Define a critical section spinlock
static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#ifdef USE_IR_SENSORS
#include "../include/ir_receiver.h"
#endif

// --- Global Variables ---
#ifdef RECEIVER_MODE
Mode currentMode = LAP_TIMER;
float distance = 3.0;
#endif

// Timer State Machine
volatile TimerStatus timerStatus = STATUS_READY;
volatile unsigned long displayStartTime = 0;

Measurement speedHistory[HISTORY_SIZE];
Measurement lapHistory[HISTORY_SIZE];
int historyIndex = 0;

// Core timing variables
volatile unsigned long long startTime = 0;
volatile unsigned long long endTime = 0;
volatile bool measurementReady = false;
volatile bool measurementInProgress = false;

// Timestamps for the last received IR pulse for each sensor
volatile unsigned long lastSensor1PulseTime = 0;

// State tracking for beam break detection
bool beam1Broken = false;

// Display & UI variables
volatile unsigned long long currentRaceTime = 0;
volatile float currentValue = 0.0;
bool sensor1Active = false;

// Battery measurement variables
float batteryVoltage = 0.0;
int batteryPercentage = 0;
unsigned long lastBatteryRead = 0;

// --- Minimalistic Interrupt Service Routines ---

void IRAM_ATTR handleSensor1() {
  portENTER_CRITICAL_ISR(&timerMux);
  lastSensor1PulseTime = micros();
  portEXIT_CRITICAL_ISR(&timerMux);
}

// --- Core Logic ---

void readBattery() {
  if (millis() - lastBatteryRead > 5000) { // Read every 5 seconds
    int raw = analogRead(BATTERY_PIN);
    batteryVoltage = (raw * ADC_REFERENCE_VOLTAGE / ADC_MAX_READING) * VOLTAGE_DIVIDER_MULTIPLIER;
    batteryPercentage = constrain(map(batteryVoltage * 100, BATTERY_MIN_V * 100, BATTERY_MAX_V * 100, 0, 100), 0, 100);
    lastBatteryRead = millis();
    
    Serial.print("Значения батареи: ");
    Serial.print("U: [");
    Serial.print(batteryVoltage);
    Serial.print(" V ] percents: [");
    Serial.print(batteryPercentage);
    Serial.println(" %]");
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

void processMeasurements() {
  readBattery();

  // --- Phase 1: Gather Inputs (outside lock) ---
  unsigned long currentTime_ms = millis();
  unsigned long currentTime_us = micros();
  unsigned long pulseTime;

  // Atomically get the last pulse time
  portENTER_CRITICAL(&timerMux);
  pulseTime = lastSensor1PulseTime;
  portEXIT_CRITICAL(&timerMux);

  bool isBeam1CurrentlyBroken = (currentTime_us - pulseTime) > 10000; // 10ms threshold
  sensor1Active = isBeam1CurrentlyBroken;

  // --- Phase 2: Process State Machine (inside a single critical section) ---
  bool should_broadcast = false;
  bool isLapFinished = false;

  lockMeasurements(); // Enter critical section

  // 1. Cooldown Check
  if (timerStatus == STATUS_DISPLAY && (currentTime_ms - displayStartTime > TIMER_COOLDOWN_PERIOD)) {
    timerStatus = STATUS_READY;
    measurementInProgress = false;
    should_broadcast = true;
  }

  // 2. Sensor Trigger Logic (State-dependent)
  if (isBeam1CurrentlyBroken && !beam1Broken) {
    beam1Broken = true;
    if (timerStatus == STATUS_READY) {
      // START timer
      startTime = currentTime_us;
      measurementInProgress = true;
      timerStatus = STATUS_RUNNING;
      currentRaceTime = 0;
      should_broadcast = true;
    } else if (timerStatus == STATUS_RUNNING) {
      // STOP timer
      if (currentTime_us - startTime > MIN_LAP_TIME) {
        endTime = currentTime_us;
        measurementReady = true; // Flag that a lap is ready for processing
      }
    }
    // In STATUS_DISPLAY, we do nothing on a new trigger.
  } else if (!isBeam1CurrentlyBroken) {
    beam1Broken = false;
  }

  // 3. Data Processing (if a lap was finished in the previous step)
  if (measurementReady) {
    unsigned long long duration = endTime - startTime;

    if (currentMode == SPEEDOMETER) {
      currentValue = (duration > 0) ? (distance / (duration / 1000000.0)) * 3.6 : 0.0;
      addToHistory(speedHistory, currentValue);
      measurementInProgress = false;
      startTime = 0;
    } else { // LAP_TIMER and RACE_TIMER
      currentValue = duration / 1000000.0;
      isLapFinished = true; // Flag for serial print outside lock
      addToHistory(lapHistory, currentValue);

      // Enter Display State
      timerStatus = STATUS_DISPLAY;
      displayStartTime = currentTime_ms;
      
      if (currentMode == RACE_TIMER) {
        startTime = endTime; // For race timer, next lap starts from this end time
      }
    }
    
    measurementReady = false; // Mark as processed
    should_broadcast = true;
  }

  // 4. Update Live Race Timer
  if (timerStatus == STATUS_RUNNING) {
    currentRaceTime = currentTime_us - startTime;
  } else if (timerStatus == STATUS_DISPLAY) {
    currentRaceTime = endTime - startTime; // Show final time
  } else {
    currentRaceTime = 0;
  }

  unlockMeasurements(); // Exit critical section

  // --- Phase 3: Perform Side-Effects (outside lock) ---
  
  // 1. Broadcast data via WebSocket if needed
  static unsigned long lastWsBroadcastTime = 0;
  if (should_broadcast || (timerStatus == STATUS_RUNNING && (currentTime_ms - lastWsBroadcastTime > 100))) {
    #ifdef RECEIVER_MODE
      ws_broadcast_data();
    #endif
    lastWsBroadcastTime = currentTime_ms;
  }

  // 2. Print final lap time to Serial
  if (isLapFinished) {
    Serial.print("Время круга: ");
    Serial.print(currentValue, 3);
    Serial.println(" с");
  }

  // 3. Print live race time to Serial periodically
  static unsigned long lastSerialPrintTime = 0;
  if (measurementInProgress && (currentTime_ms - lastSerialPrintTime > 1000)) {
    Serial.print("Текущее время: ");
    Serial.print(currentRaceTime / 1000000.0, 3);
    Serial.println(" с");
    lastSerialPrintTime = currentTime_ms;
  }
}

// --- Unused Functions (can be removed or left for future use) ---

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
  value = beam1Broken;
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
