# Архитектурный план разделения проекта на излучатель и приемник

## Текущее состояние
- В проекте SFMgTimer излучатель и приемник ИК сигнала находятся на одной ESP32
- Используется флаг `USE_IR_SENSORS` для включения ИК функциональности
- ИК передатчики работают на пинах 13 и 12 с ШИМ частотой 38 кГц
- ИК приемники подключены к пинам 14 и 27 как входы
- Есть веб-сервер с обработчиками API и интерфейсом

## Цель
Разделить проект на две независимые прошивки:
1. Излучатель (transmitter) - только передача ИК сигнала
2. Приемник (receiver) - прием ИК сигнала и веб-сервер для отображения данных

## Коммуникация
- Используется Wi-Fi с UDP для связи между устройствами
- Приемник будет получать информацию о состоянии излучателя (например, уровень заряда батареи)
- Используется общий SSID и пароль для Wi-Fi

## Архитектура излучателя (transmitter)

### Конфигурация (include/transmitter_config.h)
```cpp
// transmitter_config.h
#ifndef TRANSMITTER_CONFIG_H
#define TRANSMITTER_CONFIG_H

#include <Arduino.h>

// Пины для ИК передатчиков
#define IR_TX1_PIN 13  // Пин для управления первым ИК передатчиком
#define IR_TX2_PIN 12  // Пин для управления вторым ИК передатчиком

// Частота ШИМ для модуляции ИК сигнала (38 кГц)
#define IR_CARRIER_FREQ 38000
#define PWM_CHANNEL_1 0  // Канал ШИМ для первого передатчика
#define PWM_CHANNEL_2 1  // Канал ШИМ для второго передатчика
#define PWM_RESOLUTION 10 // Разрешение ШИМ (10 бит = 0-1023)

// Параметры Wi-Fi
#define WIFI_SSID "SFMTimer"
#define WIFI_PASSWORD "123456789"

// Параметры UDP
#define UDP_PORT 8888
#define RECEIVER_IP {192, 168, 4, 1} // IP адрес приемника (AP режим)
#define BEACON_INTERVAL 5000 // Интервал отправки beacon сообщений (мс)

// Пины для измерения напряжения батареи
#define BATTERY_PIN 34
#define BATTERY_MIN_V 3.0  // Минимальное напряжение (0%)
#define BATTERY_MAX_V 4.2  // Максимальное напряжение (100%)

#endif
```

### Инициализация излучателя (src/transmitter_main.cpp)
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "../include/transmitter_config.h"
#include "../include/ir_transmitter.h"

WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК передатчиков
  initIRTransmitters();
  
  // Подключение к Wi-Fi сети
  WiFi.mode(WIFI_STA); // Работаем в режиме станции
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("Wi-Fi подключен");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());
  
  // Инициализация UDP
  udp.begin(UDP_PORT);
}

unsigned long lastBeaconTime = 0;

void loop() {
  unsigned long currentTime = millis();
  
  // Отправка beacon сообщения каждые BEACON_INTERVAL миллисекунд
  if (currentTime - lastBeaconTime > BEACON_INTERVAL) {
    sendTelemetry();
    lastBeaconTime = currentTime;
  }
  
  // Обработка UDP пакетов (опционально)
  handleUDPPackets();
  
  delay(10);
}

void sendTelemetry() {
  // Получение данных о состоянии
  float batteryVoltage = analogRead(BATTERY_PIN) * 3.3 / 4095.0 * 2.0; // Предполагаем делитель 2:1
  int batteryLevel = map(batteryVoltage * 100, BATTERY_MIN_V * 100, BATTERY_MAX_V * 100, 0, 100);
  if (batteryLevel > 100) batteryLevel = 100;
  if (batteryLevel < 0) batteryLevel = 0;
  
  // Формирование и отправка UDP пакета
  IPAddress receiverIP(RECEIVER_IP);
  udp.beginPacket(receiverIP, UDP_PORT);
  udp.printf("TELEMETRY:%d:%.2fV", batteryLevel, batteryVoltage);
  udp.endPacket();
}

void handleUDPPackets() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    udp.read(incomingPacket, sizeof(incomingPacket));
    incomingPacket[packetSize] = '\0';
    
    // Обработка входящих команд (опционально)
    Serial.printf("Получено UDP сообщение: %s\n", incomingPacket);
  }
}
```

## Архитектура приемника (receiver)

### Конфигурация (include/receiver_config.h)
```cpp
// receiver_config.h
#ifndef RECEIVER_CONFIG_H
#define RECEIVER_CONFIG_H

#include <Arduino.h>

// Пины для ИК приемников
#define SENSOR1_PIN 14
#define SENSOR2_PIN 27
#define DEBOUNCE_TIME 1.0 * 1000000 // задержка от срабатывания датчиков пересечения 1сек
#define MIN_LAP_TIME 1.0 * 1000000

// Параметры Wi-Fi
#define WIFI_SSID "SFMTimer"
#define WIFI_PASSWORD "123456789"

// Параметры UDP
#define UDP_PORT 8888

// Светодиоды
#define STATUS_LED_PIN 2     // Встроенный светодиод (или другой пин)
#define MEASUREMENT_LED_PIN 4 // Пин для светодиода измерений
#define LED_BLINK_INTERVAL 2000 // Интервал моргания 2 секунды
#define LED_BLINK_DURATION 500  // Длительность моргания 0.5 секунды

#endif
```

### Основной файл приемника (src/receiver_main.cpp)
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <DNSServer.h>

#include "../include/receiver_config.h"
#include "../include/ir_receiver.h"
#include "../include/web_handlers.h"

WebServer server(80);
WiFiUDP udp;
DNSServer dnsServer;

// Переменные для хранения данных излучателя
struct TransmitterTelemetry {
  int batteryLevel = -1;
  float batteryVoltage = 0.0;
  unsigned long lastUpdate = 0;
} transmitterData;

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК приемников
  initIRReceivers();
  
  // Настройка пинов датчиков как входы для ИК приемников
  pinMode(SENSOR1_PIN, INPUT_PULLUP);
  pinMode(SENSOR2_PIN, INPUT_PULLUP);
  
  // При нормальной работе ИК луча на пинах будет LOW (есть сигнал)
  // При пересечении луча на пинах будет HIGH (нет сигнала)
  // Поэтому используем прерывание по RISING (по положительному фронту)
  attachInterrupt(digitalPinToInterrupt(SENSOR1_PIN), handleSensor1, RISING);
  attachInterrupt(digitalPinToInterrupt(SENSOR2_PIN), handleSensor2, RISING);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed. Formatting...");
  }

  // Загрузка сохраненных настроек wifi
  loadWiFiSettings();

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  IPAddress IP = WiFi.softAPIP();
  // Настройка DNS для перенаправления всех запросов
  dnsServer.start(53, "*", WiFi.softAPIP());
 // Установка имени хоста
  WiFi.setHostname("chrono.mg");

  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/api/v1/data", handleData);
  server.on("/api/v1/reset", handleReset);
  server.on("/api/v1/mode", HTTP_GET, handleMode);
  server.on("/api/v1/distance", HTTP_GET, handleDistance);
  
  server.on("/", handleRoot);
  server.on("/wifisettings", handleWiFiSettings);
  server.on("/updatewifi", HTTP_POST, handleUpdateWiFi);
  server.on("/style.css", handleCSS);
  server.on("/script.js", handleJS);

  server.begin();
  
  // Инициализация UDP
  udp.begin(UDP_PORT);
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest(); // Обработка DNS запросов
  
  // Обработка UDP пакетов от излучателя
  handleUDPPackets();
  
  // Обновление состояния измерений
  processMeasurements();
 updateSensorDisplay(); // состояние датчиков
}

void handleUDPPackets() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incomingPacket[255];
    udp.read(incomingPacket, sizeof(incomingPacket));
    incomingPacket[packetSize] = '\0';
    
    // Парсинг телеметрии излучателя
    if (strncmp(incomingPacket, "TELEMETRY:", 10) == 0) {
      sscanf(incomingPacket, "TELEMETRY:%d:%fV", &transmitterData.batteryLevel, &transmitterData.batteryVoltage);
      transmitterData.lastUpdate = millis();
      Serial.printf("Получена телеметрия излучателя: %d%% (%.2fV)\n", 
                    transmitterData.batteryLevel, transmitterData.batteryVoltage);
    }
  }
}

// Функции для получения телеметрии излучателя
int getTransmitterBatteryLevel() {
  // Если данных нет более 10 секунд, считаем что излучатель отключен
  if (millis() - transmitterData.lastUpdate > 10000) {
    return -1; // Сигнал о том, что излучатель недоступен
  }
  return transmitterData.batteryLevel;
}

float getTransmitterBatteryVoltage() {
  return transmitterData.batteryVoltage;
}
```

## Обновленный platformio.ini
```ini
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:transmitter]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = -DTRANSMITTER_MODE
src_filter = +<*> -<.git/> -<.svn/> -<example/> -<examples/> -<tests/> -<src/main.cpp> +<src/transmitter_main.cpp>

[env:receiver]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
monitor_speed = 115200
upload_speed = 921600
build_flags = -DRECEIVER_MODE
src_filter = +<*> -<.git/> -<.svn/> -<example/> -<examples/> -<tests/> -<src/main.cpp> +<src/receiver_main.cpp>
```

## Дополнительные изменения

1. Файлы `ir_transmitter.h` и `ir_transmitter.cpp` будут использоваться только в излучателе
2. Файлы `ir_receiver.h` и `ir_receiver.cpp` будут использоваться только в приемнике
3. Файлы `web_handlers.h`, `web_handlers.cpp`, `web_content.h`, `web_content.cpp` будут использоваться только в приемнике
4. Файл `measurements.h` и `measurements.cpp` будут адаптированы для работы с прерываниями только на приемнике

## Протокол UDP

Формат сообщений от излучателя к приемнику:
- TELEMETRY:battery_level:voltage.V (например: TELEMETRY:75:3.95V)

Это позволит приемнику отображать информацию о состоянии излучателя в веб-интерфейсе.