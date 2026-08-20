#pragma once
#include <Arduino.h>

// ============================================================
//  espnow_broadcast.h
//  Стриминг состояния гонки в broadcast 2 раза/сек.
//  Только для RECEIVER_MODE.
//  Подключить в receiver_main.cpp:
//    #include "espnow_broadcast.h"
//  Вызвать espnow_broadcast_init() после espnow_init()
//  Вызвать espnow_broadcast_loop() из loop()
// ============================================================

// Инициализация. Вызвать после espnow_init().
void espnow_broadcast_init();

// Вызывать из loop() — отправляет пакет по таймеру.
void espnow_broadcast_loop();

// Изменить интервал трансляции в рантайме (мс, по умолчанию 500)
void espnow_broadcast_setInterval(uint32_t intervalMs);