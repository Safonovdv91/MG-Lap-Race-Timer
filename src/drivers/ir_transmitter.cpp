#include "utils/transmitter_config.h"
#include <Arduino.h>

// Инициализация ИК передатчиков
void initIRTransmitters() {
    // Настройка пинов как выходы
  ledcSetup(0,IR_FREQ,8);
  ledcAttachPin(IR_TX1_PIN,0);
  ledcWrite(0,127); //duty 50% 255 - 100%
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

void ledOutBattery(int batteryPercentage){
    // функция отображения заряда батареи на LED индикаторах
        if (batteryPercentage < 10) {
            digitalWrite(RED_LED_PIN, HIGH);
            Serial.println("Battery: < 10%");
        }
        else if (batteryPercentage >= 10 and batteryPercentage <= 20)
        {
            digitalWrite(RED_LED_PIN, HIGH);
            digitalWrite(GREEN_LED_PIN, HIGH);
            Serial.println("Battery: 10-20%");
        }
        else {
            digitalWrite(RED_LED_PIN, LOW);
            digitalWrite(GREEN_LED_PIN, LOW);
        }
}