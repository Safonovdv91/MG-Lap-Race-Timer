#include "../include/ir_transmitter.h"
#include <Arduino.h>

// Инициализация ИК передатчиков
void initIRTransmitters() {
    // Настройка пинов как выходы
    pinMode(IR_TX1_PIN, OUTPUT);
    pinMode(IR_TX2_PIN, OUTPUT);
    
    // Изначально выключаем передатчики
    digitalWrite(IR_TX1_PIN, LOW);
    digitalWrite(IR_TX2_PIN, LOW);
    
    // Настройка ШИМ для генерации 38 кГц сигнала
    ledcSetup(PWM_CHANNEL_1, IR_CARRIER_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_2, IR_CARRIER_FREQ, PWM_RESOLUTION);
    
    // Привязка каналов ШИМ к пинам
    ledcAttachPin(IR_TX1_PIN, PWM_CHANNEL_1);
    ledcAttachPin(IR_TX2_PIN, PWM_CHANNEL_2);
    
    // Включаем передатчики по умолчанию
    enableIRTransmitter1(true);
    enableIRTransmitter2(true);
}

// Управление первым ИК передатчиком
void enableIRTransmitter1(bool enable) {
    if (enable) {
        ledcWrite(PWM_CHANNEL_1, 512); // 50% скважность (512 из 1023)
    } else {
        ledcWrite(PWM_CHANNEL_1, 0); // Выключить ШИМ
    }
}

// Управление вторым ИК передатчиком
void enableIRTransmitter2(bool enable) {
    if (enable) {
        ledcWrite(PWM_CHANNEL_2, 512); // 50% скважность (512 из 1023)
    } else {
        ledcWrite(PWM_CHANNEL_2, 0); // Выключить ШИМ
    }
}

// Установка мощности излучения (в процентах)
void setIRPower(uint8_t powerPercent) {
    uint32_t dutyCycle = map(powerPercent, 0, 100, 0, 1023);
    
    // Применяем одинаковую мощность к обоим передатчикам
    ledcWrite(PWM_CHANNEL_1, dutyCycle);
    ledcWrite(PWM_CHANNEL_2, dutyCycle);
}