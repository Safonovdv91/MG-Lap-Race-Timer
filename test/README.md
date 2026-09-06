# 🧪 Тестирование MG-Lap-Race-Timer

## 📖 Оглавление

1. [Быстрый старт](#быстрый-старт)
2. [Структура тестов](#структура-тестов)
3. [Команды запуска](#команды-запуска)
4. [Написание новых тестов](#написание-новых-тестов)
5. [Моки и заглушки](#моки-и-заглушки)

---

## 🚀 Быстрый старт

### 1. Установка зависимостей

```bash
# Установите PlatformIO CLI (если не установлен)
pip install platformio

# Или через Homebrew (macOS)
brew install platformio
```

### 2. Запуск всех тестов

```bash
pio test
```

### 3. Запуск конкретного теста

```bash
# Тесты батареи
pio test -f test_battery

# Тесты ядра измерений
pio test -f test_measurement_core
```

### 4. Запуск с подробным выводом

```bash
pio test -v
```

---

## 📁 Структура тестов

```
test/
├── battery/
│   └── test_battery.cpp           # Тесты модуля battery.cpp
├── core/
│   └── test_measurement_core.cpp  # Тесты measurement_core.cpp
├── mocks/
│   └── arduino_mocks.h            # Заглушки Arduino API (TODO)
└── README.md                      # Этот файл
```

---

## 🎯 Команды запуска

| Команда | Описание |
|---------|----------|
| `pio test` | Запуск всех тестов |
| `pio test -f test_battery` | Запуск тестов батареи |
| `pio test -f test_measurement_core` | Запуск тестов ядра измерений |
| `pio test -v` | Подробный вывод (verbose) |
| `pio test --without-building` | Запуск без пересборки |
| `pio test --without-testing` | Только сборка, без запуска |

---

## ✍️ Написание новых тестов

### Шаблон теста

Создайте файл `test/<module>/test_<name>.cpp`:

```cpp
#include <unity.h>
#include <Arduino.h>

// Подключаем тестируемый модуль
#include "path/to/module.h"

// Тестовая функция
void test_example(void) {
    int result = someFunction(5);
    TEST_ASSERT_EQUAL_INT(10, result);
}

// Настройка (перед каждым тестом)
void setUp(void) {
    // Инициализация
}

// Очистка (после каждого теста)
void tearDown(void) {
    // Очистка
}

// Главная функция
int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_example);
    return UNITY_END();
}

void setup() {
    runUnityTests();
}

void loop() {}

int main(int argc, char **argv) {
    return runUnityTests();
}
```

### Основные макросы Unity

| Макрос | Описание |
|--------|----------|
| `TEST_ASSERT_EQUAL_INT(expected, actual)` | Проверка целых чисел |
| `TEST_ASSERT_EQUAL_FLOAT(expected, actual)` | Проверка float |
| `TEST_ASSERT_TRUE(condition)` | Проверка истинности |
| `TEST_ASSERT_NULL(pointer)` | Проверка на NULL |
| `TEST_ASSERT_NOT_NULL(pointer)` | Проверка на не-NULL |
| `TEST_ASSERT_GREATER_OR_EQUAL_INT(min, actual)` | Проверка >= |
| `TEST_ASSERT_LESS_OR_EQUAL_INT(max, actual)` | Проверка <= |

---

## 🔧 Моки и заглушки

### Проблема

Код использует Arduino/ESP32 API (`Serial`, `micros`, `millis`, `portENTER_CRITICAL`), 
которые недоступны в нативных тестах.

### Решение (TODO)

Создайте `test/mocks/arduino_mocks.h`:

```cpp
#ifndef ARDUINO_MOCKS_H
#define ARDUINO_MOCKS_H

#include <stdint.h>
#include <stdio.h>

// Мок для Serial
struct SerialMock {
    void printf(const char* fmt, ...) {
        // Пустая реализация для тестов
    }
};
extern SerialMock Serial;

// Мок для millis/micros
extern unsigned long mock_millis_time;
extern unsigned long mock_micros_time;

unsigned long millis() { return mock_millis_time; }
unsigned long micros() { return mock_micros_time; }

// Мок для критических секций
#define portENTER_CRITICAL(x)
#define portEXIT_CRITICAL(x)
#define portENTER_CRITICAL_ISR(x)
#define portEXIT_CRITICAL_ISR(x)

// Мок для IRAM_ATTR
#define IRAM_ATTR

#endif
```

---

## 📊 Покрытие кода

### Текущий статус

| Модуль | Статус | Файл теста |
|--------|--------|------------|
| `battery.cpp` | ✅ Готово | `test/battery/test_battery.cpp` |
| `measurement_core.cpp` | ✅ Готово | `test/core/test_measurement_core.cpp` |
| `config.cpp` | ⏳ Ожидает | - |
| `espnow_*.cpp` | ⏳ Ожидает | - |
| `web_*.cpp` | ⏳ Ожидает | - |

### План покрытия

1. **Приоритет 1** (чистая логика, без зависимостей):
   - ✅ `battery.cpp` → `calculateBatteryPercentage()`
   - ✅ `measurement_core.cpp` → `addToHistory()`, `handleCooldown()`, `updateLiveTimer()`

2. **Приоритет 2** (требуют моков):
   - `config.cpp` → загрузка/сохранение настроек
   - `transmitter_data.cpp` → обработка данных

3. **Приоритет 3** (интеграционные тесты):
   - `espnow_*.cpp` → требуют моков ESP-NOW
   - `web_*.cpp` → требуют моков WebSocket

---

## 🐛 Отладка тестов

### Ошибки компиляции

```bash
# Ошибка: Arduino.h не найден
# Решение: PlatformIO автоматически подключает фреймворк
# Проверьте platformio.ini

# Ошибка: undefined reference
# Решение: Добавьте исходный файл в test_src_filter
```

### Ошибки выполнения

```bash
# Segmentation fault
# Причина: доступ к памяти ESP32 в нативном тесте
# Решение: Используйте моки для hardware-specific кода
```

---

## 📚 Ресурсы

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [CppUTest](https://cpputest.github.io/) - альтернатива для C++

---

## ❓ FAQ

**Q: Тесты не запускаются на ESP32?**  
A: PlatformIO Test может запускать тесты как на хосте (native), так и на устройстве. 
   Для unit-тестов используйте native-режим (по умолчанию).

**Q: Как тестировать код с Serial?**  
A: Создайте мок для Serial (см. раздел [Моки и заглушки](#моки-и-заглушки)).

**Q: Можно ли запускать тесты в CI/CD?**  
A: Да! Добавьте шаг `pio test` в ваш pipeline (GitHub Actions, GitLab CI, etc.).
