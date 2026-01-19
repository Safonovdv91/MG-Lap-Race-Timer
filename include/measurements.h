#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <Arduino.h>
#include "config.h"

enum Mode { SPEEDOMETER, LAP_TIMER, RACE_TIMER };

extern Mode currentMode;
extern float distance;

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

// Переменные напряжения питания
extern float batteryVoltage;
extern int batteryPercentage;

void addToHistory(Measurement history[], float value);
void processMeasurements();
void IRAM_ATTR handleSensor1();

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

extern volatile unsigned long lastSensor1PulseTime;

// Функции для безопасного получения значений
unsigned long long getStartTimeSafe();
unsigned long long getCurrentRaceTimeSafe();
bool getSensor1TriggeredSafe();
bool getMeasurementReadySafe();
bool getMeasurementInProgressSafe();

// Добавляем объявление мьютекса для синхронизации
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>


#endif