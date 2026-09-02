// transmitter_config.h
#ifndef TRANSMITTER_CONFIG_H
#define TRANSMITTER_CONFIG_H

#include <Arduino.h>

// Пины для ИК передатчиков
#define IR_TX1_PIN 13  //D13 (GPIO13) Пин для управления первым ИК передатчиком
// LED Pin
#define RED_LED_PIN 32
#define GREEN_LED_PIN 33    
#define BLUE_LED_PIN 25 
const unsigned long BLINK_ON_TIME  = 200;   // мс, горит
const unsigned long BLINK_OFF_TIME = 3000;  // мс, пауза

const unsigned long HEARTBEAT_ON_TIME  = 100;   // мс, горит
const unsigned long HEARTBEAT_OFF_TIME = 3000;  // мс, пауза

// Частота ШИМ для модуляции ИК сигнала (38 кГц)
#define PWM_CHANNEL_1 0  // Канал ШИМ для первого передатчика
#define IR_FREQ 38000 // Частота генерации ИК сигнала


void initIRTransmitters();
void initLEDTransmitters();
void updateBatteryLed();

#endif