# 🧪 Тестирование MG-Lap-Race-Timer

## 📖 Оглавление

1. [Быстрый старт](#быстрый-старт)
2. [Структура проекта](#структура-проекта)
3. [Структура тестов](#структура-тестов)
4. [Команды запуска](#команды-запуска)
5. [Написание новых тестов](#написание-новых-тестов)
6. [Моки и заглушки](#моки-и-заглушки)

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
pio test -e native-tests
```

### 3. Запуск с подробным выводом

```bash
pio test -e native-tests -v
```

**Ожидаемый результат:**
```
test/test_all.cpp:187:test_battery_100_percent:PASS
test/test_all.cpp:188:test_battery_90_percent:PASS
...
-----------------------
16 Tests 0 Failures 0 Ignored 
OK
```

---

## 📁 Структура проекта

```
project/
├── include/              # Публичные заголовки
│   ├── core/             # Ядро системы
│   ├── drivers/          # Драйверы периферии
│   ├── modules/          # Бизнес-логика модулей
│   └── utils/            # Утилиты (конфиги)
├── src/                  # Исходный код
│   ├── core/             # Ядро системы (main файлы)
│   ├── drivers/          # Драйверы периферии
│   ├── modules/          # Бизнес-логика модулей
│   └── utils/            # Утилиты
├── test/                 # Тесты
│   ├── test_all.cpp      # Все unit-тесты
│   └── fixtures/
│       └── mocks/        # Заглушки и моки
├── scripts/              # Скрипты деплоя и CI
├── platformio.ini        # Конфигурация сборки
└── README.md             # Документация
```

### Детализация по директориям

| Директория | Описание | Примеры |
|------------|----------|---------|
| `src/core/` | Ядро системы, main файлы | `receiver_main.cpp`, `tx_main.cpp` |
| `src/drivers/` | Драйверы периферии | `battery/`, `espnow_*.cpp`, `ir_transmitter.cpp` |
| `src/modules/` | Бизнес-логика | `web_handlers.cpp`, `transmitter_data.cpp` |
| `src/utils/` | Утилиты | `config.cpp` |
| `include/core/` | Заголовки ядра | `measurement_core.h` |
| `include/drivers/` | Заголовки драйверов | `espnow_receiver.h`, `battery/` |
| `include/modules/` | Заголовки модулей | `web_handlers.h`, `measurements.h` |
| `include/utils/` | Заголовки утилит | `config.h`, `receiver_config.h` |
| `test/fixtures/mocks/` | Моки для тестов | `arduino_mocks.cpp`, `arduino_mocks.h` |

---

## 📁 Структура тестов

```
test/
├── test_all.cpp              # Все тесты (16 тестов)
├── fixtures/
│   └── mocks/
│       ├── arduino_mocks.h   # Заголовки моков
│       └── arduino_mocks.cpp # Реализация моков
├── RUNNING_TESTS.md          # Этот файл
├── TESTING_PLAN.md           # План тестирования
├── QUICKSTART.md             # Быстрый старт
└── README.md                 # Общая документация
```

### Тестируемые модули

| Модуль | Файл | Кол-во тестов |
|--------|------|---------------|
| `battery.cpp` | `src/drivers/battery/battery.cpp` | 9 тестов |
| `measurement_core.cpp` | `src/core/measurement_core.cpp` | 7 тестов |

---

## 🎯 Команды запуска

| Команда | Описание |
|---------|----------|
| `pio test -e native-tests` | Запуск всех тестов |
| `pio test -e native-tests -v` | Подробный вывод (verbose) |
| `pio test -e native-tests --without-building` | Запуск без пересборки |
| `pio test -e native-tests --without-testing` | Только сборка, без запуска |

### ❌ Не работает (устарело)

```bash
# Эти команды НЕ работают - используйте native-tests
pio test -e native          # НЕ РАБОТАЕТ
pio test -e native -f test_battery  # НЕ РАБОТАЕТ
```

---

## ✍️ Написание новых тестов

### Шаг 1: Добавить тест в test/test_all.cpp

Открой `test/test_all.cpp` и добавь новую тестовую функцию:

```cpp
void test_my_function(void) {
    int result = myFunction(5);
    TEST_ASSERT_EQUAL_INT(10, result);
}
```

### Шаг 2: Добавить тест в runUnityTests()

```cpp
int runUnityTests(void) {
    UNITY_BEGIN();
    
    // ... существующие тесты
    
    // Новый тест
    RUN_TEST(test_my_function);
    
    return UNITY_END();
}
```

### Шаг 3: Запустить тест

```bash
pio test -e native-tests -v
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

Моки находятся в `test/fixtures/mocks/` и предоставляют заглушки для:

- `Serial` — вывод отладочной информации
- `millis()`, `micros()` — функции времени
- `analogRead()`, `analogSetPinAttenuation()` — АЦП
- `portENTER_CRITICAL()`, `portEXIT_CRITICAL()` — критические секции
- `Preferences` — хранение настроек
- `pinMode()`, `digitalWrite()`, `digitalRead()` — GPIO

### Как это работает

```cpp
// В исходных файлах (.cpp, .h)
#ifdef UNIT_TEST
    #include "fixtures/mocks/arduino_mocks.h"  // Моки для тестов
#else
    #include <Arduino.h>              // Реальный Arduino.h для ESP32
#endif
```

---

## 📊 Покрытие кода

### Текущий статус

| Модуль | Статус | Файл теста |
|--------|--------|------------|
| `battery.cpp` | ✅ Готово | `test/test_all.cpp` |
| `measurement_core.cpp` | ✅ Готово | `test/test_all.cpp` |
| `config.cpp` | ⏳ Ожидает | - |
| `transmitter_data.cpp` | ⏳ Ожидает | - |
| `espnow_*.cpp` | ⏳ Ожидает | - |
| `web_*.cpp` | ⏳ Ожидает | - |

**Пройдено тестов:** 16 ✅

### План покрытия

1. **Приоритет 1** (чистая логика, без зависимостей):
   - ✅ `battery.cpp` → `calculateBatteryPercentage()`
   - ✅ `measurement_core.cpp` → `addToHistory()`, `handleCooldown()`, `updateLiveTimer()`

2. **Приоритет 2** (требуют моков):
   - ⏳ `config.cpp` → загрузка/сохранение настроек
   - ⏳ `transmitter_data.cpp` → обработка данных

3. **Приоритет 3** (интеграционные тесты):
   - ⏳ `espnow_*.cpp` → требуют моков ESP-NOW
   - ⏳ `web_*.cpp` → требуют моков WebSocket

---

## 🐛 Отладка тестов

### Ошибки компиляции

```bash
# Ошибка: Arduino.h не найден
# Решение: Убедитесь что определён UNIT_TEST в platformio.ini

# Ошибка: undefined reference
# Решение: Проверьте что моки подключены в test_all.cpp
```

### Ошибки выполнения

```bash
# Segmentation fault
# Причина: доступ к памяти ESP32 в нативном тесте
# Решение: Используйте моки для hardware-specific кода
```

### Тесты не запускаются

**Решение:**

```bash
# Очистите кэш и пересоберите
rm -rf .pio
pio test -e native-tests -v
```

---

## 📚 Ресурсы

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [test/TESTING_PLAN.md](TESTING_PLAN.md) — План тестирования
- [test/QUICKSTART.md](QUICKSTART.md) — Быстрый старт

---

## ❓ FAQ

**Q: Почему все тесты в одном файле?**  
A: Для упрощения сборки в PlatformIO. Все тесты компилируются вместе с моками.

**Q: Как тестировать код с Serial?**  
A: Моки предоставляют `SerialMock` который игнорирует вывод (или выводит в stdout при `DEBUG_TESTS`).

**Q: Можно ли запускать тесты в CI/CD?**  
A: Да! Добавьте шаг `pio test -e native-tests` в ваш pipeline (GitHub Actions, GitLab CI, etc.).

**Q: Как добавить новый тест?**  
A: См. раздел [Написание новых тестов](#написание-новых-тестов).
