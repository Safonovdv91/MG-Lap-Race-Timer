// transmitter_config.h
#ifndef TRANSMITTER_CONFIG_H
#define TRANSMITTER_CONFIG_H

#include <Arduino.h>

// Пины для ИК передатчиков
#define IR_TX1_PIN 13  //D13 (GPIO13) Пин для управления первым ИК передатчиком

// Частота ШИМ для модуляции ИК сигнала (38 кГц)
#define PWM_CHANNEL_1 0  // Канал ШИМ для первого передатчика


void initIRTransmitters();

#endif