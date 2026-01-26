#include "measurements.h"
#include "config.h"
#include "receiver_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Define a critical section spinlock
static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#ifdef USE_IR_SENSORS
#include "../include/ir_receiver.h"
#endif

// --- Global Variables ---
Mode currentMode = LAP_TIMER;
float distance = 3.0;

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
    batteryVoltage = (raw * 3.3 / 4095.0) * 2;
    batteryPercentage = constrain(map(batteryVoltage * 100, BATTERY_MIN_V * 100, BATTERY_MAX_V * 100, 0, 100), 0, 100);
    lastBatteryRead = millis();
  }
}

void addToHistory(Measurement history[], float value) {
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

  // --- State Machine: Cooldown Check ---
  if (timerStatus == STATUS_DISPLAY && millis() - displayStartTime > TIMER_COOLDOWN_PERIOD) {
    portENTER_CRITICAL(&timerMux);
    timerStatus = STATUS_READY;
    measurementInProgress = false;
    startTime = 0;
    endTime = 0;
    portEXIT_CRITICAL(&timerMux);
  }

  unsigned long currentTime = micros();
  unsigned long pulseTime;

  portENTER_CRITICAL(&timerMux);
  pulseTime = lastSensor1PulseTime;
  portEXIT_CRITICAL(&timerMux);

  bool isBeam1CurrentlyBroken = (currentTime - pulseTime) > 10000; // 10ms threshold
  sensor1Active = isBeam1CurrentlyBroken;

  // --- SENSOR 1 TRIGGER LOGIC (State-dependent) ---
  if (isBeam1CurrentlyBroken && !beam1Broken) {
    beam1Broken = true;

    portENTER_CRITICAL(&timerMux);
    if (timerStatus == STATUS_READY) {
      // START timer
      startTime = currentTime;
      measurementInProgress = true;
      timerStatus = STATUS_RUNNING;
      Serial.println("Таймер запущен!");

    } else if (timerStatus == STATUS_RUNNING) {
      // STOP timer
      if (currentTime - startTime > MIN_LAP_TIME) {
        endTime = currentTime;
        measurementReady = true;
      }
    }
    // In STATUS_DISPLAY, we do nothing.
    portEXIT_CRITICAL(&timerMux);

  } else if (!isBeam1CurrentlyBroken) {
    beam1Broken = false;
  }

  // --- DATA PROCESSING ---
  portENTER_CRITICAL(&timerMux);
  bool isMeasurementReady = measurementReady;
  portEXIT_CRITICAL(&timerMux);

  if (isMeasurementReady) {
    unsigned long long duration = endTime - startTime;

    if (currentMode == SPEEDOMETER) {
      // This mode is not affected by the new state machine in the same way
      // For now, we process it simply.
      if (duration > 0) {
        currentValue = (distance / (duration / 1000000.0)) * 3.6;
      } else {
        currentValue = 0.0;
      }
      addToHistory(speedHistory, currentValue);
      // Reset for next speed measurement
      portENTER_CRITICAL(&timerMux);
      measurementInProgress = false;
      startTime = 0;
      portEXIT_CRITICAL(&timerMux);

    } else { // LAP_TIMER and RACE_TIMER
      currentValue = duration / 1000000.0;
      Serial.print("Время круга: ");
      Serial.print(currentValue, 3);
      Serial.println(" с");
      addToHistory(lapHistory, currentValue);

      // --- Enter Display State ---
      portENTER_CRITICAL(&timerMux);
      timerStatus = STATUS_DISPLAY;
      displayStartTime = millis();
      
      if (currentMode == RACE_TIMER) {
        startTime = endTime; // For race timer, next lap starts from this end time
      }
      // Note: measurementInProgress remains true during display
      portEXIT_CRITICAL(&timerMux);
    }

    portENTER_CRITICAL(&timerMux);
    measurementReady = false; // Mark as processed
    portEXIT_CRITICAL(&timerMux);
  }

  // --- LIVE RACE TIMER FOR UI AND SERIAL ---
  static unsigned long lastSerialPrintTime = 0;
  
  portENTER_CRITICAL(&timerMux);
  if (timerStatus == STATUS_RUNNING) {
    currentRaceTime = micros() - startTime;
  } else if (timerStatus == STATUS_DISPLAY) {
    currentRaceTime = endTime - startTime; // Show final time
  } else {
    currentRaceTime = 0;
  }
  portEXIT_CRITICAL(&timerMux);

  if (measurementInProgress && millis() - lastSerialPrintTime > 1000) {
    Serial.print("Текущее время: ");
    Serial.print(currentRaceTime / 1000000.0, 3);
    Serial.println(" с");
    lastSerialPrintTime = millis();
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
  return beam1Broken;
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
