#pragma once
#include <Arduino.h>

void initTemperatureSensor();
void updateTemperature();
extern int temperature;
extern int targetTempC;
extern int temperatureMin;
extern int maxTemp; // Maximum temperature for the target