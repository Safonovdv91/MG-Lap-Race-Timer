# ⚡ Быстрый старт: Запуск тестов

## 1️⃣ Установка PlatformIO

```bash
# macOS
brew install platformio

# Или через pip
pip3 install platformio
```

## 2️⃣ Запуск тестов

```bash
# Перейдите в директорию проекта
cd /Users/safonov/Documents/MG-Lap-Race-Timer

# Запуск всех тестов
pio test -e native-tests

# Подробный вывод
pio test -e native-tests -v
```

## 3️⃣ Ожидаемый результат

```
test/test_all.cpp:187:test_battery_100_percent:PASS
test/test_all.cpp:188:test_battery_90_percent:PASS
test/test_all.cpp:189:test_battery_50_percent:PASS
...
-----------------------
16 Tests 0 Failures 0 Ignored 
OK
```

## 📚 Документация

- **test/TESTING_PLAN.md** — План покрытия кода тестами
- **test/README.md** — Общая документация по тестам

## 🆘 Помощь

Если тесты не запускаются, см. раздел "Решение проблем" в **test/TESTING_PLAN.md**
