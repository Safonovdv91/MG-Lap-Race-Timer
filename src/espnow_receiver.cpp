// ============================================================
//    #include "espnow_receiver.h"
//  Вызвать espnow_init() после WiFi.softAP(...)
//  Заменить handleUDPPackets() на espnow_loop() в loop()
// ============================================================

#include "espnow_receiver.h"
#include "espnow_protocol.h"
#include <esp_now.h>
#include <WiFi.h>

// ---- Внешние зависимости из основного кода ----
// Структура с данными transmitter'а (определена в основном файле)

extern struct TransmitterTelemetry {
  int batteryLevel;
  float batteryVoltage;
  unsigned long lastUpdate;
} transmitterData;

// ---- Внутренние переменные ----
static uint8_t  s_requestId       = 0;
static bool     s_responsePending = false;
static uint32_t s_requestSentAt   = 0;
static const uint32_t REQUEST_TIMEOUT_MS = 3000;

// MAC устройства #3 для ретрансляции.
// Замените на реальный MAC или оставьте broadcast.
static uint8_t s_device3Mac[] = MAC_BROADCAST;
// static uint8_t s_device3Mac[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

// ---- Forward declarations ----
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
static void onDataRecv(const uint8_t *mac_addr,
                       const uint8_t *data, int len);
static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt);
static void relayToDevice3(const BatteryResponsePacket *resp);

// ============================================================
//  Публичные функции
// ============================================================

void espnow_init() {
    // ESP-NOW требует WiFi уже поднятого.
    // Receiver работает в WIFI_AP_STA для совместимости ESP-NOW + SoftAP.
    // WiFi.softAP() должен быть вызван ДО этой функции.
    WiFi.mode(WIFI_AP_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Ошибка инициализации!");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    // Добавляем transmitter как peer
    uint8_t transmitterMac[] = MAC_TRANSMITTER;
    if (!addPeer(transmitterMac, 0, false)) {
        Serial.println("[ESP-NOW] Не удалось добавить transmitter как peer");
    }

    // Добавляем device3 (или broadcast)
    if (!addPeer(s_device3Mac, 0, false)) {
        Serial.println("[ESP-NOW] Не удалось добавить device3 как peer");
    }

    Serial.println("[ESP-NOW] Инициализация завершена (receiver)");
    Serial.printf("[ESP-NOW] SoftAP MAC: %s\n",
                  WiFi.softAPmacAddress().c_str());
}

// Запрос данных батареи у transmitter'а.
// Вызывайте по таймеру или по событию (кнопка, WebSocket-команда и т.д.)
void espnow_requestBattery() {
    if (s_responsePending) {
        Serial.println("[ESP-NOW] Запрос уже в ожидании, пропускаем");
        return;
    }

    uint8_t transmitterMac[] = MAC_TRANSMITTER;
    BatteryRequestPacket pkt;
    pkt.type      = PACKET_BATTERY_REQUEST;
    pkt.requestId = ++s_requestId;
    pkt.reserved  = 0;

    esp_err_t result = esp_now_send(transmitterMac,
                                    (uint8_t *)&pkt, sizeof(pkt));
    if (result == ESP_OK) {
        s_responsePending = true;
        s_requestSentAt   = millis();
        Serial.printf("[ESP-NOW] Запрос батареи отправлен (id=%d)\n",
                      pkt.requestId);
    } else {
        Serial.printf("[ESP-NOW] Ошибка отправки запроса: %d\n", result);
    }
}

// Вызывать из loop() для обработки таймаутов
void espnow_loop() {
    if (s_responsePending &&
        (millis() - s_requestSentAt > REQUEST_TIMEOUT_MS)) {
        Serial.println("[ESP-NOW] Таймаут ответа от transmitter'а");
        s_responsePending = false;
    }
}

// ============================================================
//  Приватные функции
// ============================================================

static void onDataSent(const uint8_t *mac_addr,
                       esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.printf("[ESP-NOW] Ошибка доставки пакета к "
                      "%02X:%02X:%02X:%02X:%02X:%02X\n",
                      mac_addr[0], mac_addr[1], mac_addr[2],
                      mac_addr[3], mac_addr[4], mac_addr[5]);
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

            // Обновляем данные transmitter'а (та же структура, что была с UDP)
            transmitterData.batteryLevel   = resp->batteryLevel;
            transmitterData.batteryVoltage = resp->batteryVoltage;
            transmitterData.lastUpdate     = millis();
            s_responsePending              = false;

            Serial.printf("[ESP-NOW] Батарея transmitter: %d%% (%.2fV)\n",
                          resp->batteryLevel, resp->batteryVoltage);

            // Ретранслируем на device3
            relayToDevice3(resp);
            break;
        }

        default:
            Serial.printf("[ESP-NOW] Неизвестный тип пакета: 0x%02X\n", type);
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
        Serial.printf("[ESP-NOW] Ошибка ретрансляции на device3: %d\n",
                      result);
    } else {
        Serial.println("[ESP-NOW] Данные ретранслированы на device3");
    }
}

static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt) {
    if (esp_now_is_peer_exist(mac)) return true;

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = channel;  // 0 = текущий канал
    peerInfo.encrypt = encrypt;
    peerInfo.ifidx   = WIFI_IF_AP; // важно! receiver работает как AP

    return esp_now_add_peer(&peerInfo) == ESP_OK;
}