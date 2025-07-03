#include "temperature.h"
#include <OneWire.h>
#include <DallasTemperature.h>

int temperature = 0;
int targetTempC = 40;
int maxTemp = 80; // Maximum temperature for the target
int temperatureMin = 30; // Minimum temperature to avoid negative values
static OneWire oneWire(6);
static DallasTemperature ds(&oneWire);
static DeviceAddress sensor1 = {0x28, 0xFC, 0x70, 0x96, 0xF0, 0x01, 0x3C, 0xC0};

static unsigned long lastTempRead = 0;
static const unsigned long tempReadInterval = 3000;
static unsigned long tempRequestTime = 0;
static bool tempRequested = false;

void initTemperatureSensor() {
    ds.begin();
    ds.setResolution(sensor1, 9);
}

void updateTemperature() {
    if (!tempRequested && millis() - lastTempRead >= tempReadInterval) {
        ds.requestTemperaturesByAddress(sensor1);
        tempRequestTime = millis();
        tempRequested = true;
    }

    if (tempRequested && millis() - tempRequestTime >= 100) {
        temperature = ds.getTempC(sensor1);
        if (temperature< 0) {
            temperature = 10; // Avoid negative temperatures
        }
        lastTempRead = millis();
        tempRequested = false;
    }
}