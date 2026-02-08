#include <Arduino.h>

#define IR_PIN 13

// Несущая
const uint32_t CARRIER_FREQ = 38000;
const uint8_t DUTY = 50; // ~33%

// Тайминги пачек (будут задаваться из Serial)
volatile uint32_t burst_us = 0;
volatile uint32_t pause_us = 0;

hw_timer_t *timer = NULL;
volatile bool carrierOn = false;
volatile bool running = false;

// Переключение burst/pause аппаратным таймером
void IRAM_ATTR onTimer() {
    if (!running) return;

    if (carrierOn) {
        // Выключаем несущую -> пауза
        ledcWrite(0, 0);
        timerAlarmWrite(timer, pause_us, true);
        carrierOn = false;
    } else {
        // Включаем несущую -> burst
        ledcWrite(0, DUTY);
        timerAlarmWrite(timer, burst_us, true);
        carrierOn = true;
    }
}

void setupPWM() {
    ledcSetup(0, CARRIER_FREQ, 8);
    ledcAttachPin(IR_PIN, 0);
    ledcWrite(0, 0);
}

void startIR() {
    carrierOn = false;
    running = true;
    timerAlarmWrite(timer, 100, true); // старт через 100 мкс
    timerAlarmEnable(timer);
}

void stopIR() {
    running = false;
    ledcWrite(0, 0);
    timerAlarmDisable(timer);
}

void setup() {
    Serial.begin(115200);
    setupPWM();

    // Аппаратный таймер 1 мкс тик
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);

    Serial.println("\n=== TSOP IR tuner (HW timer) ===");
    Serial.println("Enter: <burst_us> <pause_us>");
}

void loop() {
    if (Serial.available()) {
        uint32_t b = Serial.parseInt();
        uint32_t p = Serial.parseInt();

        if (b > 0 && p > 0) {
            burst_us = b;
            pause_us = p;

            Serial.println();
            Serial.print("BURST_US: ");
            Serial.println(burst_us);
            Serial.print("PAUSE_US: ");
            Serial.println(pause_us);
            Serial.println("Running 10 seconds...\n");

            startIR();

            uint32_t t0 = millis();
            while (millis() - t0 < 10000) {
                delay(10); // Serial живой
            }

            stopIR();

            Serial.println("Stopped.\nEnter new values:");
        }

        while (Serial.available()) Serial.read();
    }
}
