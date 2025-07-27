#include "settings_storage.h"
#include "temperature.h"
#include "damper_control.h"
#include "lvgl_display.h"
#include "display_manager.h"
#include "settings_screen.h"
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
const char* KEY_READ_INTERVAL = "readIntrvl";

// Display settings
const char* KEY_BRIGHTNESS = "brightness";
const char* KEY_TIME_UPDATE = "timeUpdate";
const char* KEY_TOUCH_UPDATE = "touchUpdate";

// Damper settings
const char* KEY_MIN_DAMPER = "minDamper";
const char* KEY_MAX_DAMPER = "maxDamper";
const char* KEY_ZERO_DAMPER = "zeroDamper";
const char* KEY_SERVO_ANGLE = "servoAngle";
const char* KEY_SERVO_OFFSET = "servoOffs";
const char* KEY_SERVO_STEP = "servoStep";

// Control parameters
const char* KEY_KP = "kP";
const char* KEY_TAU_I = "tauI";
const char* KEY_TAU_D = "tauD";
const char* KEY_END_TRIGGER = "endTrigger";
const char* KEY_REFILL_TRIGGER = "refillTrig";
const char* KEY_LOW_TEMP_TIMEOUT = "lowTmpTout";

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
    success &= saveDisplaySettings();
    
    // Force preferences write to flash
    preferences.end();
    preferences.begin(PREFERENCES_NAMESPACE, false);
    
    // SVARĪGI: Atjauninām galveno ekrānu pēc iestatījumu saglabāšanas
    // Šis nodrošinās, ka visas vērtības ir sinhronizētas pēc saglabāšanas
    lvgl_display_update_target_temp();
    
    return success;
}

bool saveTemperatureSettings() {
    // Save temperature related settings
    preferences.putInt(KEY_TARGET_TEMP, targetTempC);
    preferences.putInt(KEY_MIN_TEMP, temperatureMin);
    preferences.putInt(KEY_MAX_TEMP, maxTemp);
    preferences.putInt(KEY_WARNING_TEMP, warningTemperature);
    preferences.putULong(KEY_READ_INTERVAL, tempReadIntervalMs);
    
    Serial.println("Temperature settings saved");
    return true;
}

bool saveDamperSettings() {
    // Save damper related settings
    preferences.putInt(KEY_MIN_DAMPER, minDamper);
    preferences.putInt(KEY_MAX_DAMPER, maxDamper);
    preferences.putInt(KEY_ZERO_DAMPER, zeroDamper);
    preferences.putInt(KEY_SERVO_ANGLE, servoAngle);
    preferences.putInt(KEY_SERVO_OFFSET, servoOffset);
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
    preferences.putULong(KEY_LOW_TEMP_TIMEOUT, LOW_TEMP_TIMEOUT);
    
    Serial.println("Control settings saved");
    return true;
}

// Saglabā ekrāna iestatījumus
bool saveDisplaySettings() {
    // Mainīgie jau ir pieejami caur settings_screen.h
    
    // Saglabājam ekrāna iestatījumus
    preferences.putUChar(KEY_BRIGHTNESS, screenBrightness);
    preferences.putULong(KEY_TIME_UPDATE, timeUpdateIntervalMs);
    preferences.putULong(KEY_TOUCH_UPDATE, touchUpdateIntervalMs);
    
    Serial.println("Display settings saved");
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
    tempReadIntervalMs = preferences.getULong(KEY_READ_INTERVAL, tempReadIntervalMs);
    
    // Load damper settings
    minDamper = preferences.getInt(KEY_MIN_DAMPER, minDamper);
    maxDamper = preferences.getInt(KEY_MAX_DAMPER, maxDamper);
    zeroDamper = preferences.getInt(KEY_ZERO_DAMPER, zeroDamper);
    servoAngle = preferences.getInt(KEY_SERVO_ANGLE, servoAngle);
    servoOffset = preferences.getInt(KEY_SERVO_OFFSET, servoOffset);
    servoStepInterval = preferences.getInt(KEY_SERVO_STEP, servoStepInterval);
    
    // Load PID and control settings
    kP = preferences.getInt(KEY_KP, kP);
    tauI = preferences.getFloat(KEY_TAU_I, tauI);
    tauD = preferences.getFloat(KEY_TAU_D, tauD);
    endTrigger = preferences.getFloat(KEY_END_TRIGGER, endTrigger);
    refillTrigger = preferences.getFloat(KEY_REFILL_TRIGGER, refillTrigger);
    LOW_TEMP_TIMEOUT = preferences.getULong(KEY_LOW_TEMP_TIMEOUT, LOW_TEMP_TIMEOUT);
    
    // Update dependent values
    kI = kP / tauI;
    kD = kP * tauD;
    
    // Mainīgie jau ir pieejami caur settings_screen.h
    
    // Load display settings
    screenBrightness = preferences.getUChar(KEY_BRIGHTNESS, 255);
    timeUpdateIntervalMs = preferences.getULong(KEY_TIME_UPDATE, TIME_UPDATE_INTERVAL_MS);
    touchUpdateIntervalMs = preferences.getULong(KEY_TOUCH_UPDATE, TOUCH_UPDATE_INTERVAL_MS);
    
    // Apply the loaded display manager settings
    display_manager_set_update_intervals(
        DEFAULT_TEMP_UPDATE_INTERVAL,
        DEFAULT_DAMPER_UPDATE_INTERVAL,
        timeUpdateIntervalMs,
        touchUpdateIntervalMs
    );
    
    Serial.println("Settings loaded from storage");
    printCurrentSettings();
    return true;
}

void printCurrentSettings() {
    // Mainīgie jau ir pieejami caur settings_screen.h
    
    Serial.println("======= Current Settings =======");
    Serial.print("Target Temp: "); Serial.println(targetTempC);
    Serial.print("Min Temp: "); Serial.println(temperatureMin);
    Serial.print("Max Temp: "); Serial.println(maxTemp);
    Serial.print("Warning Temp: "); Serial.println(warningTemperature);
    Serial.print("Read Interval (ms): "); Serial.println(tempReadIntervalMs);
    Serial.println("--- PID & Damper Settings ---");
    Serial.print("kP: "); Serial.println(kP);
    Serial.print("tauI: "); Serial.println(tauI);
    Serial.print("tauD: "); Serial.println(tauD);
    Serial.print("End Trigger: "); Serial.println(endTrigger);
    Serial.print("Refill Trigger: "); Serial.println(refillTrigger);
    Serial.print("Low Temp Timeout (ms): "); Serial.println(LOW_TEMP_TIMEOUT);
    Serial.println("--- Servo Settings ---");
    Serial.print("Servo Angle: "); Serial.println(servoAngle);
    Serial.print("Servo Offset: "); Serial.println(servoOffset);
    Serial.print("Servo Step Interval: "); Serial.println(servoStepInterval);
    Serial.println("--- Display Settings ---");
    Serial.print("Screen Brightness: "); Serial.println(screenBrightness);
    Serial.print("Time Update Interval (ms): "); Serial.println(timeUpdateIntervalMs);
    Serial.print("Touch Update Interval (ms): "); Serial.println(touchUpdateIntervalMs);
    Serial.println("==============================");
}
