#include <Arduino.h>
#include "lvgl_display.h"
#include <TFT_eSPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h> // ESP32 atbalsta šo bibliotēku

// Pin definitions
int servoPort = 5; // servo pin
int oneWirePort = 6; // OneWire pin 
int buttonPort1 = 4; // button pin
int buttonPort2 = 1; // button pin
int buttonPort3 = 2; // button pin
int relay = 17; // relay pin
int buzzer = 14; // buzzer pin

// Include the LVGL library
TFT_eSPI tft = TFT_eSPI();


int skaititajs = 75;
int temperature =0;

// OneWire and DallasTemperature setup
OneWire oneWire(oneWirePort); // OneWire bus on pin 6 
DallasTemperature ds(&oneWire);
DeviceAddress sensor1 = {0x28, 0xFC, 0x70, 0x96, 0xF0, 0x01, 0x3C, 0xC0};
byte E;

// Buzzer settings
int buzzerPort = 2;               // Buzzer port id
int buzzerRefillFrequency = 1900; // Buzzer tone frequency for refill alarm
int buzzerRefillRepeat = 1;       // Number of refill alarm tones
int buzzerRefillDelay = 1000;     // Delay between refill alarm tones
int buzzerEndFrequency = 950;     // Buzzer tone frequency for end of fire damper close alarm
int buzzerEndRepeat = 1;          // Number of tones for end of fire damper close alarm
int buzzerEndDelay = 200;        // Delay of tone for end of fire damper close alarm

Servo mansServo; // Servo objekts

void setup() {
    Serial.begin(115200);
    Serial.println("Sistēmas inicializācija...");
    lvgl_display_init();
    E= ds.getDeviceCount();  
    ds.begin();
    mansServo.attach(servoPort); // ESP32: var izmantot jebkuru PWM-capable GPIO
    Serial.println("Sistēma gatava lietošanai!");
}

void loop() {
    lv_timer_handler();

    static uint32_t last_update = 0;
    if (millis() - last_update >= 100) {
        lvgl_display_touch_update();
        last_update = millis();
    }

    lvgl_display_update_bars(skaititajs);
    
    lv_tick_inc(5);
    delay(5);
}