#include "measurements.h"
#include "config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef USE_IR_SENSORS
#include "../include/ir_receiver.h"
#endif

// --- Global Variables ---
Mode currentMode = LAP_TIMER;
float distance = 3.0;

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
volatile unsigned long lastSensor2PulseTime = 0;

// State tracking for beam break detection
bool beam1Broken = false;
bool beam2Broken = false;

// Display & UI variables
volatile unsigned long long currentRaceTime = 0;
volatile float currentValue = 0.0;
bool sensor1Active = false;
bool sensor2Active = false;

// Battery measurement variables
float batteryVoltage = 0.0;
int batteryPercentage = 0;
unsigned long lastBatteryRead = 0;

// --- Minimalistic Interrupt Service Routines ---

void IRAM_ATTR handleSensor1() {
  lastSensor1PulseTime = micros();
}

void IRAM_ATTR handleSensor2() {
  lastSensor2PulseTime = micros();
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

  unsigned long currentTime = micros();
  bool isBeam1CurrentlyBroken = (currentTime - lastSensor1PulseTime) > 100000; // 100ms threshold
  bool isBeam2CurrentlyBroken = (currentTime - lastSensor2PulseTime) > 100000; // 100ms threshold

  // Update sensor active status for UI
  sensor1Active = isBeam1CurrentlyBroken;
  sensor2Active = isBeam2CurrentlyBroken;

  // --- SENSOR 1 TRIGGER LOGIC ---
  if (isBeam1CurrentlyBroken && !beam1Broken) { // Beam 1 was just broken (rising edge of broken state)
    beam1Broken = true;

    if (currentMode == LAP_TIMER || currentMode == RACE_TIMER) {
      if (!measurementInProgress) {
        // First pass: Start the timer
        startTime = currentTime;
        measurementInProgress = true;
        Serial.println("Таймер запущен!");
      } else {
        // Second pass: Stop the timer and mark measurement as ready
        if (currentTime - startTime > MIN_LAP_TIME) {
          endTime = currentTime;
          measurementReady = true;
        }
      }
    } else if (currentMode == SPEEDOMETER) {
      if (!measurementInProgress) {
        startTime = currentTime;
        measurementInProgress = true;
        Serial.println("Таймер запущен!");
      }
    }
  } else if (!isBeam1CurrentlyBroken) {
    beam1Broken = false;
  }

  // --- SENSOR 2 TRIGGER LOGIC (for Speedometer) ---
  if (isBeam2CurrentlyBroken && !beam2Broken) { // Beam 2 was just broken
    beam2Broken = true;
    if (currentMode == SPEEDOMETER && measurementInProgress) {
      endTime = currentTime;
      measurementReady = true;
    }
  } else if (!isBeam2CurrentlyBroken) {
    beam2Broken = false;
  }

  // --- DATA PROCESSING ---
  if (measurementReady) {
    unsigned long long duration = endTime - startTime;
    
    if (currentMode == SPEEDOMETER) {
      if (duration > 0) {
        currentValue = (distance / (duration / 1000000.0)) * 3.6;
        Serial.print("Скорость: ");
        Serial.print(currentValue);
        Serial.println(" км/ч");
      } else {
        currentValue = 0.0;
      }
      addToHistory(speedHistory, currentValue);
    } else { // LAP_TIMER and RACE_TIMER
      currentValue = duration / 1000000.0;
      Serial.print("Время круга: ");
      Serial.print(currentValue, 3);
      Serial.println(" с");
      addToHistory(lapHistory, currentValue);
    }
    
    // Reset for next measurement
    measurementReady = false;
    if (currentMode != RACE_TIMER) { // In RACE_TIMER, timing continues until reset
        measurementInProgress = false;
        startTime = 0;
    } else { // For RACE_TIMER, the end of one lap is the start of the next
        startTime = endTime;
    }
    endTime = 0;
  }

  // --- LIVE RACE TIMER FOR UI AND SERIAL ---
  static unsigned long lastSerialPrintTime = 0;
  if (measurementInProgress) {
      currentRaceTime = currentTime - startTime;
      if (millis() - lastSerialPrintTime > 1000) {
        Serial.print("Текущее время: ");
        Serial.print(currentRaceTime / 1000000.0, 3);
        Serial.println(" с");
        lastSerialPrintTime = millis();
      }
  } else {
      currentRaceTime = 0;
  }
}

// --- Unused Functions (can be removed or left for future use) ---
void updateSensorDisplay() {}
void updateRaceTimer() {}

// --- Safe Accessor Functions ---
unsigned long long getStartTimeSafe() {
  return startTime;
}

unsigned long long getCurrentRaceTimeSafe() {
  return currentRaceTime;
}

bool getSensor1TriggeredSafe() {
  return beam1Broken; // Not really used by new API, but reflects state
}

bool getSensor2TriggeredSafe() {
  return beam2Broken;
}

bool getMeasurementReadySafe() {
  return measurementReady;
}

bool getMeasurementInProgressSafe() {
  return measurementInProgress;
}
