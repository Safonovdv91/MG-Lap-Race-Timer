#ifndef BATTERY_H
#define BATTERY_H

extern float batteryVoltage;
extern int batteryPercentage;
extern unsigned long lastBatteryRead;

void initReadBattery();
void readBattery();
int calculateBatteryPercentage(float voltage);

float getBatteryVoltage();
int getBatteryPercentage();

// измерение напряжения (
// R1 (-) 1 MOm 
// R2 (+) 300kOm 
//          |   Vin        |     ADC_PIN_V 32  | 
//  100%    |    4.1       |        2.767      |
//          |    4.0       |        2.710      |
//          |    3.8       |        2.527      |
//          |    3.6       |        2.439      |
//  0%      |    3.4       |        2.263      |

#define BATTERY_PIN 34

#define BATTERY_MAX_V 4.1  // Максимальное напряжение (100%)
#define BATTERY_MIN_V 3.4  // Минимальное напряжение (0%)

#define VOLTAGE_DIVIDER_MULTIPLIER 1.488
// Характеристики АЦП ESP32
#define ADC_REFERENCE_VOLTAGE 3.3 // Опорное напряжение АЦП ESP32
#define ADC_MAX_READING 4095.0    // Максимальное значение 12-битного АЦП 

#endif
