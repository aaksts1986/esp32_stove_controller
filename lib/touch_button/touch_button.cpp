#include "touch_button.h"
#include <Arduino.h>
#include "lvgl_display.h"
#include "damper_control.h"
#include "temperature.h"

int buttonPort1 = 4;
int buttonPort2 = 1;
int buttonPort3 = 2;

static int buttonPin;
static int touchThreshold;
static int debounceMs;
static unsigned long lastTouchTime = 0;
static bool lastTouch = false;

void touchButtonInit(int pin, int threshold, int debounce) {
    buttonPin = pin;
    touchThreshold = threshold;
    debounceMs = debounce;
}

bool touchButtonPressed() {
    int touchValue = touchRead(buttonPin);
    if (touchValue > touchThreshold && !lastTouch) {
        unsigned long now = millis();
        if (now - lastTouchTime > debounceMs) {
            lastTouchTime = now;
            lastTouch = true;
            return true;
        }
    }
    if (touchRead(buttonPin) < touchThreshold) {
        lastTouch = false;
    }
    return false;
}

void touchButtonHandle() {
    if (touchButtonPressed()) {
        targetTempC += 1;
        if (targetTempC > maxTemp) targetTempC = temperatureMin;
        lvgl_display_update_target_temp();
    }
}