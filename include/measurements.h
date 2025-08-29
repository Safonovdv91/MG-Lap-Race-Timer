#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <Arduino.h>
#include "config.h"

enum Mode { SPEEDOMETER, LAP_TIMER, RACE_TIMER };

extern Mode currentMode;
extern float distance;

// Переменные для отслеживания времени срабатывания пересечения датчика
extern volatile unsigned long sensor1DisplayTime;
extern volatile unsigned long sensor2DisplayTime;
extern bool sensor1Active;
extern bool sensor2Active;

struct Measurement {
  float value;
  unsigned long timestamp;
};

extern Measurement speedHistory[HISTORY_SIZE];
extern Measurement lapHistory[HISTORY_SIZE];
extern int historyIndex;

extern volatile unsigned long startTime;
extern volatile unsigned long endTime;

extern volatile bool sensor1Triggered;
extern volatile bool sensor2Triggered;
extern volatile bool measurementReady;
extern volatile bool measurementInProgress;

extern volatile unsigned long currentRaceTime; // Для режима отображения реального времени
extern volatile float currentValue;

// Переменные напряжения питания
extern float batteryVoltage;
extern int batteryPercentage;

void addToHistory(Measurement history[], float value);
void processMeasurements();
void IRAM_ATTR handleSensor1();
void IRAM_ATTR handleSensor2();
void updateSensorDisplay();
void updateRaceTimer();

#endif