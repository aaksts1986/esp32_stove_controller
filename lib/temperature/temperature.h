#pragma once
#include <Arduino.h>

void initTemperatureSensor();
void updateTemperature();

// Change detection functions
bool hasTemperatureChanged();
unsigned long getTimeSinceLastTempChange();

extern int temperature;
extern int targetTempC;
extern int temperatureMin;
extern int temperaturemini2; // Maximum temperature for the sensor
extern int maxTemp; // Maximum temperature for the target
extern int warningTemperature; // Brīdinājuma temperatūras slieksnis
extern unsigned long tempReadIntervalMs; // Read interval in milliseconds