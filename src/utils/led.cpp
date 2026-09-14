#include "utils/led_config.h"
#include <Arduino.h>
#include <drivers/battery/battery.h>
#include <core/measurement_core.h>

// ================= Координация доступа к общим LED =================
struct LedRequest {
    bool active;    // подсистема сейчас претендует на этот пин
    uint8_t value;  // 0-255 (0 = выкл, 255 = полностью вкл)
};

LedRequest batteryRedReq   = {false, 0};
LedRequest batteryGreenReq = {false, 0};
LedRequest batteryBlueReq  = {false, 0};

LedRequest operationRedReq = {false, 0};

LedRequest irBeamLostBlueReq = {false, 0}; // высший приоритет на BLUE
LedRequest irNormalBlueReq   = {false, 0}; // низший приоритет на BLUE

// ===== Обновление индикации батареи + heartbeat (вызывать каждый loop()) =====
enum BlinkColor { NONE, RED, YELLOW, VIOLET, ORANGE };

unsigned long lastBlinkChange = 0;
bool ledIsOn = false;

unsigned long lastHeartbeatChange = 0;
bool heartbeatIsOn = false;


void initLED(){
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

void updateBatteryLed() {
    unsigned long now = millis();
    BlinkColor color;

    if (batteryPercentage < 10)      color = RED;
    else if (batteryPercentage < 20) color = YELLOW;
    else if (batteryPercentage < 30) color = VIOLET;
    else if (batteryPercentage < 50) color = ORANGE;
    else                              color = NONE;

    batteryRedReq   = {false, 0};
    batteryGreenReq = {false, 0};
    batteryBlueReq  = {false, 0};

    if (color == NONE) {
        ledIsOn = false;
        lastBlinkChange = now;

        unsigned long hbElapsed = now - lastHeartbeatChange;
        if (heartbeatIsOn) {
            if (hbElapsed >= HEARTBEAT_ON_TIME) {
                heartbeatIsOn = false;
                lastHeartbeatChange = now;
            }
        } else {
            if (hbElapsed >= HEARTBEAT_OFF_TIME) {
                heartbeatIsOn = true;
                lastHeartbeatChange = now;
            }
        }

        batteryBlueReq.active = true; // претендует на BLUE, но уступает "лучу потерян"
        batteryBlueReq.value  = heartbeatIsOn ? 255 : 0;
        return;
    }

    heartbeatIsOn = false;
    lastHeartbeatChange = now;

    unsigned long elapsed = now - lastBlinkChange;
    if (ledIsOn) {
        if (elapsed >= BLINK_ON_TIME) {
            ledIsOn = false;
            lastBlinkChange = now;
        }
    } else {
        if (elapsed >= BLINK_OFF_TIME) {
            ledIsOn = true;
            lastBlinkChange = now;
        }
    }

    batteryRedReq.active = true; // RED всегда за battery, когда цвет != NONE

    if (!ledIsOn) {
        batteryRedReq.value    = 0;
        batteryGreenReq.active = (color == YELLOW || color == ORANGE);
        batteryGreenReq.value  = 0;
        batteryBlueReq.active  = (color == VIOLET);
        batteryBlueReq.value   = 0;
        return;
    }

    switch (color) {
        case RED:
            batteryRedReq.value = 255;
            break;
        case YELLOW:
            batteryRedReq.value    = 255;
            batteryGreenReq.active = true;
            batteryGreenReq.value  = 255;
            break;
        case VIOLET:
            batteryRedReq.value   = 255;
            batteryBlueReq.active = true;
            batteryBlueReq.value  = 255;
            break;
        case ORANGE:
            batteryRedReq.value    = 255;
            batteryGreenReq.active = true;
            batteryGreenReq.value  = 140;
            break;
        default:
            break;
    }
}


void updateOperationLed() {
    static unsigned long lastBlinkTime = 0;
    static bool ledState = false;
    
    static TimerStatus lastStatus = STATUS_READY;

    TimerStatus currentStatus = getTimerStatus();

    if (currentStatus != lastStatus) {
        lastBlinkTime = 0;
        ledState = false;
        lastStatus = currentStatus;
    }

    unsigned long currentTime = millis();

    switch (currentStatus) {
        case STATUS_READY:
            if (ledState && (currentTime - lastBlinkTime >= LED_BLINK_DURATION)) {
                ledState = false;
                lastBlinkTime = currentTime;
            } else if (!ledState && (currentTime - lastBlinkTime >= (unsigned long)LED_BLINK_INTERVAL - LED_BLINK_DURATION)) {
                ledState = true;
                lastBlinkTime = currentTime;
            }
            break;

        case STATUS_RUNNING:
            if (getCurrentRaceTimeSafe() < MIN_LAP_TIME) {
                if (currentTime - lastBlinkTime >= FAST_BLINK_INTERVAL) {
                    ledState = !ledState;
                    lastBlinkTime = currentTime;
                }
            } else {
                ledState = true;
            }
            break;

        case STATUS_DISPLAY:
            if (currentTime - getDisplayStartTimeSafe() < (MIN_LAP_TIME / 1000)) {
                if (currentTime - lastBlinkTime >= FAST_BLINK_INTERVAL) {
                    ledState = !ledState;
                    lastBlinkTime = currentTime;
                }
            } else {
                ledState = true;
            }
            break;
    }

    operationRedReq.active = true;
    operationRedReq.value  = ledState ? 255 : 0;
}

void handleStatusLED() {
    static unsigned long lastBlinkTime = 0;
    static bool blinkState = false;

    if (isBeamLost()) {
        unsigned long nowMs = millis();
        if (nowMs - lastBlinkTime >= 500) {
            blinkState = !blinkState;
            lastBlinkTime = nowMs;
        }
        irBeamLostBlueReq.active = true;
        irBeamLostBlueReq.value  = blinkState ? 255 : 0;
        irNormalBlueReq.active   = false;
        return;
    }

    irBeamLostBlueReq.active = false;

    int sensorState = digitalRead(SENSOR_PIN);
    irNormalBlueReq.active = true;
    irNormalBlueReq.value  = (sensorState == LOW) ? 255 : 0;
}

void applyLedOutputs() {
    // RED: battery > operation
    if (batteryRedReq.active) {
        analogWrite(RED_LED_PIN, batteryRedReq.value);
    } else if (operationRedReq.active) {
        analogWrite(RED_LED_PIN, operationRedReq.value);
    } else {
        analogWrite(RED_LED_PIN, 0);
    }

    // GREEN: только battery
    analogWrite(GREEN_LED_PIN, batteryGreenReq.active ? batteryGreenReq.value : 0);

    // BLUE: "луч потерян" > battery (heartbeat/VIOLET) > обычный sensorState
    if (irBeamLostBlueReq.active) {
        digitalWrite(BLUE_LED_PIN, irBeamLostBlueReq.value ? HIGH : LOW);
    } else if (batteryBlueReq.active) {
        digitalWrite(BLUE_LED_PIN, batteryBlueReq.value ? HIGH : LOW);
    } else if (irNormalBlueReq.active) {
        digitalWrite(BLUE_LED_PIN, irNormalBlueReq.value ? HIGH : LOW);
    } else {
        digitalWrite(BLUE_LED_PIN, LOW);
    }
}