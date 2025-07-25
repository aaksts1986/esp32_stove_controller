#include "settings_storage.h"
#include "temperature.h"
#include "damper_control.h"
#include <Arduino.h>

// Preferences instance
Preferences preferences;
const char* PREFERENCES_NAMESPACE = "settings";

// Keys for different settings
// Temperature settings
const char* KEY_TARGET_TEMP = "targetTemp";
const char* KEY_MIN_TEMP = "minTemp";
const char* KEY_MAX_TEMP = "maxTemp";
const char* KEY_WARNING_TEMP = "warnTemp";

// Damper settings
const char* KEY_MIN_DAMPER = "minDamper";
const char* KEY_MAX_DAMPER = "maxDamper";
const char* KEY_ZERO_DAMPER = "zeroDamper";
const char* KEY_SERVO_ANGLE = "servoAngle";
const char* KEY_SERVO_STEP = "servoStep";

// Control parameters
const char* KEY_KP = "kP";
const char* KEY_TAU_I = "tauI";
const char* KEY_TAU_D = "tauD";
const char* KEY_END_TRIGGER = "endTrigger";
const char* KEY_REFILL_TRIGGER = "refillTrig";

void initSettingsStorage() {
    // Initialize preferences
    preferences.begin(PREFERENCES_NAMESPACE, false);
    
    // Load settings on startup
    loadAllSettings();
}

bool saveAllSettings() {
    bool success = true;
    
    success &= saveTemperatureSettings();
    success &= saveDamperSettings();
    success &= saveControlSettings();
    
    // Force preferences write to flash
    preferences.end();
    preferences.begin(PREFERENCES_NAMESPACE, false);
    
    return success;
}

bool saveTemperatureSettings() {
    // Save temperature related settings
    preferences.putInt(KEY_TARGET_TEMP, targetTempC);
    preferences.putInt(KEY_MIN_TEMP, temperatureMin);
    preferences.putInt(KEY_MAX_TEMP, maxTemp);
    preferences.putInt(KEY_WARNING_TEMP, warningTemperature);
    
    Serial.println("Temperature settings saved");
    return true;
}

bool saveDamperSettings() {
    // Save damper related settings
    preferences.putInt(KEY_MIN_DAMPER, minDamper);
    preferences.putInt(KEY_MAX_DAMPER, maxDamper);
    preferences.putInt(KEY_ZERO_DAMPER, zeroDamper);
    preferences.putInt(KEY_SERVO_ANGLE, servoAngle);
    preferences.putInt(KEY_SERVO_STEP, servoStepInterval);
    
    Serial.println("Damper settings saved");
    return true;
}

bool saveControlSettings() {
    // Save PID and control settings
    preferences.putInt(KEY_KP, kP);
    preferences.putFloat(KEY_TAU_I, tauI);
    preferences.putFloat(KEY_TAU_D, tauD);
    preferences.putFloat(KEY_END_TRIGGER, endTrigger);
    preferences.putFloat(KEY_REFILL_TRIGGER, refillTrigger);
    
    Serial.println("Control settings saved");
    return true;
}

bool loadAllSettings() {
    // Check if we have saved settings
    if (!preferences.isKey(KEY_TARGET_TEMP)) {
        Serial.println("No saved settings found, using defaults");
        return false;
    }
    
    // Load temperature settings
    targetTempC = preferences.getInt(KEY_TARGET_TEMP, targetTempC);
    temperatureMin = preferences.getInt(KEY_MIN_TEMP, temperatureMin);
    maxTemp = preferences.getInt(KEY_MAX_TEMP, maxTemp);
    warningTemperature = preferences.getInt(KEY_WARNING_TEMP, warningTemperature);
    
    // Load damper settings
    minDamper = preferences.getInt(KEY_MIN_DAMPER, minDamper);
    maxDamper = preferences.getInt(KEY_MAX_DAMPER, maxDamper);
    zeroDamper = preferences.getInt(KEY_ZERO_DAMPER, zeroDamper);
    servoAngle = preferences.getInt(KEY_SERVO_ANGLE, servoAngle);
    servoStepInterval = preferences.getInt(KEY_SERVO_STEP, servoStepInterval);
    
    // Load PID and control settings
    kP = preferences.getInt(KEY_KP, kP);
    tauI = preferences.getFloat(KEY_TAU_I, tauI);
    tauD = preferences.getFloat(KEY_TAU_D, tauD);
    endTrigger = preferences.getFloat(KEY_END_TRIGGER, endTrigger);
    refillTrigger = preferences.getFloat(KEY_REFILL_TRIGGER, refillTrigger);
    
    // Update dependent values
    kI = kP / tauI;
    kD = kP * tauD;
    
    Serial.println("Settings loaded from storage");
    printCurrentSettings();
    return true;
}

void printCurrentSettings() {
    Serial.println("======= Current Settings =======");
    Serial.print("Target Temp: "); Serial.println(targetTempC);
    Serial.print("Min Temp: "); Serial.println(temperatureMin);
    Serial.print("Max Temp: "); Serial.println(maxTemp);
    Serial.print("Warning Temp: "); Serial.println(warningTemperature);
    Serial.print("kP: "); Serial.println(kP);
    Serial.print("Servo Angle: "); Serial.println(servoAngle);
    Serial.print("Servo Step Interval: "); Serial.println(servoStepInterval);
    Serial.println("==============================");
}
