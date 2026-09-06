// ============================================================
//  espnow_receiver.cpp  (v3 — матчинг requestId + мьютекс)
// ============================================================

#include "drivers/espnow_receiver.h"
#include "drivers/espnow_protocol.h"
#include "modules/transmitter_data.h"
#include <esp_now.h>
#include <WiFi.h>

// ---- Вспомогательные макросы ----
#define MAC_FMT "%02X:%02X:%02X:%02X:%02X:%02X"
#define MAC_ARG(mac) mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]
#define MAC_EQ(a,b) (memcmp((a),(b),6)==0)

// ---- Внутренние переменные ----
static uint8_t  s_requestId       = 0;
static bool     s_responsePending = false;
static uint32_t s_requestSentAt   = 0;
static uint8_t  s_pendingMac[6]   = {0}; // MAC ожидаемого отправителя
static uint8_t  s_pendingReqId    = 0;   // requestId ожидаемого ответа
static uint32_t s_lastTxRequest   = 0;

static const uint32_t REQUEST_TIMEOUT_MS  = 3000;
static const uint32_t TX_POLL_INTERVAL_MS = 10000;

// MAC device3 для ретрансляции
static uint8_t s_device3Mac[] = MAC_BROADCAST;
// static uint8_t s_device3Mac[] = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};

// ---- Forward declarations ----
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
static void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len);
static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt);
static void relayToDevice3(const BatteryResponsePacket *resp);

// ============================================================
//  Публичные функции
// ============================================================

void espnow_init() {
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Ошибка инициализации!");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    uint8_t txMac[] = MAC_TRANSMITTER;
    addPeer(txMac, 0, false);
    addPeer(s_device3Mac, 0, false);

    Serial.println("[ESP-NOW] Инициализация завершена (receiver)");
    Serial.printf("[ESP-NOW] SoftAP MAC: %s\n", WiFi.softAPmacAddress().c_str());
}

// ------------------------------------------------------------
//  Унифицированный запрос батареи — любой MAC
// ------------------------------------------------------------
void espnow_requestBattery(const uint8_t *targetMac) {
    if (s_responsePending) {
        Serial.println("[ESP-NOW] Запрос уже в ожидании, пропускаем");
        return;
    }
    if (!esp_now_is_peer_exist(targetMac)) {
        if (!addPeer(targetMac, 0, false)) {
            Serial.printf("[ESP-NOW] Не удалось добавить peer " MAC_FMT "\n",
                          MAC_ARG(targetMac));
            return;
        }
    }

    BatteryRequestPacket pkt;
    pkt.type      = PACKET_BATTERY_REQUEST;
    pkt.requestId = ++s_requestId;
    pkt.reserved  = 0;

    // Запоминаем ЧТО и ОТ КОГО ждём — матчинг ответа
    memcpy(s_pendingMac, targetMac, 6);
    s_pendingReqId = pkt.requestId;

    esp_err_t result = esp_now_send(targetMac, (uint8_t *)&pkt, sizeof(pkt));
    if (result == ESP_OK) {
        s_responsePending = true;
        s_requestSentAt   = millis();
        Serial.printf("[ESP-NOW] Запрос батареи → " MAC_FMT " (id=%d)\n",
                      MAC_ARG(targetMac), pkt.requestId);
    } else {
        memset(s_pendingMac, 0, 6);
        s_pendingReqId = 0;
        Serial.printf("[ESP-NOW] Ошибка отправки err=%d\n", result);
    }
}

// ------------------------------------------------------------
//  Периодический опрос transmitter'а — вызывать из loop()
// ------------------------------------------------------------
void handleTxReadBattery() {
    if (millis() - s_lastTxRequest < TX_POLL_INTERVAL_MS) return;
    s_lastTxRequest = millis();

    uint8_t txMac[] = MAC_TRANSMITTER;
    Serial.printf("[Battery] Опрос transmitter " MAC_FMT "\n",
                  MAC_ARG(txMac));
    espnow_requestBattery(txMac);
}

// ------------------------------------------------------------
//  Обработка таймаутов — вызывать из loop()
// ------------------------------------------------------------
void espnow_loop() {
    if (s_responsePending &&
        (millis() - s_requestSentAt > REQUEST_TIMEOUT_MS)) {
        Serial.printf("[ESP-NOW] Таймаут ответа от " MAC_FMT " (id=%d)\n",
                      MAC_ARG(s_pendingMac), s_pendingReqId);
        s_responsePending = false;
        memset(s_pendingMac, 0, 6);
        s_pendingReqId = 0;
    }
}

// ============================================================
//  Приватные функции
// ============================================================

static void onDataSent(const uint8_t *mac_addr,
                       esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.printf("[ESP-NOW] Ошибка доставки → " MAC_FMT "\n",
                      MAC_ARG(mac_addr));
    }
}

static void onDataRecv(const uint8_t *mac_addr,
                       const uint8_t *data, int len) {
    if (len < 1) return;

    PacketType type = (PacketType)data[0];

    switch (type) {
        case PACKET_BATTERY_RESPONSE: {
            if (len < (int)sizeof(BatteryResponsePacket)) {
                Serial.println("[ESP-NOW] Неверная длина BATTERY_RESPONSE");
                return;
            }
            const BatteryResponsePacket *resp =
                (const BatteryResponsePacket *)data;

            // ✅ Матчинг 1: проверяем requestId
            if (resp->requestId != s_pendingReqId) {
                Serial.printf("[ESP-NOW] Неожиданный requestId: got=%d expect=%d — игнорируем\n",
                              resp->requestId, s_pendingReqId);
                return;
            }

            // ✅ Матчинг 2: проверяем MAC отправителя
            if (!MAC_EQ(mac_addr, s_pendingMac)) {
                Serial.printf("[ESP-NOW] Ответ от неожиданного MAC " MAC_FMT " — игнорируем\n",
                              MAC_ARG(mac_addr));
                return;
            }

            // ✅ Потокобезопасная запись через хранилище
            transmitterData_set(resp->batteryLevel,
                                resp->batteryVoltage,
                                mac_addr);

            s_responsePending = false;
            memset(s_pendingMac, 0, 6);
            s_pendingReqId = 0;

            Serial.printf("[ESP-NOW] Батарея от " MAC_FMT ": %d%% (%.2fV)\n",
                          MAC_ARG(mac_addr),
                          resp->batteryLevel, resp->batteryVoltage);

            relayToDevice3(resp);
            break;
        }
        default:
            Serial.printf("[ESP-NOW] Неизвестный тип: 0x%02X\n", type);
            break;
    }
}

static void relayToDevice3(const BatteryResponsePacket *resp) {
    RelayDataPacket relay;
    relay.type           = PACKET_RELAY_DATA;
    relay.batteryLevel   = resp->batteryLevel;
    relay.batteryVoltage = resp->batteryVoltage;
    relay.reserved       = 0;

    esp_err_t result = esp_now_send(s_device3Mac,
                                    (uint8_t *)&relay, sizeof(relay));
    if (result != ESP_OK) {
        Serial.printf("[ESP-NOW] Ошибка ретрансляции: %d\n", result);
    }
}

static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt) {
    if (esp_now_is_peer_exist(mac)) return true;
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = channel;
    peer.encrypt = encrypt;
    peer.ifidx   = WIFI_IF_AP;
    return esp_now_add_peer(&peer) == ESP_OK;
}