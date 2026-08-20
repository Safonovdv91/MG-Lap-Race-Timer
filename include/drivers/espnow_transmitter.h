#pragma once
#include <Arduino.h>

// Инициализация ESP-NOW. Вызвать в setup() вместо WiFi.begin()
void espnow_init();

// Обработка входящих пакетов. Вызывать из loop()
void espnow_loop();