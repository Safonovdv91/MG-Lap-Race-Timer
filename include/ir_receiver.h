#ifndef IR_RECEIVER_H
#define IR_RECEIVER_H

#include <Arduino.h>
#include "config.h"

// Функции для работы с ИК приемниками
void initIRReceivers();
bool isIRBeamBlocked1();
bool isIRBeamBlocked2();

// Функции для диагностики состояния ИК лучей
bool getIRSignalStatus1();
bool getIRSignalStatus2();

#endif