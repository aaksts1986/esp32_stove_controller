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
#include "settings_storage.h" // Iestatījumu saglabāšanas bibliotēka
#include "telnet.h" // Telnet servera bibliotēka

int relay = 17;

void setup() {
    Serial.begin(115200);
    Serial.println("Sistēmas inicializācija.v..");
    
    // Inicializējam iestatījumu saglabāšanas sistēmu un ielādējam iepriekšējos iestatījumus
    initSettingsStorage();
    
    touchButtonInit(2, TOUCH_THRESHOLD, TOUCH_DEBOUNCE_MS);
    
    // Initialize display system first (with PSRAM optimization)
    lvgl_display_init();
    display_manager_init(); // Initialize display manager
    
    // Connect WiFi and sync time (this will now automatically update time display)
    wifi_connect();   // <-- Pieslēdz WiFi
    sync_time();      // <-- This will now automatically update time display!
    
    telegram_setup(); // <-- Inicializē Telegram bot
    initTemperatureSensor();
    ota_setup();

    Serial.print("Mana IP adrese: ");
    Serial.println(WiFi.localIP());

    // Inicializējam Telnet serveri
    Telnet.begin(23);

    delay(3000); // Pagaidām, lai nodrošinātu, ka sensori ir gatavi

    // display_manager_set_update_intervals(temp_ms, damper_ms, time_ms, touch_ms)
    // temp_ms   = 0ms     - Temperatūra: tikai kad mainās (nav laika limits!)
    // damper_ms = 0ms     - Damper: tikai kad mainās (nav laika limits!)  
    // time_ms   = 30000ms - Laika attēlošana (katras 30 sekundes)
    // touch_ms  = 50ms    - Touch response (20 FPS, labs response)
    // 
    display_manager_set_update_intervals(0, 0, 30000, 50);

    lvgl_display_update_target_temp();
    
    delay(500); // Pagaidām, lai nodrošinātu, ka displejs ir gatavs
    //amperControlInit();
    startDamperControlTask();

    // Final time display check (backup if NTP didn't work immediately)
    show_time_on_display();           // Display time immediately

    
}

void loop() {
    // Core system functions
    Telnet.handle(); // Apstrādā Telnet savienojumus

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