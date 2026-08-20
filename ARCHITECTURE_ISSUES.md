# Архитектурные проблемы проекта MG-Lap-Race-Timer

**Дата создания:** 20 февраля 2026  
**Статус:** Активный документ для отслеживания технического долга

---  Для продолжения этой сессии, выполните qwen --resume 4ae15635-b17d-4480-b5b9-cfc8339a5aa1


## 🔴 Критические (P0) — Требуют немедленного устранения

| # | Проблема | Файлы | Статус | Примечание |
|---|----------|-------|--------|------------|
| 0.1 | Глобальные `volatile` переменные без защиты от гонок | `measurements.cpp`, `measurements.h` | ✅ completed | Добавлены `getCurrentValueSafe()`, `getSensorActiveSafe()` |
| 0.2 | RACE CONDITION в `handleSensor()` ISR | `measurements.cpp` | ✅ completed | Добавлен `sensorTriggered` флаг, чтение в critical section |
| 0.3 | `resetMeasurements()` без critical section | `web_handlers.cpp` | ✅ completed | Обёрнуто в `lockMeasurements()/unlockMeasurements()` |
| 0.4 | Смешение логики и UI в `processMeasurements()` | `measurements.cpp` | ✅ completed | Выделено в `core/measurement_core.cpp` + `measurements_side_effects.cpp` |

---

## 🟠 Серьёзные (P1) — Устранить в ближайших итерациях

| # | Проблема | Файлы | Статус | Примечание |
|---|----------|-------|--------|------------|
| 1.1 | UDP вместо заявленного ESP-NOW | `receiver_main.cpp`, `transmitter_main.cpp` | ⬜ pending | В документации указан ESP-NOW, реализован UDP |
| 1.2 | Хардкод IP-адресов | `transmitter_main.cpp` | ✅ completed | Добавлен mDNS: `chrono.mg` |
| 1.3 | Нет обработки потерь Wi-Fi | `transmitter_main.cpp` | ⬜ pending | Отсутствует механизм реконнекта |
| 1.4 | `Serial.print()` в функциях получения данных | `battery.cpp` | ⬜ pending | Лишние аллокации, замедление работы |
| 1.5 | Статический `JsonDocument<1024>` может переполниться | `websocket_handlers.cpp` | ✅ completed | Увеличен до 1200, история ограничена 5 записями |
| 1.6 | SPIFFS: чтение HTML в `String` | `web_handlers.cpp` | ✅ completed | HTML отправляется через `streamFile()`, данные через WebSocket |

---

## 🟡 Дизайн-проблемы (P2) — Улучшения архитектуры

| # | Проблема | Файлы | Статус | Примечание |
|---|----------|-------|--------|------------|
| 2.1 | Дублирование конфигураций | `config.h`, `receiver_config.h`, `transmitter_config.h` | ⬜ pending | Пересекающиеся определения пинов, портов |
| 2.2 | Магические числа | `measurements.cpp`, `receiver_config.h` | ⬜ pending | `1.488`, `300000`, `4.0*1000` без имён |
| 2.3 | Нет модульности — большие файлы | `measurements.cpp` (300+ строк) | ⬜ pending | Разная ответственность в одном файле |
| 2.4 | C-стиль кода в C++ проекте | Все `.cpp` | ⬜ pending | Массивы вместо `std::array`, `sscanf`, ручное управление памятью |
| 2.5 | Глобальные переменные без инкапсуляции | `web_handlers.cpp`, `measurements.cpp` | ⬜ pending | `extern Mode currentMode`, `historyIndex` |

---

## 🟢 Потенциальные баги (P3) — Профилактика

| # | Проблема | Файлы | Статус | Примечание |
|---|----------|-------|--------|------------|
| 3.1 | Неиспользуемая переменная `beamBroken` | `measurements.cpp` | ⬜ pending | Объявлена, но логика работает через `beamBrokenNow` |
| 3.2 | Нет debounce для кнопок | `receiver_config.h` | ⬜ pending | GPIO 18, 19 зарезервированы, но нет обработки |
| 3.3 | Нет валидации входных данных | `web_handlers.cpp` | ⬜ pending | `handleUpdateWiFi()` — проверка длины, но не символов |
| 3.4 | Жёсткая привязка к пинам в коде | `receiver_config.h` | ⬜ pending | Сложно портировать на другую плату |

---

## 📋 Чеклист устранения

### Фаза 1: Критические исправления (P0)
- [x] **0.1** Добавить critical sections для всех операций с `volatile` переменными
- [x] **0.2** Рефакторинг `handleSensor()` — минимизировать логику в ISR
- [x] **0.3** Обернуть `resetMeasurements()` в critical section
- [x] **0.4** Выделить side effects (WebSocket, Serial) из `processMeasurements()`

### Фаза 2: Стабилизация связи (P1)
- [ ] **1.1** Интегрировать ESP-NOW вместо UDP
- [x] **1.2** Заменить хардкод IP на mDNS (`chrono.mg`)
- [ ] **1.3** Добавить автоматический реконнект Wi-Fi с экспоненциальной задержкой
- [ ] **1.4** Удалить `Serial.print()` из getter'ов, вынести в отдельную функцию логгирования
- [x] **1.5** Увеличить `JsonDocument` до 1200 байт (история ограничена 5 записями)
- [x] **1.6** Упростить код: HTML через `streamFile()`, данные через WebSocket (без замен на сервере)

### Фаза 3: Рефакторинг архитектуры (P2)
- [ ] **2.1** Создать единый `system_config.h` с наследованием для TX/RX
- [ ] **2.2** Заменить `#define` на `constexpr`, дать имена константам
- [ ] **2.3** Разделить `measurements.cpp` на модули:
  - `src/core/timer_fsm.cpp`
  - `src/core/sensor_reader.cpp`
  - `src/core/measurement_core.cpp`
- [ ] **2.4** Использовать `std::array`, `std::optional`, RAII
- [ ] **2.5** Инкапсулировать состояние в классы/структуры

### Фаза 4: Полировка (P3)
- [ ] **3.1** Удалить или использовать `beamBroken`
- [ ] **3.2** Добавить программный debounce для кнопок
- [ ] **3.3** Добавить валидацию SSID/password (символы, спецсимволы)
- [ ] **3.4** Вынести pinout в отдельный файл конфигурации платы

---

## История изменений

| Дата | Изменение | Автор |
|------|-----------|-------|
| 20.02.2026 | Создан документ, добавлены все выявленные проблемы | Qwen Code |
| 20.02.2026 | Исправлено **0.1**: Добавлены функции безопасного доступа `getCurrentValueSafe()`, `getSensorActiveSafe()` | Qwen Code |
| 20.02.2026 | Исправлено **0.3**: `resetMeasurements()` обёрнуто в critical section | Qwen Code |
| 20.02.2026 | Исправлено **0.2**: Рефакторинг `handleSensor()` — добавлен флаг `sensorTriggered`, всё чтение из ISR в critical section | Qwen Code |
| 20.02.2026 | Исправлено **0.4**: Выделение core логики в `src/core/measurement_core.cpp`, side effects в `measurements_side_effects.cpp` | Qwen Code |
| 20.02.2026 | Исправлено **1.2**: mDNS для разрешения IP (`chrono.mg`) | Qwen Code |
| 20.02.2026 | Исправлено **1.5**: `StaticJsonDocument` увеличен до 1200 байт, история ограничена 5 записями | Qwen Code |
| 20.02.2026 | Исправлено **1.6**: `handleCSS/JS` используют `streamFile()`, HTML файлы ограничены ~2KB | Qwen Code |
| 20.02.2026 | **P2.3 Упрощение**: HTML отправляется через `streamFile()` без замен, данные обновляются через WebSocket | Qwen Code |

---

## Приоритеты работы

```
P0 > P1 > P2 > P3
```

**Правило:** Не переходить к следующей категории, пока все проблемы текущей не устранены или не приняты осознанные решения об отложенном исправлении.
