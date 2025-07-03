#pragma once

#define TOUCH_THRESHOLD 30000
#define TOUCH_DEBOUNCE_MS 75

extern int buttonPort1;
extern int buttonPort2;
extern int buttonPort3;

void touchButtonInit(int pin, int threshold, int debounceMs);
bool touchButtonPressed();
void touchButtonHandle();