#include "drivers/power_button.h"

// ============================================================
// Настройки
// ============================================================
namespace {
    constexpr gpio_num_t BUTTON_PIN   = GPIO_NUM_0;   // RTC GPIO, есть внутренний pull-up
    constexpr unsigned long HOLD_TIME_MS = 2000;      // удержание для ухода в сон
    constexpr unsigned long DEBOUNCE_MS  = 50;        // антидребезг

    unsigned long pressStartTime  = 0;
    bool buttonWasPressed   = false;
    bool longPressHandled   = false;
}

// Сохраняется в RTC-памяти между циклами deep sleep
RTC_DATA_ATTR static int bootCount = 0;

static void printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[PowerButton] Пробуждение по кнопке (EXT0)");
            break;
        default:
            Serial.println("[PowerButton] Обычный запуск / сброс питания");
            break;
    }
}

static void goToDeepSleep() {
    Serial.println("[PowerButton] Ухожу в глубокий сон...");
    Serial.flush();

    // Здесь при необходимости гасим периферию/радио перед сном
    // например: espnow_deinit(); WiFi.mode(WIFI_OFF);

    // Пробуждение по этой же кнопке: нажатие замыкает пин на GND -> LOW.
    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);

    // Ждём отпускания кнопки, чтобы не проснуться сразу же
    while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
    }
    delay(100);

    esp_deep_sleep_start();
    // дальше код не выполняется
}

void powerButton_init() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    bootCount++;
    Serial.print("[PowerButton] Запуск #");
    Serial.println(bootCount);
    printWakeupReason();

    // Если разбудились по кнопке - ждём, пока её отпустят,
    // чтобы тот же клик не засчитался как новое долгое нажатие
    while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
    }
}

void powerButton_loop() {
    bool pressed = (digitalRead(BUTTON_PIN) == LOW);

    if (pressed && !buttonWasPressed) {
        delay(DEBOUNCE_MS);
        if (digitalRead(BUTTON_PIN) == LOW) {
            buttonWasPressed = true;
            pressStartTime = millis();
            longPressHandled = false;
        }
    }

    if (pressed && buttonWasPressed && !longPressHandled) {
        if (millis() - pressStartTime >= HOLD_TIME_MS) {
            longPressHandled = true;
            goToDeepSleep();
        }
    }

    if (!pressed && buttonWasPressed) {
        unsigned long heldFor = millis() - pressStartTime;
        if (heldFor < HOLD_TIME_MS) {
            Serial.print("[PowerButton] Короткое нажатие, держали ");
            Serial.print(heldFor);
            Serial.println(" мс");
        }
        buttonWasPressed = false;
    }
}