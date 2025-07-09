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
extern int maxTemp; // Maximum temperature for the target