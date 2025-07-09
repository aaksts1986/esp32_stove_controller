#include <Arduino.h>
#include "lvgl_display.h"
#include <ESP32Servo.h>
#include "damper_control.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "temperature.h" 
#include "touch_button.h"
#include "wifi1.h" 
#include "display_manager.h"
#include <WiFi.h>  



int relay = 17;
int buzzer = 14;


void setup() {
    Serial.begin(115200);
    Serial.println("Sistēmas inicializācija.v..");
    
    touchButtonInit(2, TOUCH_THRESHOLD, TOUCH_DEBOUNCE_MS);
    delay(500); // Pagaidām, lai nodrošinātu, ka sensori ir gatavi

    // Initialize display system first (with PSRAM optimization)
    lvgl_display_init();
    display_manager_init(); // Initialize display manager
    
    // Connect WiFi and sync time (this will now automatically update time display)
    wifi_connect();   // <-- Pieslēdz WiFi
    sync_time();      // <-- This will now automatically update time display!
    
    telegram_setup(); // <-- Inicializē Telegram bot
    initTemperatureSensor();
    ota_setup();
    delay(2000); // Pagaidām, lai nodrošinātu, ka sensori ir gatavi
    Serial.print("Mana IP adrese: ");
    Serial.println(WiFi.localIP());
    
    // Display update intervals configuration (EVENT-DRIVEN MODE):
    // display_manager_set_update_intervals(temp_ms, damper_ms, time_ms, touch_ms)
    // temp_ms   = 0ms     - Temperatūra: tikai kad mainās (nav laika limits!)
    // damper_ms = 0ms     - Damper: tikai kad mainās (nav laika limits!)  
    // time_ms   = 30000ms - Laika attēlošana (katras 30 sekundes)
    // touch_ms  = 50ms    - Touch response (20 FPS, labs response)
    // 
    // PURE EVENT-DRIVEN: Temp un Damper atjaunojas TIKAI kad tiešām mainās!
    // Nav nevajadzīgu fallback atjauninājumu!
    display_manager_set_update_intervals(0, 0, 30000, 50);

    lvgl_display_update_target_temp();
    
    delay(1000); // Pagaidām, lai nodrošinātu, ka displejs ir gatavs
    damperControlInit();
    startDamperControlTask();

    // Final time display check (backup if NTP didn't work immediately)
    Serial.println("Final time display initialization check...");
    show_time_on_display();           // Display time immediately

    


}

void loop() {
    // Core system functions
    lv_timer_handler();
    ota_loop();
    handleTelegramMessages(); // Apstrādā Telegram ziņojumus

    // Sensor updates
    updateTemperature();
    touchButtonHandle();

    // All display logic handled by display manager
    display_manager_update();

    // LVGL timing
    lv_tick_inc(5);
    delay(5);
}