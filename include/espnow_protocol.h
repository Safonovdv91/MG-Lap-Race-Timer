#pragma once
#include <stdint.h>

// ============================================================
//  ESP-NOW Protocol — общий заголовок для Receiver и Transmitter
// ============================================================

// --- MAC адреса устройств ---
#define MAC_RECEIVER    {0x00, 0x4B, 0x12, 0x3B, 0xB3, 0x61}  // SoftAP MAC
#define MAC_TRANSMITTER {0x88, 0x57, 0x21, 0xBC, 0x47, 0x7C}  // STA MAC
#define MAC_BROADCAST   {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

// --- Типы пакетов ---
typedef enum : uint8_t {
    PACKET_BATTERY_REQUEST  = 0x01,  // receiver  → transmitter: запрос батареи
    PACKET_BATTERY_RESPONSE = 0x02,  // transmitter → receiver:  данные батареи
    PACKET_RELAY_DATA       = 0x03,  // receiver  → device3:     ретрансляция
} PacketType;

// --- Пакет запроса батареи (receiver → transmitter) ---
// Размер: 4 байта
typedef struct __attribute__((packed)) {
    PacketType type;        // PACKET_BATTERY_REQUEST
    uint8_t    requestId;   // ID запроса (echo в ответе для матчинга)
    uint16_t   reserved;    // зарезервировано
} BatteryRequestPacket;

// --- Пакет ответа с данными батареи (transmitter → receiver) ---
// Размер: 8 байт
typedef struct __attribute__((packed)) {
    PacketType type;          // PACKET_BATTERY_RESPONSE
    uint8_t    requestId;     // echo requestId из запроса
    uint8_t    batteryLevel;  // % заряда (0–100)
    uint8_t    reserved;      // выравнивание
    float      batteryVoltage;// напряжение в вольтах
} BatteryResponsePacket;

// --- Пакет ретрансляции (receiver → device3) ---
// Размер: 8 байт — те же данные батареи transmitter'а
typedef struct __attribute__((packed)) {
    PacketType type;           // PACKET_RELAY_DATA
    uint8_t    batteryLevel;   // % заряда transmitter
    uint16_t   reserved;
    float      batteryVoltage; // напряжение transmitter
} RelayDataPacket;