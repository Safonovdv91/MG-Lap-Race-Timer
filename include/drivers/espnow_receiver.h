#pragma once

// Для тестов: если определено UNIT_TEST, используем моки вместо Arduino.h
#ifdef UNIT_TEST
#include "fixtures/mocks/arduino_mocks.h"
#else
#include <Arduino.h>
#endif

// Инициализация ESP-NOW. Вызвать после WiFi.softAP(...)
void espnow_init();

// Запрос батареи у устройства с указанным MAC.
// Можно вызывать для любого ESP устройства.
// Пример: uint8_t mac[] = MAC_TRANSMITTER; espnow_requestBattery(mac);
void espnow_requestBattery(const uint8_t *targetMac);

// Вызывать из loop():

// Обработка таймаутов
void espnow_loop();

// опрос заряда батареи у трансмиттеров
void handleTxReadBattery();
