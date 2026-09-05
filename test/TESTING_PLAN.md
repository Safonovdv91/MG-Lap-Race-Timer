# 📋 План покрытия кода тестами

## 🎯 Цель

Покрыть тестами критическую бизнес-логику проекта MG-Lap-Race-Timer для обеспечения надёжности и возможности рефакторинга.

---

## 📊 Текущий статус

| Модуль | Файл | Статус | Тесты |
|--------|------|--------|-------|
| `battery.cpp` | `src/drivers/battery/battery.cpp` | ✅ Готово | `test/test_all.cpp` |
| `measurement_core.cpp` | `src/core/measurement_core.cpp` | ✅ Готово | `test/test_all.cpp` |
| `config.cpp` | `src/utils/config.cpp` | ⏳ Ожидает | - |
| `transmitter_data.cpp` | `src/modules/transmitter_data.cpp` | ⏳ Ожидает | - |
| `espnow_*.cpp` | `src/drivers/espnow_*.cpp` | ⏳ Ожидает | - |
| `web_*.cpp` | `src/modules/web_*.cpp` | ⏳ Ожидает | - |

**Пройдено тестов:** 16 ✅

---

## 🚀 Быстрый старт

```bash
# Запуск всех тестов
pio test -e native-tests

# Подробный вывод
pio test -e native-tests -v
```

---

## 🗺️ Дорожная карта тестирования

### Этап 1: Базовые модули (выполнено)

**Цель:** Покрыть тестами модули без внешних зависимостей

- ✅ `battery.cpp` — функция `calculateBatteryPercentage()` (9 тестов)
- ✅ `measurement_core.cpp` — функции:
  - `addToHistory()` (2 теста)
  - `handleCooldown()` (2 теста)
  - `updateLiveTimer()` (3 теста)

**Файл:**
- `test/test_all.cpp` — все тесты (16 тестов)

**Результат:** 16 тестов ✅

---

### Этап 2: Модули средней сложности

**Цель:** Покрыть тестами модули с минимальными зависимостями

#### 2.1 `config.cpp`

**Функции для тестирования:**
- `loadWiFiSettings()` — загрузка настроек WiFi
- `saveWiFiSettings()` — сохранение настроек

**Требуется:**
- Моки для `Preferences` (хранение настроек)
- Моки для `Serial`

**Приоритет:** Средний

#### 2.2 `transmitter_data.cpp`

**Функции для тестирования:**
- Обработка данных перед отправкой
- Форматирование пакетов

**Требуется:**
- Моки для структур данных

**Приоритет:** Средний

---

### Этап 3: Сложные модули

**Цель:** Покрыть тестами модули с внешними зависимостями

#### 3.1 `espnow_*.cpp`

**Файлы:**
- `src/drivers/espnow_transmitter.cpp`
- `src/drivers/espnow_receiver.cpp`
- `src/drivers/espnow_broadcast.cpp`

**Функции для тестирования:**
- Инициализация ESP-NOW
- Отправка/приём пакетов
- Обработка ошибок

**Требуется:**
- Моки для ESP-NOW API
- Моки для WiFi

**Приоритет:** Низкий (интеграционные тесты сложнее)

#### 3.2 `web_*.cpp`

**Файлы:**
- `src/modules/web_handlers.cpp`
- `src/modules/web_content.cpp`
- `src/modules/websocket_handlers.cpp`

**Функции для тестирования:**
- HTTP обработчики
- WebSocket сообщения
- Генерация HTML/JSON

**Требуется:**
- Моки для WebServer
- Моки для WebSocket
- Моки для ArduinoJson

**Приоритет:** Низкий

---

## 🏗️ Архитектура тестов

### Структура директорий

```
project/
├── test/
│   ├── test_all.cpp                 # Все тесты
│   └── fixtures/
│       └── mocks/                   # Моки Arduino/ESP32
├── src/
│   ├── core/                        # Ядро
│   ├── drivers/                     # Драйверы
│   ├── modules/                     # Модули
│   └── utils/                       # Утилиты
└── include/
    ├── core/
    ├── drivers/
    ├── modules/
    └── utils/
```

### Зависимости между компонентами

```
┌─────────────────────────────────────────────────────────┐
│                    Тестовый файл                        │
│                  (test/test_all.cpp)                    │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Arduino/ESP32 Mocks                        │
│         (test/fixtures/mocks/arduino_mocks.*)           │
│  - Serial, millis(), micros()                           │
│  - portENTER_CRITICAL, portEXIT_CRITICAL                │
│  - Preferences, analogRead                              │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                  Исходный код                           │
│              (src/**/*.cpp)                             │
└─────────────────────────────────────────────────────────┘
```

---

## 🛠️ Инструменты

### PlatformIO Test Runner

**Установка:**
```bash
pip3 install platformio
```

**Команды:**
```bash
# Запуск всех тестов
pio test -e native-tests

# Запуск с подробным выводом
pio test -e native-tests -v

# Очистка и запуск
rm -rf .pio && pio test -e native-tests -v
```

### Unity Test Framework

Используется как фреймворк для unit-тестов.

**Основные макросы:**
- `TEST_ASSERT_EQUAL_INT(expected, actual)`
- `TEST_ASSERT_EQUAL_FLOAT(expected, actual)`
- `TEST_ASSERT_TRUE(condition)`
- `TEST_ASSERT_NULL(pointer)`

---

## 📝 Как писать новые тесты

### Шаг 1: Добавить тест в test/test_all.cpp

```cpp
void test_my_function(void) {
    int result = myFunction(5);
    TEST_ASSERT_EQUAL_INT(10, result);
}
```

### Шаг 2: Добавить в runUnityTests()

```cpp
int runUnityTests(void) {
    UNITY_BEGIN();
    
    // ... существующие тесты
    
    RUN_TEST(test_my_function);
    
    return UNITY_END();
}
```

### Шаг 3: Запустить

```bash
pio test -e native-tests -v
```

---

## 🎓 Советы для новичков в C

### 1. Начинайте с простых функций

Функции без побочных эффектов (без `Serial`, без `WiFi`) тестировать проще:

```cpp
// ✅ Легко: чистая функция
int calculateBatteryPercentage(float voltage) { ... }

// ❌ Сложно: функция с побочными эффектами
void sendViaESPNow(const char* data) { ... }
```

### 2. Используйте моки для внешних зависимостей

```cpp
// В исходных файлах
#ifdef UNIT_TEST
    #include "fixtures/mocks/arduino_mocks.h"
#else
    #include <Arduino.h>
#endif
```

### 3. Тестируйте одно поведение за раз

```cpp
// ✅ Хорошо: один тест — одна проверка
void test_battery_100_percent(void) { ... }
void test_battery_0_percent(void) { ... }

// ❌ Плохо: много проверок в одном тесте
void test_battery_all_cases(void) { ... }
```

### 4. Сбрасывайте состояние между тестами

```cpp
void setUp(void) {
    // Сброс глобальных переменных
    timerStatus = STATUS_READY;
    currentRaceTime = 0;
}
```

### 5. Используйте понятные имена тестов

```cpp
// ✅ Хорошо
void test_battery_100_percent(void)
void test_handleCooldown_ready_after_timeout(void)

// ❌ Плохо
void test1(void)
void test_battery(void)
```

---

## 🔍 Пример: Тестирование функции

### Исходная функция

```cpp
// src/drivers/battery/battery.cpp
int calculateBatteryPercentage(float voltage) {
    if (voltage >= 4.10) return 100;
    if (voltage >= 3.95) return 90 + (voltage - 3.95) * 100;
    // ...
    return 0;
}
```

### Тест

```cpp
// test/test_all.cpp
#include <unity.h>
#include "fixtures/mocks/arduino_mocks.cpp"
#include "src/drivers/battery/battery.cpp"

void test_battery_100_percent(void) {
    int result = calculateBatteryPercentage(4.20);
    TEST_ASSERT_EQUAL_INT(100, result);
}

void test_battery_0_percent(void) {
    int result = calculateBatteryPercentage(3.30);
    TEST_ASSERT_EQUAL_INT(0, result);
}

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_battery_100_percent);
    RUN_TEST(test_battery_0_percent);
    return UNITY_END();
}
```

### Запуск

```bash
pio test -e native-tests -v
```

---

## 📈 Метрики качества

### Целевые показатели

| Модуль | Целевое покрытие |
|--------|------------------|
| `battery.cpp` | 90% |
| `measurement_core.cpp` | 80% |
| `config.cpp` | 70% |
| Остальные | 50%+ |

---

## 🚀 Следующие шаги

1. **Запустить существующие тесты**
   ```bash
   pio test -e native-tests
   ```

2. **Добавить тесты для `config.cpp`**
   - Протестировать загрузку/сохранение настроек

3. **Добавить тесты для `transmitter_data.cpp`**
   - Протестировать обработку данных

4. **Настроить CI/CD**
   - Добавить шаг `pio test -e native-tests` в GitHub Actions

---

## 📚 Ресурсы

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [test/RUNNING_TESTS.md](RUNNING_TESTS.md) — Инструкция по запуску
- [test/QUICKSTART.md](QUICKSTART.md) — Быстрый старт
