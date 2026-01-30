#ifndef BATTERY_H
#define BATTERY_H

extern float batteryVoltage;
extern int batteryPercentage;
extern unsigned long lastBatteryRead;

void readBattery();
int calculateBatteryPercentage(float voltage);

float getBatteryVoltage();
int getBatteryPercentage();

#endif
