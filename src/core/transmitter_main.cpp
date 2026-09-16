#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "utils/transmitter_config.h"
#include "drivers/battery/battery.h"
#include "drivers/espnow_transmitter.h"
#include "esp_sleep.h"

// ============================================================
// Power control
// ============================================================
#define BUTTON_PIN GPIO_NUM_0     // RTC GPIO с поддержкой INPUT_PULLUP
#define HOLD_TIME_MS 2000           // время удержания для ухода в сон
#define DEBOUNCE_MS 50   


RTC_DATA_ATTR int bootCount = 0;    // счётчик пробуждений, сохраняется в RTC-памяти

unsigned long pressStartTime = 0;
bool buttonWasPressed = false;
bool longPressHandled = false;


void goToDeepSleep() {
  Serial.println("Ухожу в глубокий сон...");
  Serial.flush();

  // Гасим периферию/радиомодуль здесь, если нужно (LoRa/NRF24/RF и т.д.)
  digitalWrite(IR_TX1_PIN,LOW);
  // например: radio.sleep();  digitalWrite(RADIO_EN, LOW);

  // Настраиваем пробуждение по этой же кнопке.
  // Кнопка при нажатии замыкает пин на GND -> уровень LOW.
  // 0 = wake on LOW level.
  esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 0);

  // Ждём, пока пользователь отпустит кнопку, чтобы не проснуться сразу же
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(100); // небольшая пауза после отпускания

  esp_deep_sleep_start();
}

void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Пробуждение по кнопке (EXT0)");
      break;
    default:
      Serial.println("Обычный запуск / сброс питания");
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Инициализация ИК передатчиков(Включен постоянно)
  initIRTransmitters();

  // Инициализация LED 
  initLEDTransmitters();
 
  
  bootCount++;
  Serial.print("Запуск #");
  Serial.println(bootCount);
  printWakeupReason();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
 while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  // Подключение к Wi-Fi сети
  espnow_init();   // WiFi.mode(WIFI_STA) вызывается внутри

  Serial.print("Transmitter MAC: ");
  Serial.println(WiFi.macAddress());
  
  // Инициализация определения заряда батареи
  initReadBattery();

  Serial.println("Плата включена и готова к работе.");
}

unsigned long lastBeaconTime = 0;
bool isIRPulseOn = true;


void loop() {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  if (pressed && !buttonWasPressed) {
    // Кнопку только что нажали
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
      Serial.print("Короткое нажатие, держали ");
      Serial.print(heldFor);
      Serial.println(" мс");
    }
    buttonWasPressed = false;
  }

  unsigned long currentTime = millis();

  // --- Battery Reading ---
  readBattery();
  updateBatteryLed();
  
  // --- ESP-NOW ---
  espnow_loop();  
  }
