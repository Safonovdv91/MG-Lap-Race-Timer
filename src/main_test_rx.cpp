#include <Arduino.h>

#define TSOP_PIN 12  // GPIO12 для ESP32

volatile uint32_t lastLowTime = 0;
volatile bool beamPresent = true;

hw_timer_t *timer = NULL;

// ISR — вызывается на каждый спад (приход burst)
void IRAM_ATTR tsopISR() {
    lastLowTime = micros();
}

// Таймер проверяет: давно ли не было burst
void IRAM_ATTR checkBeam() {
    uint32_t now = micros();

    // Если больше 3000 мкс не было LOW — луч прерван
    if (now - lastLowTime > 3000) {
        beamPresent = false;
    } else {
        beamPresent = true;
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(TSOP_PIN, INPUT);

    attachInterrupt(digitalPinToInterrupt(TSOP_PIN), tsopISR, FALLING);

    // Аппаратный таймер ESP32 — проверка каждые 1 мс
    timer = timerBegin(0, 80, true);  // 1 мкс тик
    timerAttachInterrupt(timer, &checkBeam, true);
    timerAlarmWrite(timer, 1000, true); // 1 мс
    timerAlarmEnable(timer);

    Serial.println("TSOP receiver started");
}

void loop() {
    static bool prevState = true;

    if (beamPresent != prevState) {
        prevState = beamPresent;

        if (beamPresent) {
            Serial.println("Beam PRESENT");
        } else {
            Serial.println(">>> Beam INTERRUPTED <<<");
        }
    }
}
