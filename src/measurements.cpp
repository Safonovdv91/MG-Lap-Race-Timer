#include "measurements.h"
#include "config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Добавляем поддержку ИК сенсоров
#ifdef USE_IR_SENSORS
#include "../include/ir_receiver.h"
#endif

#ifdef RECEIVER_MODE
#include "../include/web_handlers.h"  // для доступа к телеметрии излучателя
#endif

// Инициализация переменных
Mode currentMode = LAP_TIMER;
float distance = 3.0;

Measurement speedHistory[HISTORY_SIZE];
Measurement lapHistory[HISTORY_SIZE];
int historyIndex = 0;

volatile unsigned long long startTime = 0;
volatile unsigned long long endTime = 0;

volatile bool sensor1Triggered = false;
volatile bool sensor2Triggered = false;
volatile bool measurementReady = false;
volatile bool measurementInProgress = false;

// Переменные времени срабатывания датчика
volatile unsigned long long sensor1DisplayTime = 0;
volatile unsigned long long sensor2DisplayTime = 0;

bool sensor1Active = false;
bool sensor2Active = false;

// переменная отображение времени
volatile unsigned long long currentRaceTime = 0;
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
  unsigned long long now = micros();
  static unsigned long long lastInterrupt = 0;

  if (now - lastInterrupt < DEBOUNCE_TIME) return;
  lastInterrupt = now;

  // Обновление времени отображения
  sensor1DisplayTime = micros();

#ifdef USE_IR_SENSORS
  // Для ИК сенсоров устанавливаем активность только при пересечении луча
  sensor1Active = true;
#else
  sensor1Active = true;
#endif

  if (currentMode == SPEEDOMETER) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);
    if (!sensor1Triggered && !sensor2Triggered) {
      measurementInProgress = true;
      startTime = now;
      sensor1Triggered = true;
    }
    taskEXIT_CRITICAL(&mux);
  }
  else if (currentMode == LAP_TIMER) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);
    if (!sensor1Triggered) {
      startTime = now;
      sensor1Triggered = true;
    } else if (now - startTime > MIN_LAP_TIME) {
      endTime = now;
      sensor1Triggered = false;
      measurementReady = true;
    }
    taskEXIT_CRITICAL(&mux);
  }

  else if (currentMode == RACE_TIMER) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);
    if (!sensor1Triggered) {
      startTime = now;
      currentRaceTime = now; // Инициализация таймера
      sensor1Triggered = true;
    }
    taskEXIT_CRITICAL(&mux);
  }
}

void IRAM_ATTR handleSensor2() {
  unsigned long long now = micros();
  static unsigned long long lastInterrupt = 0;

  
  if (now - lastInterrupt < DEBOUNCE_TIME) return;
  lastInterrupt = now;

  // Обновление времени отображения сработки датчика
  sensor2DisplayTime = micros();

#ifdef USE_IR_SENSORS
  // Для ИК сенсоров устанавливаем активность только при пересечении луча
  sensor2Active = true;
#else
  sensor2Active = true;
#endif

  if (currentMode == SPEEDOMETER) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);
    if (sensor1Triggered && !sensor2Triggered) {
      endTime = now;
      sensor2Triggered = true;
      measurementReady = true;
    }
    taskEXIT_CRITICAL(&mux);
  }
  else if (currentMode == RACE_TIMER) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);
    if (sensor1Triggered) {
      endTime = now;
      sensor1Triggered = false;
      measurementReady = true;
    }
    taskEXIT_CRITICAL(&mux);
  }
}

void processMeasurements() {
  readBattery(); // Чтение данных батарейки
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);
  if (measurementReady) {
    measurementInProgress = false;
    unsigned long long startTimeVal = startTime;
    unsigned long long endTimeVal = endTime;
    unsigned long long duration = 0;
    if (endTimeVal >= startTimeVal) {
      duration = endTimeVal - startTimeVal;
    }
    
    if (currentMode == SPEEDOMETER) {
      if (duration > 0) {
        currentValue = (distance / (duration / 1000000.0)) * 3.6;
      } else {
        currentValue = 0.0;
      }
      addToHistory(speedHistory, currentValue);
    } else {
      currentValue = duration / 1000000.0;
      addToHistory(lapHistory, currentValue);
    }
    
    measurementReady = false;
    sensor1Triggered = false;
    sensor2Triggered = false;
  }
  taskEXIT_CRITICAL(&mux);
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
  unsigned long long currentTime = micros();
  
#ifdef USE_IR_SENSORS
  // Для ИК сенсоров обновляем состояние на основе наличия/отсутствия луча
  // sensor1Active = true, когда луч заблокирован (пересечен)
  sensor1Active = isIRBeamBlocked1();
  sensor2Active = isIRBeamBlocked2();
#else
  // Проверяем, прошло ли 3 секунды с момента срабатывания для отображения сработки датчиков
  if (sensor1Active && (currentTime - sensor1DisplayTime > 3000000)) {
    sensor1Active = false;
  }
  
  if (sensor2Active && (currentTime - sensor2DisplayTime > 3000000)) {
    sensor2Active = false;
  }
#endif
}

// Функцию для обновления времени
void updateRaceTimer() {
  if ((currentMode == RACE_TIMER || currentMode == LAP_TIMER)) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);
    if (sensor1Triggered && !measurementReady) {
      currentRaceTime = micros(); // Обновляем время в реальном времени
    }
    taskEXIT_CRITICAL(&mux);
  }
}

// Функции для безопасного получения значений
unsigned long long getStartTimeSafe() {
  unsigned long long val;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);  // Критическая секция
  val = startTime;
  taskEXIT_CRITICAL(&mux);
  return val;
}

unsigned long long getCurrentRaceTimeSafe() {
  unsigned long long val;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);  // Критическая секция
  val = currentRaceTime;
  taskEXIT_CRITICAL(&mux);
  return val;
}

bool getSensor1TriggeredSafe() {
  bool val;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);  // Критическая секция
  val = sensor1Triggered;
  taskEXIT_CRITICAL(&mux);
  return val;
}

bool getSensor2TriggeredSafe() {
  bool val;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);  // Критическая секция
  val = sensor2Triggered;
  taskEXIT_CRITICAL(&mux);
  return val;
}

bool getMeasurementReadySafe() {
  bool val;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  taskENTER_CRITICAL(&mux);  // Критическая секция
  val = measurementReady;
  taskEXIT_CRITICAL(&mux);
  return val;
}
