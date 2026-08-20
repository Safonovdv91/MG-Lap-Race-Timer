// ============================================================
//  espnow_transmitter.cpp  (v2 — отложенное чтение батареи)
// ============================================================
#include <WiFi.h>

#include "drivers/espnow_transmitter.h"
#include "drivers/espnow_protocol.h"
#include "esp_now.h"

// ---- Внешние зависимости ----
extern float getBatteryVoltage();
extern int   getBatteryPercentage();

// ---- Внутренние переменные ----
static bool s_initialized = false;
// Флаг устанавливается в callback (WiFi task),
// обрабатывается в loop() (main task) — безопасное чтение ADC
static volatile bool    s_replyPending = false;
static volatile uint8_t s_pendingReqId = 0;

// ---- Forward declarations ----
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
// v2.x сигнатура ( для версии ядра 2):
static void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len);
// v3.x: static void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt);

// ============================================================
//  Публичные функции
// ============================================================

void espnow_init() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    Serial.printf("[ESP-NOW] Transmitter MAC: %s\n",
                  WiFi.macAddress().c_str());

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Ошибка инициализации!");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    uint8_t receiverMac[] = MAC_RECEIVER;
    if (!addPeer(receiverMac, 0, false)) {
        Serial.println("[ESP-NOW] Не удалось добавить receiver как peer");
        return;
    }

    s_initialized = true;
    Serial.println("[ESP-NOW] Инициализация завершена (transmitter)");
}

void espnow_loop() {
    // Обрабатываем отложенный запрос в main task —
    // ADC читается безопасно, вне WiFi callback
    if (!s_replyPending) return;
    s_replyPending = false;

    BatteryResponsePacket resp;
    resp.type           = PACKET_BATTERY_RESPONSE;
    resp.requestId      = s_pendingReqId;
    resp.batteryLevel   = (uint8_t)getBatteryPercentage();
    resp.batteryVoltage = getBatteryVoltage();
    resp.reserved       = 0;

    Serial.printf("[ESP-NOW] Отправка батареи: %d%% (%.2fV)\n",
                  resp.batteryLevel, resp.batteryVoltage);

    uint8_t receiverMac[] = MAC_RECEIVER;
    esp_err_t result = esp_now_send(receiverMac,
                                    (uint8_t *)&resp, sizeof(resp));
    if (result != ESP_OK) {
        Serial.printf("[ESP-NOW] Ошибка отправки ответа: %d\n", result);
    }
}

// ============================================================
//  Приватные функции
// ============================================================

static void onDataSent(const uint8_t *mac_addr,
                       esp_now_send_status_t status) {
    Serial.printf("[ESP-NOW] Доставка: %s\n",
                  status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// v2.x сигнатура
static void onDataRecv(const uint8_t *mac_addr,
                       const uint8_t *data, int len) {
    if (len < 1) return;

    PacketType type = (PacketType)data[0];

    switch (type) {
        case PACKET_BATTERY_REQUEST: {
            if (len < (int)sizeof(BatteryRequestPacket)) return;
            const BatteryRequestPacket *req =
                (const BatteryRequestPacket *)data;
            Serial.printf("[ESP-NOW] Запрос батареи (id=%d) — читаем в loop()\n",
                          req->requestId);
            // Только флаг — ADC НЕ читаем здесь!
            s_pendingReqId = req->requestId;
            s_replyPending = true;
            break;
        }
        case PACKET_RELAY_DATA:
            // Broadcast пакет для device3 — игнорируем
            break;

        default:
            Serial.printf("[ESP-NOW] Неизвестный тип: 0x%02X\n", type);
            break;
    }
}

static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt) {
    if (esp_now_is_peer_exist(mac)) return true;

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = channel;
    peerInfo.encrypt = encrypt;
    peerInfo.ifidx   = WIFI_IF_STA;

    return esp_now_add_peer(&peerInfo) == ESP_OK;
}