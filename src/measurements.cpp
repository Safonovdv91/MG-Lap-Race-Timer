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

// Переменные времени срабатывания датчика
volatile unsigned long sensor1DisplayTime = 0;
volatile unsigned long sensor2DisplayTime = 0;

bool sensor1Active = false;
bool sensor2Active = false;

// переменная отображение времени
volatile unsigned long currentRaceTime = 0;
volatile float currentValue = 0.0;

// переменные измерения напряжения
float batteryVoltage = 0.0;
int batteryPercentage = 0;
unsigned long lastBatteryRead = 0;


// Функция чтения батареи
void readBattery() {
  if (millis() - lastBatteryRead > 5000) { // Читаем каждые 5 секунд
    int raw = analogRead(BATTERY_PIN);
    batteryVoltage = (raw * 3.3 / 4095.0) * 2; // Умножаем на коэффициент делителя (100кОм / 100кОм - 1:1)
    
    // Рассчитываем процент заряда
    batteryPercentage = constrain(
      map(batteryVoltage * 100, BATTERY_MIN_V * 100, BATTERY_MAX_V * 100, 0, 100),
      0, 100
    );
    
    lastBatteryRead = millis();
  }
}

// Реализации функций
void IRAM_ATTR handleSensor1() {
  unsigned long now = micros();
  sensor1Active = true;
  static unsigned long lastInterrupt = 0;


  if (now - lastInterrupt < DEBOUNCE_TIME) return;
  lastInterrupt = now;

  // Обновление времени отображения
  sensor1DisplayTime = micros();

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
      currentRaceTime = now; // Инициализация таймера
      sensor1Triggered = true;
    }
  }
}

void IRAM_ATTR handleSensor2() {
  unsigned long now = micros();
  sensor2Active = true;
  static unsigned long lastInterrupt = 0;

  
  if (now - lastInterrupt < DEBOUNCE_TIME) return;
  lastInterrupt = now;

  // Обновление времени отображения сработки датчика
  sensor2DisplayTime = micros();

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
  readBattery(); // Чтение данных батарейки
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

// Функцию для обновления состояния датчиков
void updateSensorDisplay() {
  unsigned long currentTime = micros();
  
  // Проверяем, прошло ли 3 секунды с момента срабатывания для отображения сработки датчиков
  if (sensor1Active && (currentTime - sensor1DisplayTime > 3000000)) {
    sensor1Active = false;
  }
  
  if (sensor2Active && (currentTime - sensor2DisplayTime > 3000000)) {
    sensor2Active = false;
  }
}

// Функцию для обновления времени
void updateRaceTimer() {
  if ((currentMode == RACE_TIMER || currentMode == LAP_TIMER) && sensor1Triggered && !measurementReady) {
    currentRaceTime = micros(); // Обновляем время в реальном времени
  }
}