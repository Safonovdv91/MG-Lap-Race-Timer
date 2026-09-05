// ============================================================
//  espnow_broadcast.cpp
//  Стриминг RaceStatePacket в broadcast, 2 раза/сек.
//  Потери пакетов допустимы — fire and forget, без подтверждений.
// ============================================================
#include "drivers/espnow_broadcast.h"
#include "drivers/espnow_protocol.h"
#include "core/measurement_core.h"
#include <esp_now.h>
#include <WiFi.h>


// ---- Внешние зависимости из основного кода receiver'а ----
extern unsigned long long getCurrentRaceTimeSafe();  // microseconds
extern float           getCurrentValueSafe();       // не используем, но пусть будет
extern TimerStatus   getTimerStatus();
extern Mode           currentMode;                 // или getter если есть

// ---- Внутренние переменные ----
static uint32_t s_intervalMs  = 500;   // 2 раза в секунду
static uint32_t s_lastSentAt  = 0;
static bool     s_initialized = false;

static uint8_t s_broadcastMac[] = MAC_BROADCAST;

// ---- Forward declarations ----
static void onBroadcastSent(const uint8_t *mac_addr,
                            esp_now_send_status_t status);

// ============================================================

void espnow_broadcast_init() {
    // Broadcast peer — добавляем один раз
    if (!esp_now_is_peer_exist(s_broadcastMac)) {
        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, s_broadcastMac, 6);
        peer.channel = 0;
        peer.encrypt = false;
        peer.ifidx   = WIFI_IF_AP;

        if (esp_now_add_peer(&peer) != ESP_OK) {
            Serial.println("[Broadcast] Ошибка добавления broadcast peer!");
            return;
        }
    }

    // Отдельный send callback для broadcast чтобы не спамить лог
    // (общий onDataSent из espnow_receiver.cpp уже зарегистрирован,
    //  дополнительный регистрировать не нужно — ESP-NOW поддерживает
    //  только один глобальный send callback)

    s_initialized = true;
    Serial.printf("[Broadcast] Инициализация завершена, интервал %dмс\n",
                  s_intervalMs);
}

void espnow_broadcast_setInterval(uint32_t intervalMs) {
    s_intervalMs = intervalMs;
}

void espnow_broadcast_loop() {
    if (!s_initialized) return;
    if (millis() - s_lastSentAt < s_intervalMs) return;
    s_lastSentAt = millis();

    // Собираем пакет из текущего состояния
    RaceStatePacket pkt;
    pkt.type     = PACKET_RACE_STATE;
    pkt.mode     = (uint8_t)currentMode;
    pkt.reserved = 0;

    TimerStatus status = getTimerStatus();
    switch (status) {
        case STATUS_READY:   pkt.timerStatus = RACE_STATUS_READY;   break;
        case STATUS_RUNNING: pkt.timerStatus = RACE_STATUS_RUNNING; break;
        case STATUS_DISPLAY: pkt.timerStatus = RACE_STATUS_DISPLAY; break;
        default:             pkt.timerStatus = RACE_STATUS_READY;   break;
    }

    if (getMeasurementInProgressSafe()) {
        pkt.raceTime = getCurrentRaceTimeSafe() / 1000000.0f;
    } else {
        pkt.raceTime = getCurrentValueSafe();
    }
    esp_now_send(s_broadcastMac, (uint8_t *)&pkt, sizeof(pkt));
    // Ошибки не логируем — стриминг, потери допустимы
}