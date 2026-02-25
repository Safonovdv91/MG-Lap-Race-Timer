// ============================================================
//  espnow_transmitter.cpp
//  Подключить в основной файл transmitter:
//    #include "espnow_transmitter.h"
//  Вызвать espnow_init() в setup() ВМЕСТО WiFi.begin()/udp.begin()
//  Заменить handleUDPPackets() на espnow_loop() в loop()
//  Убрать sendTelemetry() и вызов udp.*
// ============================================================

#include "espnow_transmitter.h"
#include "espnow_protocol.h"
#include <esp_now.h>
#include <WiFi.h>

// ---- Внешние зависимости из основного кода transmitter'а ----
extern float getBatteryVoltage();
extern int   getBatteryPercentage();

// ---- Внутренние переменные ----
static bool s_initialized = false;

// ---- Forward declarations ----
static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
static void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len);
static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt);

// ============================================================
//  Публичные функции
// ============================================================

void espnow_init() {
    // Transmitter работает в STA режиме.
    // WiFi.begin() НЕ нужен — ESP-NOW работает без ассоциации с AP.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // на всякий случай

    Serial.printf("[ESP-NOW] Transmitter MAC: %s\n",
                  WiFi.macAddress().c_str());

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Ошибка инициализации!");
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataRecv);

    // Добавляем receiver как peer.
    // ВАЖНО: используем SoftAP MAC receiver'а (00:4B:12:3B:B3:61)
    uint8_t receiverMac[] = MAC_RECEIVER;
    if (!addPeer(receiverMac, 0, false)) {
        Serial.println("[ESP-NOW] Не удалось добавить receiver как peer");
        return;
    }

    s_initialized = true;
    Serial.println("[ESP-NOW] Инициализация завершена (transmitter)");
}

// Вызывать из loop() — обрабатывает входящие пакеты через callback
void espnow_loop() {
    // ESP-NOW callbacks вызываются асинхронно в задаче WiFi.
    // Здесь можно добавить обработку очереди если понадобится.
    // Пока оставляем пустым — вся логика в onDataRecv.
}

// ============================================================
//  Приватные функции
// ============================================================

static void onDataSent(const uint8_t *mac_addr,
                       esp_now_send_status_t status) {
    Serial.printf("[ESP-NOW] Отправка: %s\n",
                  status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

static void onDataRecv(const uint8_t *mac_addr,
                       const uint8_t *data, int len) {
    if (len < 1) return;

    PacketType type = (PacketType)data[0];

    switch (type) {
        case PACKET_BATTERY_REQUEST: {
            if (len < (int)sizeof(BatteryRequestPacket)) {
                Serial.println("[ESP-NOW] Неверная длина BATTERY_REQUEST");
                return;
            }
            const BatteryRequestPacket *req =
                (const BatteryRequestPacket *)data;

            Serial.printf("[ESP-NOW] Получен запрос батареи (id=%d)\n",
                          req->requestId);

            // Формируем ответ
            BatteryResponsePacket resp;
            resp.type           = PACKET_BATTERY_RESPONSE;
            resp.requestId      = req->requestId; // echo
            resp.batteryLevel   = (uint8_t)getBatteryPercentage();
            resp.batteryVoltage = getBatteryVoltage();
            resp.reserved       = 0;

            uint8_t receiverMac[] = MAC_RECEIVER;
            esp_err_t result = esp_now_send(receiverMac,
                                            (uint8_t *)&resp, sizeof(resp));
            if (result != ESP_OK) {
                Serial.printf("[ESP-NOW] Ошибка отправки ответа: %d\n",
                              result);
            }
            break;
        }

        default:
            Serial.printf("[ESP-NOW] Неизвестный тип пакета: 0x%02X\n", type);
            break;
    }
}

static bool addPeer(const uint8_t *mac, uint8_t channel, bool encrypt) {
    if (esp_now_is_peer_exist(mac)) return true;

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = channel; // 0 = текущий канал
    peerInfo.encrypt = encrypt;
    peerInfo.ifidx   = WIFI_IF_STA; // transmitter работает как STA

    return esp_now_add_peer(&peerInfo) == ESP_OK;
}