#include "touch_button.h"
#include <Arduino.h>

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

void touchButtonHandle(int &targetTempC, int minTemp, int maxTemp, int step) {
    if (touchButtonPressed()) {
        targetTempC += step;
        if (targetTempC > maxTemp) targetTempC = minTemp;
    }
}