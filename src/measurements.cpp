#include "measurements.h"
#include "config.h"

// Инициализация переменных
Mode currentMode = SPEEDOMETER;
float distance = 3.0; 

Measurement speedHistory[HISTORY_SIZE];
Measurement lapHistory[HISTORY_SIZE];
int historyIndex = 0;

volatile unsigned long startTime = 0;
volatile unsigned long endTime = 0;

volatile bool sensor1Triggered = false;
volatile bool sensor2Triggered = false;
volatile bool measurementReady = false;
volatile bool measurementInProgress = false;

volatile float currentValue = 0.0;
// Реализации функций
void IRAM_ATTR handleSensor1() {
  static unsigned long lastInterrupt = 0;
  unsigned long now = micros();
  
  if (now - lastInterrupt < DEBOUNCE_TIME * 1000) return;
  lastInterrupt = now;

  if (currentMode == SPEEDOMETER) {
    if (!sensor1Triggered && !sensor2Triggered) {
      measurementInProgress = true;
      startTime = now;
      sensor1Triggered = true;
    }
  } 
  else if (currentMode == LAP_TIMER) {
    if (!sensor1Triggered) {
      startTime = now;
      sensor1Triggered = true;
    } else if (now - startTime > MIN_LAP_TIME) {
      endTime = now;
      sensor1Triggered = false;
      measurementReady = true;
    }
  }
  else if (currentMode == RACE_TIMER) {
    if (!sensor1Triggered) {
      startTime = now;
      sensor1Triggered = true;
    }
  }
}

void IRAM_ATTR handleSensor2() {
  static unsigned long lastInterrupt = 0;
  unsigned long now = micros();
  
  if (now - lastInterrupt < DEBOUNCE_TIME * 1000) return;
  lastInterrupt = now;

  if (currentMode == SPEEDOMETER) {
    if (sensor1Triggered && !sensor2Triggered) {
      endTime = now;
      sensor2Triggered = true;
      measurementReady = true;
    }
  }
  else if (currentMode == RACE_TIMER) {
    if (sensor1Triggered) {
      endTime = now;
      sensor1Triggered = false;
      measurementReady = true;
    }
  }
}

void processMeasurements() {
  if (measurementReady) {
    measurementInProgress = false;
    unsigned long duration = endTime - startTime;
    
    if (currentMode == SPEEDOMETER) {
      currentValue = (distance / (duration / 1000000.0)) * 3.6;
      addToHistory(speedHistory, currentValue);
    } else {
      currentValue = duration / 1000000.0;
      addToHistory(lapHistory, currentValue);
    }
    
    measurementReady = false;
    sensor1Triggered = false;
    sensor2Triggered = false;
  }
}

void addToHistory(Measurement history[], float value) {
  // Смещаем все существующие записи вниз
  for (int i = HISTORY_SIZE - 1; i > 0; i--) {
    history[i] = history[i - 1];
  }
  
  // Добавляем новое значение в начало массива
  history[0] = {value, millis()};
  
  // Обновляем индекс (если используется где-то еще)
  if (historyIndex < HISTORY_SIZE) {
    historyIndex++;
  }
}