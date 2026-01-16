#ifndef IR_TRANSMITTER_H
#define IR_TRANSMITTER_H

#include <Arduino.h>

#ifdef TRANSMITTER_MODE
#include "transmitter_config.h"
#else
// Определения пинов для ИК передатчиков
#define IR_TX1_PIN 13  // Пин для управления первым ИК передатчиком
#define IR_TX2_PIN 12  // Пин для управления вторым ИК передатчиком

// Частота ШИМ для модуляции ИК сигнала (38 кГц)
#define IR_CARRIER_FREQ 38000
#define PWM_CHANNEL_1 0  // Канал ШИМ для первого передатчика
#define PWM_CHANNEL_2 1  // Канал ШИМ для второго передатчика
#define PWM_RESOLUTION 10 // Разрешение ШИМ (10 бит = 0-1023)
#endif

void initIRTransmitters();
void enableIRTransmitter1(bool enable);
void enableIRTransmitter2(bool enable);
void setIRPower(uint8_t powerPercent);

#endif