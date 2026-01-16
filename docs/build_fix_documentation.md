# Исправление ошибки сборки проекта SFMgTimer

## Проблема
При попытке собрать проект PlatformIO выдавала ошибку:
```
Error: Nothing to build. Please put your source code files to the '/home/safonov/Documents/PlatformIO/Projects/SFMgTimer/src' folder
```

Несмотря на наличие файлов в директории `/src`, сборка не выполнялась.

## Анализ
После анализа структуры проекта и файла `platformio.ini` было выявлено, что:

1. В конфигурации окружения `[env:transmitter]` в параметре `build_src_filter` были указаны только два файла: `src/transmitter_main.cpp` и `src/ir_transmitter.cpp`
2. Однако для полной сборки передатчика требуются дополнительные файлы, такие как `config.cpp` и `measurements.cpp`
3. В отличие от этого, в конфигурации `[env:receiver]` были правильно указаны все необходимые файлы

## Решение
Было изменено содержимое файла `platformio.ini`, а именно:

**До:**
```
[env:transmitter]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = -DTRANSMITTER_MODE
lib_deps = bblanchon/ArduinoJson@^6.18.3
build_src_filter = +<src/transmitter_main.cpp> +<src/ir_transmitter.cpp> -<.git/> -<.svn/> -<examples/> -<tests/>

[env:receiver]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = -DRECEIVER_MODE
lib_deps = bblanchon/ArduinoJson@^6.18.3
build_src_filter = +<src/receiver_main.cpp> +<src/ir_receiver.cpp> +<src/web_handlers.cpp> +<src/web_content.cpp> +<src/measurements.cpp> +<src/config.cpp> -<.git/> -<.svn/> -<examples/> -<tests/>
```

**После:**
```
[env:transmitter]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = -DTRANSMITTER_MODE
lib_deps = bblanchon/ArduinoJson@^6.18.3
src_filter = +<src/transmitter_main.cpp> +<src/ir_transmitter.cpp> +<src/config.cpp> +<src/measurements.cpp> -<.git/> -<.svn/> -<examples/> -<tests/>

[env:receiver]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = -DRECEIVER_MODE
lib_deps = bblanchon/ArduinoJson@^6.18.3
src_filter = +<src/receiver_main.cpp> +<src/ir_receiver.cpp> +<src/web_handlers.cpp> +<src/web_content.cpp> +<src/measurements.cpp> +<src/config.cpp> -<.git/> -<.svn/> -<examples/> -<tests/>
```

## Результат
Точное указание файлов для каждого окружения позволяет PlatformIO корректно собирать проект без конфликтов линковки, что должно устранить ошибку "Nothing to build".

## Дополнительная информация
Файл `transmitter_main.cpp` использует:
- Функции из `ir_transmitter.cpp` (например, `initIRTransmitters()`)
- Конфигурационные константы из `transmitter_config.h`
- Возможные функции из `config.cpp` и `measurements.cpp`, связанные с общими для проекта функциями

Файл `receiver_main.cpp` использует:
- Функции из `ir_receiver.cpp`, `web_handlers.cpp`, `web_content.cpp`, `measurements.cpp`, `config.cpp`

Все эти файлы теперь включены в процесс сборки для соответствующих окружений, без конфликта между файлами разных окружений.

## Архитектурные изменения

В результате анализа выявлена архитектурная проблема в проекте: дублирование определений переменных и функций между файлами `receiver_main.cpp` и `web_handlers.cpp`.

Для устранения конфликта линковки были выполнены следующие изменения в `src/receiver_main.cpp`:
- Удалено определение переменной `server` (заменено на внешнее объявление `extern WebServer server;`)
- Удалено дублирующее определение структуры `TransmitterTelemetry` (заменено на внешнее объявление)
- Удалены дублирующие функции `getTransmitterBatteryLevel()` и `getTransmitterBatteryVoltage()` (оставлены только в `web_handlers.cpp`)