#include <Arduino.h>
#include "lvgl_display.h"
#include <TFT_eSPI.h>
#include <ESP32Servo.h>
#include "damper_control.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "temperature.h" 

int servoPort = 5;
int buttonPort1 = 4;
int buttonPort2 = 1;
int buttonPort3 = 2;
int relay = 17;
int buzzer = 14;

TFT_eSPI tft = TFT_eSPI();

int damper = 0;

Servo mansServo;

#define TOUCH_THRESHOLD 30000
#define TOUCH_DEBOUNCE_MS 75

void setup() {
    Serial.begin(115200);
    Serial.println("Sistēmas inicializācija...");
    
    lvgl_display_init();
    lvgl_display_update_target_temp();

    damperControlInit();
    startDamperControlTask();

    initTemperatureSensor();

    Serial.println("Sistēma gatava lietošanai!");
}

void loop() {
    lv_timer_handler();

    static uint32_t last_update = 0;
    if (millis() - last_update >= 100) {
        lvgl_display_touch_update();
        last_update = millis();
    }

    updateTemperature(); // <-- pievieno šo


    lvgl_display_update_bars(temperature, targetTempC);
    lvgl_display_update_damper(damper);

    static unsigned long lastTouchTime = 0;
    static bool lastTouch = false;

    int touchValue = touchRead(buttonPort1);

    if (touchValue > TOUCH_THRESHOLD && !lastTouch) {
        unsigned long now = millis();
        if (now - lastTouchTime > TOUCH_DEBOUNCE_MS) {
            targetTempC += 1.0;
            if (targetTempC > 90) targetTempC = 40;
            lvgl_display_update_target_temp();
            lastTouchTime = now;
        }
        lastTouch = true;
    }
    if (touchValue < TOUCH_THRESHOLD) {
        lastTouch = false;
    }

    lv_tick_inc(5);
    delay(5);
}