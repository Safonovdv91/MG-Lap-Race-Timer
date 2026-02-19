#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <Arduino.h>
#include "config.h"
#include "receiver_config.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>


enum Mode { LAP_TIMER, RACE_TIMER };
enum TimerStatus { STATUS_READY, STATUS_RUNNING, STATUS_DISPLAY };

struct Measurement {
  float value;
  unsigned long timestamp;
};

extern Measurement speedHistory[HISTORY_SIZE];
extern Measurement lapHistory[HISTORY_SIZE];
extern int historyIndex;

extern volatile unsigned long long startTime;
extern volatile unsigned long long endTime;

extern volatile bool measurementReady;
extern volatile bool measurementInProgress;

extern volatile unsigned long long currentRaceTime; // Для режима отображения реального времени
extern volatile float currentValue;

void addToHistory(Measurement history[], float value);
void processMeasurements();
void IRAM_ATTR handleSensor();

void handleStatusLED();


extern volatile unsigned long lastSensorPulseTime;

// Функции для безопасного получения значений
unsigned long long getStartTimeSafe();
unsigned long long getCurrentRaceTimeSafe();
bool getSensor1TriggeredSafe();
bool getMeasurementReadySafe();
bool getMeasurementInProgressSafe();
TimerStatus getTimerStatus();
unsigned long getDisplayStartTimeSafe();

void lockMeasurements();
void unlockMeasurements();

#endif