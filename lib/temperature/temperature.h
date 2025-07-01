#pragma once
#include <Arduino.h>

void initTemperatureSensor();
void updateTemperature();
extern int temperature;
extern int targetTempC;