#include <Arduino.h>

// LED Pin
#define RED_LED_PIN 32
#define GREEN_LED_PIN 33    
#define BLUE_LED_PIN 25 

const unsigned long BLINK_ON_TIME  = 200;   // мс, горит
const unsigned long BLINK_OFF_TIME = 3000;  // мс, пауза

const unsigned long HEARTBEAT_ON_TIME  = 100;   // мс, горит
const unsigned long HEARTBEAT_OFF_TIME = 3000;  // мс, пауза


void initLED();
void updateBatteryLed();
void ledOutBattery(int batteryPercentage);
void updateBatteryLed();
void updateOperationLed();
void handleStatusLED();
void applyLedOutputs();