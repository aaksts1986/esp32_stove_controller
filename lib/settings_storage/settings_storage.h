#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Iestatījumu saglabāšanas/ielādes funkcijas
void initSettingsStorage();
bool saveAllSettings();
bool loadAllSettings();

// Individual setting save/load functions
bool saveTemperatureSettings();
bool saveDamperSettings();
bool saveControlSettings();
bool saveDisplaySettings();

// Debug helper
void printCurrentSettings();
