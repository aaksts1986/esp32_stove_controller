#include <Arduino.h>
#include "lvgl_display.h"
#include <TFT_eSPI.h>
#include <ESP32Servo.h>
#include "damper_control.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "temperature.h" 

int servoPort = 5;

int relay = 17;
int buzzer = 14;

TFT_eSPI tft = TFT_eSPI();

int damper = 0;

Servo mansServo;



void setup() {
    Serial.begin(115200);
    Serial.println("Sistēmas inicializācija...");
    
    lvgl_display_init();
    lvgl_display_update_target_temp();

    damperControlInit();
    startDamperControlTask();

    initTemperatureSensor();

    touchButtonInit(buttonPort1, TOUCH_THRESHOLD, TOUCH_DEBOUNCE_MS);

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

    touchButtonHandle(targetTempC, 40, 90, 1.0);

    lv_tick_inc(5);
    delay(5);
}