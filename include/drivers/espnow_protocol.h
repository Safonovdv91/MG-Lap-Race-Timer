#pragma once
#include <stdint.h>

// ============================================================
//  ESP-NOW Protocol — общий заголовок для всех устройств
// ============================================================

// --- MAC адреса устройств ---
#define MAC_RECEIVER {0x88, 0x57, 0x21, 0xBC, 0x47, 0x7D}
#define MAC_TRANSMITTER {0x00, 0x4B, 0x12, 0x3B, 0xB3, 0x60}

// #define MAC Lilygo      {0x96, 0xA9, 0x90, 0x2B, 0x16, 0x24}  // Lilygo точечно
#define MAC Lilygo      {0x30, 0xED, 0xA0, 0xBF, 0x82, 0x90}  // Lilygo точечно
#define MAC_BROADCAST   {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

// --- Типы пакетов ---
typedef enum : uint8_t {
    PACKET_BATTERY_REQUEST  = 0x01,  // receiver  → transmitter
    PACKET_BATTERY_RESPONSE = 0x02,  // transmitter → receiver
    PACKET_RELAY_DATA       = 0x03,  // receiver  → device3 (батарея)
    PACKET_RACE_STATE       = 0x04,  // receiver  → broadcast (стриминг гонки)
} PacketType;

// --- Пакет запроса батареи ---
typedef struct __attribute__((packed)) {
    PacketType type;        // PACKET_BATTERY_REQUEST
    uint8_t    requestId;
    uint16_t   reserved;
} BatteryRequestPacket;

// --- Пакет ответа с данными батареи ---
typedef struct __attribute__((packed)) {
    PacketType type;          // PACKET_BATTERY_RESPONSE
    uint8_t    requestId;
    uint8_t    batteryLevel;
    uint8_t    reserved;
    float      batteryVoltage;
} BatteryResponsePacket;

// --- Пакет ретрансляции батареи на device3 ---
typedef struct __attribute__((packed)) {
    PacketType type;           // PACKET_RELAY_DATA
    uint8_t    batteryLevel;
    uint16_t   reserved;
    float      batteryVoltage;
} RelayDataPacket;

// --- Пакет стриминга состояния гонки (broadcast, 2 раза/сек) ---
// timer_status кодируется одним байтом:
//   0x00 = ready, 0x01 = running, 0x02 = display
// mode — произвольное uint8_t значение (приводи свой enum к uint8_t)
// race_time — секунды с дробной частью (microseconds / 1 000 000.0)
typedef struct __attribute__((packed)) {
    PacketType type;          // PACKET_RACE_STATE
    uint8_t    timerStatus;   // 0=ready, 1=running, 2=display
    uint8_t    mode;          // значение currentMode
    uint8_t    reserved;
    float      raceTime;      // секунды, 0.0 если не running/display
} RaceStatePacket;

// Хелперы для timerStatus
#define RACE_STATUS_READY   0x00
#define RACE_STATUS_RUNNING 0x01
#define RACE_STATUS_DISPLAY 0x02