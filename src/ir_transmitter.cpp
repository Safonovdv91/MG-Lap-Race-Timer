#include "transmitter_config.h"
#include <Arduino.h>

// Инициализация ИК передатчиков
void initIRTransmitters() {
    // Настройка пинов как выходы
    pinMode(IR_TX1_PIN, OUTPUT);

    // Изначально выключаем передатчики
    digitalWrite(IR_TX1_PIN, HIGH);
}
