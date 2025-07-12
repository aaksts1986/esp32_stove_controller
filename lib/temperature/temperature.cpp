#include "temperature.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "damper_control.h"
#include "display_manager.h"  // Add display manager


int temperature = 0;
int targetTempC = 62;
int maxTemp = 80; // Maximum temperature for the target
int temperatureMin = 45; // Minimum temperature to avoid negative values
static OneWire oneWire(6);
static DallasTemperature ds(&oneWire);
static DeviceAddress sensor1 = {0x28, 0xFC, 0x70, 0x96, 0xF0, 0x01, 0x3C, 0xC0};

static unsigned long lastTempRead = 0;
static const unsigned long tempReadInterval = 3000;
static unsigned long tempRequestTime = 0;
static bool tempRequested = false;

// Change detection variables
static int lastDisplayedTemperature = -999;  // Force first update
static unsigned long lastChangeTime = 0;
static bool temperatureChanged = false;

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
        int newTemperature = ds.getTempC(sensor1);
        if (newTemperature < 0) {
            newTemperature = 5;
        }
        
        // Check if temperature actually changed
        if (newTemperature != lastDisplayedTemperature) {
            temperature = newTemperature;
            lastDisplayedTemperature = temperature;
            lastChangeTime = millis();
            temperatureChanged = true;
            
            // Notify display manager that temperature changed
            display_manager_notify_temperature_changed();
            
            Serial.printf("Temperature changed: %d°C (was %d°C)\n", 
                         temperature, lastDisplayedTemperature);
        }
        
        damperControlLoop();
        lastTempRead = millis();
        tempRequested = false;
    }
}

// New function to check if temperature has changed
bool hasTemperatureChanged() {
    if (temperatureChanged) {
        temperatureChanged = false;  // Reset flag
        return true;
    }
    return false;
}

// Get time since last temperature change
unsigned long getTimeSinceLastTempChange() {
    return millis() - lastChangeTime;
}