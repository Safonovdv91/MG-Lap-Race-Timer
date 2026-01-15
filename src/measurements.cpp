#include "measurements.h"
#include "config.h"
#include <atomic>

// Инициализация переменных
Mode currentMode = SPEEDOMETER;
float distance = 3.0; 

Measurement speedHistory[HISTORY_SIZE];
Measurement lapHistory[HISTORY_SIZE];
int historyIndex = 0;

std::atomic<unsigned long> startTime{0};
std::atomic<unsigned long> endTime{0};

std::atomic<bool> sensor1Triggered{false};
std::atomic<bool> sensor2Triggered{false};
std::atomic<bool> measurementReady{false};
std::atomic<bool> measurementInProgress{false};

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
    if (!sensor1Triggered.load() && !sensor2Triggered.load()) {
      measurementInProgress.store(true);
      startTime.store(now);
      sensor1Triggered.store(true);
    }
  } 
  else if (currentMode == LAP_TIMER) {
    if (!sensor1Triggered.load()) {
      startTime.store(now);
      sensor1Triggered.store(true);
    } else if (now - startTime.load() > MIN_LAP_TIME) {
      endTime.store(now);
      sensor1Triggered.store(false);
      measurementReady.store(true);
    }
  }

  else if (currentMode == RACE_TIMER) {
    if (!sensor1Triggered.load()) {
      startTime.store(now);
      currentRaceTime = now; // Инициализация таймера
      sensor1Triggered.store(true);
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
    if (sensor1Triggered.load() && !sensor2Triggered.load()) {
      endTime.store(now);
      sensor2Triggered.store(true);
      measurementReady.store(true);
    }
  }
  else if (currentMode == RACE_TIMER) {
    if (sensor1Triggered.load()) {
      endTime.store(now);
      sensor1Triggered.store(false);
      measurementReady.store(true);
    }
  }
}

void processMeasurements() {
  readBattery(); // Чтение данных батарейки
  if (measurementReady.load()) {
    measurementInProgress.store(false);
    unsigned long duration = endTime.load() - startTime.load();
    
    if (currentMode == SPEEDOMETER) {
      currentValue = (distance / (duration / 1000000.0)) * 3.6;
      addToHistory(speedHistory, currentValue);
    } else {
      currentValue = duration / 1000000.0;
      addToHistory(lapHistory, currentValue);
    }
    
    measurementReady.store(false);
    sensor1Triggered.store(false);
    sensor2Triggered.store(false);
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
  if ((currentMode == RACE_TIMER || currentMode == LAP_TIMER) && sensor1Triggered.load() && !measurementReady.load()) {
    currentRaceTime = micros(); // Обновляем время в реальном времени
  }
}