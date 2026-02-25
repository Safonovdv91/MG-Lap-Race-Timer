#pragma once
#include <Arduino.h>

// Инициализация ESP-NOW. Вызвать после WiFi.softAP(...)
void espnow_init();

// Запрос данных батареи у transmitter'а.
// Вызывать по событию (таймер, WebSocket-команда, кнопка и т.д.)
void espnow_requestBattery();

// Обработка таймаутов. Вызывать из loop()
void espnow_loop();