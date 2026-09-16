#include "utils/transmitter_config.h"
#include <Arduino.h>

// Инициализация ИК передатчиков
void initIRTransmitters() {
    // Настройка пинов как выходы
    Serial.print("Инициализация IR");
    // ledcSetup(0,IR_FREQ,8);
    // ledcAttachPin(IR_TX1_PIN,0);
    // ledcWrite(0,127); //duty 50% 255 - 100%
    pinMode(IR_TX1_PIN,OUTPUT);
    digitalWrite(IR_TX1_PIN, HIGH);
}

void initLEDTransmitters(){
    Serial.print("Проверка LED:");

    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN,OUTPUT);

    digitalWrite(RED_LED_PIN, HIGH);
    delay(300);

    digitalWrite(RED_LED_PIN,LOW);
    digitalWrite(BLUE_LED_PIN, HIGH);
    delay(300);

    digitalWrite(BLUE_LED_PIN,LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(300);

    digitalWrite(GREEN_LED_PIN,LOW);
    Serial.println("Ok ");
}