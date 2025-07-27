#include "settings_screen.h"
#include "lvgl_display.h"
#include "settings_storage.h"
#include "temperature.h"
#include "damper_control.h"
#include "display_config.h"
#include "display_manager.h"


// External variables
extern int targetTempC;
extern int maxTemp;
extern int temperatureMin;
extern int warningTemperature;

// Globālie mainīgie ekrāna iestatījumiem - pieejami arī ārpus šī faila
uint32_t timeUpdateIntervalMs = TIME_UPDATE_INTERVAL_MS;  // Noklusējuma vērtība no display_config.h
uint32_t touchUpdateIntervalMs = TOUCH_UPDATE_INTERVAL_MS;  // Noklusējuma vērtība no display_config.h
// Ekrāna spilgtums - globāli pieejams, lai to varētu izmantot lvgl_display.cpp
uint8_t screenBrightness = 255;  // Maksimālais spilgtums pēc noklusējuma (0-255)

// Global objects
static lv_obj_t * settings_screen = NULL;
static bool is_visible = false;
static lv_obj_t * tab_view = NULL;
static lv_obj_t * tab_temp = NULL;
static lv_obj_t * tab_damper = NULL;
static lv_obj_t * tab_servo = NULL;
static lv_obj_t * tab_display = NULL;
static lv_obj_t * tab_alarm = NULL;
static lv_obj_t * tab_system = NULL;

// Helper function to create standardized sliders
static lv_obj_t* create_settings_slider(lv_obj_t* parent, int x, int y, int min_val, int max_val, int current_val)
{
    lv_obj_t * slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 120);
    lv_obj_set_height(slider, 20);
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, x, y);
    lv_slider_set_range(slider, min_val, max_val);
    lv_slider_set_value(slider, current_val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x0088FF), LV_PART_INDICATOR);
    return slider;
}

// Generic slider event handler
static void generic_slider_event_handler(lv_event_t * e, void(*update_function)(int), const char* format)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    
    // Call the update function to set the global variable
    if (update_function) {
        update_function(value);
    }
    
    // Update the display label
    lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
    if (label && format) {
        lv_label_set_text_fmt(label, format, value);
    }
}

// Update functions for global variables
static void update_temperatureMin(int value) { temperatureMin = value; }
static void update_servoAngle(int value) { servoAngle = value; }
static void update_servoOffset(int value) { servoOffset = value; }
static void update_servoStepInterval(int value) { servoStepInterval = value; }
static void update_warningTemperature(int value) { warningTemperature = value; }

// Helper function to update slider and label values in settings_screen_show()
static void update_slider_value(lv_obj_t* tab, int slider_index, int value, const char* format)
{
    lv_obj_t * slider = lv_obj_get_child(tab, slider_index);
    if (slider) {
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        
        lv_obj_t * label = lv_obj_get_child(tab, slider_index + 1);
        if (label && format) {
            lv_label_set_text_fmt(label, format, value);
        }
    }
}

// Create settings screen
void settings_screen_create(void)
{
    if (settings_screen) {
        return;
    }
    
    // Main screen
    settings_screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(settings_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(settings_screen, 0, 0);
    lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
    
    // Back button
    lv_obj_t * back_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_set_pos(back_btn, 20, 20);
    
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        lvgl_display_close_settings();
    }, LV_EVENT_CLICKED, NULL);
    
    // Save button
    lv_obj_t * save_btn = lv_btn_create(settings_screen);
    lv_obj_set_size(save_btn, 80, 40);
    lv_obj_set_pos(save_btn, LV_HOR_RES - 120, 20);
    
    lv_obj_t * save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "Save");
    lv_obj_center(save_label);
    
    lv_obj_add_event_cb(save_btn, [](lv_event_t * e) {
        bool success = saveAllSettings();
        if (success) {
            lvgl_display_close_settings();
        }
    }, LV_EVENT_CLICKED, NULL);
    
    // Create tabview
    tab_view = lv_tabview_create(settings_screen, LV_DIR_LEFT, 80);
    lv_obj_set_size(tab_view, LV_HOR_RES-30 , LV_VER_RES - 100);
    lv_obj_set_pos(tab_view, 0, 70);
    
    // Create tabs with names
    tab_temp = lv_tabview_add_tab(tab_view, "Temp.");
    tab_damper = lv_tabview_add_tab(tab_view, "Damper");
    tab_servo = lv_tabview_add_tab(tab_view, "Servo");
    tab_display = lv_tabview_add_tab(tab_view, "Display");
    tab_alarm = lv_tabview_add_tab(tab_view, "Alarm");
    tab_system = lv_tabview_add_tab(tab_view, "System");
    
    // --- Temperature Tab Content ---
    
    // Target temperature slider
    lv_obj_t * target_temp_label = lv_label_create(tab_temp);
    lv_label_set_text(target_temp_label, "Target Temp");
    lv_obj_align(target_temp_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    // Nodrošinām, ka targetTempC ir atbilstošā diapazonā un ievēro 2 grādu soli
    if (targetTempC < 62) targetTempC = 62;
    if (targetTempC > 80) targetTempC = 80;
    targetTempC = ((targetTempC + 1) / 2) * 2; // Noapaļojam uz tuvāko pāra skaitli
    
    lv_obj_t * target_temp_slider = create_settings_slider(tab_temp, 10, 40, 62, 80, targetTempC);
    lv_slider_set_mode(target_temp_slider, LV_SLIDER_MODE_NORMAL);  // Step mode
    
    // Iestatām 2 grādu soli, kad slaidera kustība ir pabeigta
    lv_obj_add_event_cb(target_temp_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider);
        // Noapaļojam uz tuvāko pāra skaitli
        value = ((value + 1) / 2) * 2;
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        // Svarīgi: atjauninām globālo mainīgo
        targetTempC = value;
        // Arī jāatjaunina visi citi elementi, kas izmanto šo vērtību
        // (šī funkcija tiek izsaukta, kad lietotājs atlaiž slaideri)
    }, LV_EVENT_RELEASED, NULL);
    
    lv_obj_t * target_temp_value = lv_label_create(tab_temp);
    lv_label_set_text_fmt(target_temp_value, "%d°C", targetTempC);
    // Pozicionē vērtību tieši blakus slaidera beigām (80px garums + 10px sākums + 5px atstarpe)
    lv_obj_align(target_temp_value, LV_ALIGN_TOP_LEFT, 140, 40);
    
    // Apstrādājam vērtības maiņu, kad slaideris tiek vilkts
    lv_obj_add_event_cb(target_temp_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        // Pielietojam 2 grādu soli
        value = ((value + 1) / 2) * 2; // Noapaļojam uz tuvāko pāra skaitli
        
        // Atjauninām globālo mainīgo - šis ir svarīgi!
        targetTempC = value;
        
        // Atjauninām slaidera pozīciju
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        
        // Atjauninām cipara rādījumu blakus slaiderim
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d°C", value);
        
        // SVARĪGI: Sinhronizējam ar galveno ekrānu dzīvajā laikā
        // Šis ļaus lietotājam redzēt izmaiņas jau slīdināšanas laikā
        lvgl_display_update_target_temp();
    }, LV_EVENT_VALUE_CHANGED, target_temp_value);
    
    // Min temperature slider (pārvietojam uz augšu, kur iepriekš bija Max Temp)
    lv_obj_t * min_temp_label = lv_label_create(tab_temp);
    lv_label_set_text(min_temp_label, "Min Temp");
    lv_obj_align(min_temp_label, LV_ALIGN_TOP_LEFT, 10, 80);  // Bija 150, tagad 80
    
    lv_obj_t * min_temp_slider = create_settings_slider(tab_temp, 10, 110, 40, 55, temperatureMin);
    
    lv_obj_t * min_temp_value = lv_label_create(tab_temp);
    lv_label_set_text_fmt(min_temp_value, "%d°C", temperatureMin);
    // Pozicionē vērtību tieši blakus slaidera beigām
    lv_obj_align(min_temp_value, LV_ALIGN_TOP_LEFT, 140, 110);  // Bija 180, tagad 110
    
    lv_obj_add_event_cb(min_temp_slider, [](lv_event_t * e) {
        generic_slider_event_handler(e, update_temperatureMin, "%d°C");
    }, LV_EVENT_VALUE_CHANGED, min_temp_value);
    
    // Read interval slider (pārvietojam uz augšu)
    lv_obj_t * read_interval_label = lv_label_create(tab_temp);
    lv_label_set_text(read_interval_label, "Read Interval");
    lv_obj_align(read_interval_label, LV_ALIGN_TOP_LEFT, 10, 150);  // Bija 220, tagad 150
    
    // Izmantojam globālo mainīgo tempReadIntervalMs
    // Pārbaudam, vai tas ir pieņemamajā diapazonā
    if (tempReadIntervalMs < 2000) tempReadIntervalMs = 2000;
    if (tempReadIntervalMs > 10000) tempReadIntervalMs = 10000;
    // Noapaļojam uz tuvāko 2 sekunžu (2000ms) soli
    tempReadIntervalMs = ((tempReadIntervalMs + 1000) / 2000) * 2000;
    
    lv_obj_t * read_interval_slider = create_settings_slider(tab_temp, 10, 180, 2000, 10000, tempReadIntervalMs);
    
    lv_obj_t * read_interval_value = lv_label_create(tab_temp);
    lv_label_set_text_fmt(read_interval_value, "%d.0 s", tempReadIntervalMs/1000);  // Display in seconds
    // Pozicionē vērtību tieši blakus slaidera beigām
    lv_obj_align(read_interval_value, LV_ALIGN_TOP_LEFT, 140, 180);  // Bija 250, tagad 180
    
    lv_obj_add_event_cb(read_interval_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        
        // Apply 2-second steps (2000ms steps)
        value = ((value + 1000) / 2000) * 2000;
        if (value < 2000) value = 2000;
        if (value > 10000) value = 10000;
        
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        
        // Atjauninām globālo mainīgo un attēloto vērtību
        tempReadIntervalMs = value;
        
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d.0 s", value/1000);  // Convert to seconds for display
    }, LV_EVENT_VALUE_CHANGED, read_interval_value);
    
    // --- Damper Tab Content ---
    
    // kP slider - PID proporcionalitātes koeficients
    lv_obj_t * kp_label = lv_label_create(tab_damper);
    lv_label_set_text(kp_label, "kP Value");
    lv_obj_align(kp_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    lv_obj_t * kp_slider = create_settings_slider(tab_damper, 10, 40, 1, 100, kP);
    
    lv_obj_t * kp_value = lv_label_create(tab_damper);
    lv_label_set_text_fmt(kp_value, "%d", kP);
    lv_obj_align(kp_value, LV_ALIGN_TOP_LEFT, 140, 40);
    
    lv_obj_add_event_cb(kp_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        kP = value;
        // Aprēķinam atkarīgās vērtības
        kI = kP / tauI;
        kD = kP * tauD;
        
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d", value);
    }, LV_EVENT_VALUE_CHANGED, kp_value);
    
    // tauD slider - PID atvasināšanas laika konstante
    lv_obj_t * taud_label = lv_label_create(tab_damper);
    lv_label_set_text(taud_label, "tauD Value");
    lv_obj_align(taud_label, LV_ALIGN_TOP_LEFT, 10, 80);
    
    lv_obj_t * taud_slider = create_settings_slider(tab_damper, 10, 110, 1, 100, tauD);
    
    lv_obj_t * taud_value = lv_label_create(tab_damper);
    lv_label_set_text_fmt(taud_value, "%d", (int)tauD);
    lv_obj_align(taud_value, LV_ALIGN_TOP_LEFT, 140, 110);
    
    lv_obj_add_event_cb(taud_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        tauD = value;
        // Aprēķinam atkarīgās vērtības
        kD = kP * tauD;
        
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d", value);
    }, LV_EVENT_VALUE_CHANGED, taud_value);
    
    // endTrigger slider - beigu slieksnis
    lv_obj_t * end_trigger_label = lv_label_create(tab_damper);
    lv_label_set_text(end_trigger_label, "End Trigger");
    lv_obj_align(end_trigger_label, LV_ALIGN_TOP_LEFT, 10, 150);
    
    lv_obj_t * end_trigger_slider = create_settings_slider(tab_damper, 10, 180, 5000, 30000, endTrigger);
    
    lv_obj_t * end_trigger_value = lv_label_create(tab_damper);
    lv_label_set_text_fmt(end_trigger_value, "%d", (int)endTrigger);
    lv_obj_align(end_trigger_value, LV_ALIGN_TOP_LEFT, 140, 180);
    
    lv_obj_add_event_cb(end_trigger_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        endTrigger = value;
        
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d", value);
    }, LV_EVENT_VALUE_CHANGED, end_trigger_value);
    
    // LOW_TEMP_TIMEOUT slider - zemas temperatūras taimeris
    lv_obj_t * low_temp_timeout_label = lv_label_create(tab_damper);
    lv_label_set_text(low_temp_timeout_label, "Low Temp Timeout");
    lv_obj_align(low_temp_timeout_label, LV_ALIGN_TOP_LEFT, 10, 220);
    
    // Pārbaudam vai LOW_TEMP_TIMEOUT ir pieņemamajā diapazonā
    if (LOW_TEMP_TIMEOUT < 60000) LOW_TEMP_TIMEOUT = 60000; // Min 1 minūte
    if (LOW_TEMP_TIMEOUT > 600000) LOW_TEMP_TIMEOUT = 600000; // Max 10 minūtes
    // Noapaļojam uz tuvāko minūti (60000ms)
    LOW_TEMP_TIMEOUT = ((LOW_TEMP_TIMEOUT + 30000) / 60000) * 60000;
    
    lv_obj_t * low_temp_timeout_slider = create_settings_slider(tab_damper, 10, 250, 60000, 600000, LOW_TEMP_TIMEOUT);
    
    lv_obj_t * low_temp_timeout_value = lv_label_create(tab_damper);
    lv_label_set_text_fmt(low_temp_timeout_value, "%d min", (int)(LOW_TEMP_TIMEOUT/60000));
    lv_obj_align(low_temp_timeout_value, LV_ALIGN_TOP_LEFT, 140, 250);
    
    lv_obj_add_event_cb(low_temp_timeout_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        
        // Noapaļojam uz tuvāko minūti (60000ms)
        value = ((value + 30000) / 60000) * 60000;
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        
        // Atjauninām globālo mainīgo
        LOW_TEMP_TIMEOUT = value;
        
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d min", value/60000);
    }, LV_EVENT_VALUE_CHANGED, low_temp_timeout_value);
    
    // --- Servo Tab Content ---
    
    // servoAngle slider - Servo motora maksimālais leņķis
    lv_obj_t * servo_angle_label = lv_label_create(tab_servo);
    lv_label_set_text(servo_angle_label, "Servo Angle");
    lv_obj_align(servo_angle_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    lv_obj_t * servo_angle_slider = create_settings_slider(tab_servo, 10, 40, 10, 100, servoAngle);
    
    lv_obj_t * servo_angle_value = lv_label_create(tab_servo);
    lv_label_set_text_fmt(servo_angle_value, "%d°", servoAngle);
    lv_obj_align(servo_angle_value, LV_ALIGN_TOP_LEFT, 140, 40);
    
    lv_obj_add_event_cb(servo_angle_slider, [](lv_event_t * e) {
        generic_slider_event_handler(e, update_servoAngle, "%d°");
    }, LV_EVENT_VALUE_CHANGED, servo_angle_value);
    
    // servoOffset slider - Servo pozīcijas nobīde
    lv_obj_t * servo_offset_label = lv_label_create(tab_servo);
    lv_label_set_text(servo_offset_label, "Servo Offset");
    lv_obj_align(servo_offset_label, LV_ALIGN_TOP_LEFT, 10, 80);
    
    lv_obj_t * servo_offset_slider = create_settings_slider(tab_servo, 10, 110, 0, 90, servoOffset);
    
    lv_obj_t * servo_offset_value = lv_label_create(tab_servo);
    lv_label_set_text_fmt(servo_offset_value, "%d°", servoOffset);
    lv_obj_align(servo_offset_value, LV_ALIGN_TOP_LEFT, 140, 110);
    
    lv_obj_add_event_cb(servo_offset_slider, [](lv_event_t * e) {
        generic_slider_event_handler(e, update_servoOffset, "%d°");
    }, LV_EVENT_VALUE_CHANGED, servo_offset_value);
    
    // servoStepInterval slider - Servo kustības ātrums
    lv_obj_t * servo_step_interval_label = lv_label_create(tab_servo);
    lv_label_set_text(servo_step_interval_label, "Servo Step Interval");
    lv_obj_align(servo_step_interval_label, LV_ALIGN_TOP_LEFT, 10, 150);
    
    lv_obj_t * servo_step_interval_slider = create_settings_slider(tab_servo, 10, 180, 10, 200, servoStepInterval);
    
    lv_obj_t * servo_step_interval_value = lv_label_create(tab_servo);
    lv_label_set_text_fmt(servo_step_interval_value, "%d ms", servoStepInterval);
    lv_obj_align(servo_step_interval_value, LV_ALIGN_TOP_LEFT, 140, 180);
    
    lv_obj_add_event_cb(servo_step_interval_slider, [](lv_event_t * e) {
        generic_slider_event_handler(e, update_servoStepInterval, "%d ms");
    }, LV_EVENT_VALUE_CHANGED, servo_step_interval_value);
    
    // --- Display Tab Content ---
    
    // Ekrāna spilgtuma slaideris (LCD Backlight)
    lv_obj_t * brightness_label = lv_label_create(tab_display);
    lv_label_set_text(brightness_label, "display brightness");
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    lv_obj_t * brightness_slider = create_settings_slider(tab_display, 10, 40, 20, 255, screenBrightness);
    
    lv_obj_t * brightness_value = lv_label_create(tab_display);
    lv_label_set_text_fmt(brightness_value, "%d%%", (int)(screenBrightness * 100 / 255));
    lv_obj_align(brightness_value, LV_ALIGN_TOP_LEFT, 140, 40);
    
    lv_obj_add_event_cb(brightness_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        
        // Atjauninām screenBrightness mainīgo
        screenBrightness = value;
        
        // Atjauninām attēloto vērtību procentos
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d%%", (int)(value * 100 / 255));
        
        // Atjauninām ekrāna spilgtumu
        lvgl_display_set_brightness(value);
    }, LV_EVENT_VALUE_CHANGED, brightness_value);
    
    // Laika atjaunināšanas intervāla slaideris (TIME_UPDATE_INTERVAL_MS)
    lv_obj_t * time_update_label = lv_label_create(tab_display);
    lv_label_set_text(time_update_label, "Time Update");
    lv_obj_align(time_update_label, LV_ALIGN_TOP_LEFT, 10, 80);
    
    lv_obj_t * time_update_slider = create_settings_slider(tab_display, 10, 110, 10000, 60000, timeUpdateIntervalMs);
    
    lv_obj_t * time_update_value = lv_label_create(tab_display);
    lv_label_set_text_fmt(time_update_value, "%d s", timeUpdateIntervalMs / 1000);
    lv_obj_align(time_update_value, LV_ALIGN_TOP_LEFT, 140, 110);
    
    lv_obj_add_event_cb(time_update_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        
        // Noapaļojam uz tuvāko 10 sekunžu soli
        value = ((value + 5000) / 10000) * 10000;
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        
        // Atjauninām globālo mainīgo un attēloto vērtību
        timeUpdateIntervalMs = value;
        
        // Atjauninām display manager konfigurāciju
        display_manager_set_update_intervals(
            DEFAULT_TEMP_UPDATE_INTERVAL,
            DEFAULT_DAMPER_UPDATE_INTERVAL,
            timeUpdateIntervalMs,
            touchUpdateIntervalMs
        );
        
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d s", value / 1000);
    }, LV_EVENT_VALUE_CHANGED, time_update_value);
    
    // Skārienekrāna atjaunināšanas intervāla slaideris (TOUCH_UPDATE_INTERVAL_MS)
    lv_obj_t * touch_update_label = lv_label_create(tab_display);
    lv_label_set_text(touch_update_label, "touch refresh");
    lv_obj_align(touch_update_label, LV_ALIGN_TOP_LEFT, 10, 150);
    
    lv_obj_t * touch_update_slider = create_settings_slider(tab_display, 10, 180, 20, 100, touchUpdateIntervalMs);
    
    lv_obj_t * touch_update_value = lv_label_create(tab_display);
    lv_label_set_text_fmt(touch_update_value, "%d ms", touchUpdateIntervalMs);
    lv_obj_align(touch_update_value, LV_ALIGN_TOP_LEFT, 140, 180);
    
    lv_obj_add_event_cb(touch_update_slider, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        
        // Noapaļojam uz tuvāko 5 ms soli
        value = ((value + 2) / 5) * 5;
        if (value < 20) value = 20;  // Minimālā vērtība
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        
        // Atjauninām globālo mainīgo un attēloto vērtību
        touchUpdateIntervalMs = value;
        
        // Atjauninām display manager konfigurāciju
        display_manager_set_update_intervals(
            DEFAULT_TEMP_UPDATE_INTERVAL,
            DEFAULT_DAMPER_UPDATE_INTERVAL,
            timeUpdateIntervalMs,
            touchUpdateIntervalMs
        );
        
        lv_obj_t * label = (lv_obj_t*)lv_event_get_user_data(e);
        lv_label_set_text_fmt(label, "%d ms", value);
    }, LV_EVENT_VALUE_CHANGED, touch_update_value);
    
    // --- Alarm Tab Content ---
    
    // Warning temperature slider
    lv_obj_t * warning_temp_label = lv_label_create(tab_alarm);
    lv_label_set_text(warning_temp_label, "Warning Temp");
    lv_obj_align(warning_temp_label, LV_ALIGN_TOP_LEFT, 10, 10);
    
    lv_obj_t * warning_temp_slider = create_settings_slider(tab_alarm, 10, 40, 80, 95, warningTemperature);
    
    lv_obj_t * warning_temp_value = lv_label_create(tab_alarm);
    lv_label_set_text_fmt(warning_temp_value, "%d°C", warningTemperature);
    // Pozicionē vērtību tieši blakus slaidera beigām
    lv_obj_align(warning_temp_value, LV_ALIGN_TOP_LEFT, 140, 40);
    
    lv_obj_add_event_cb(warning_temp_slider, [](lv_event_t * e) {
        generic_slider_event_handler(e, update_warningTemperature, "%d°C");
    }, LV_EVENT_VALUE_CHANGED, warning_temp_value);
}

// Show settings screen
void settings_screen_show(void)
{
    if (!settings_screen) {
        settings_screen_create();
    }
    is_visible = true;
    lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
    
    // SVARĪGI: Atjauninām iestatījumu ekrāna vērtības no globālajiem mainīgajiem
    if (tab_temp) {
        // Atrodam target temp slaideri un atjauninām tā vērtību
        lv_obj_t * target_temp_slider = lv_obj_get_child(tab_temp, 1); // Target temp slider (0=label, 1=slider)
        if (target_temp_slider) {
            // Pārbaudam, vai targetTempC ir atbilstošā diapazonā un ievēro 2 grādu soli
            int corrected_value = targetTempC;
            if (corrected_value < 62) corrected_value = 62;
            if (corrected_value > 80) corrected_value = 80;
            corrected_value = ((corrected_value + 1) / 2) * 2; // Noapaļojam uz tuvāko pāra skaitli
            
            // Ja vērtība tika koriģēta, atjauninām arī globālo mainīgo
            if (corrected_value != targetTempC) {
                Serial.printf("Correcting targetTempC from %d to %d\n", targetTempC, corrected_value);
                targetTempC = corrected_value;
                // Ja mainījās vērtība, atjauninām arī galveno ekrānu
                lvgl_display_update_target_temp();
            }
            
            // Iestatām slaidera vērtību
            lv_slider_set_value(target_temp_slider, targetTempC, LV_ANIM_OFF);
            
            // Atjauninām arī redzamo vērtību
            lv_obj_t * target_temp_value = lv_obj_get_child(tab_temp, 2); // Target temp value label
            if (target_temp_value) {
                lv_label_set_text_fmt(target_temp_value, "%d°C", targetTempC);
            }
        }
        
        // Pēc maxTemp slaidera izņemšanas, indeksi ir mainījušies
        // Min temp label = 3, Min temp slider = 4, Min temp value = 5
        update_slider_value(tab_temp, 4, temperatureMin, "%d°C");
        
        // Read interval label = 6, Read interval slider = 7, Read interval value = 8
        lv_obj_t * read_interval_slider = lv_obj_get_child(tab_temp, 7); // Read interval slider
        if (read_interval_slider) {
            // Pārbaudam, vai tempReadIntervalMs ir pieņemamā diapazonā un ievēro 2s soli
            if (tempReadIntervalMs < 2000) tempReadIntervalMs = 2000;
            if (tempReadIntervalMs > 10000) tempReadIntervalMs = 10000;
            tempReadIntervalMs = ((tempReadIntervalMs + 1000) / 2000) * 2000;
            
            lv_slider_set_value(read_interval_slider, tempReadIntervalMs, LV_ANIM_OFF);
            
            // Atjauninām arī redzamo vērtību
            lv_obj_t * read_interval_value = lv_obj_get_child(tab_temp, 8); // Read interval value label
            if (read_interval_value) {
                lv_label_set_text_fmt(read_interval_value, "%d.0 s", tempReadIntervalMs/1000);
            }
        }
    }
    
    // Atjauninām damper cilnes vērtības
    if (tab_damper) {
        // kP slider
        update_slider_value(tab_damper, 1, kP, "%d");
        
        // tauD slider
        update_slider_value(tab_damper, 4, tauD, "%d");
        
        // endTrigger slider
        update_slider_value(tab_damper, 7, endTrigger, "%d");
        
        // LOW_TEMP_TIMEOUT slider
        lv_obj_t * low_temp_timeout_slider = lv_obj_get_child(tab_damper, 10); // LOW_TEMP_TIMEOUT slider
        if (low_temp_timeout_slider) {
            // Pārbaudam vai LOW_TEMP_TIMEOUT ir pieņemamajā diapazonā
            if (LOW_TEMP_TIMEOUT < 60000) LOW_TEMP_TIMEOUT = 60000; // Min 1 minūte
            if (LOW_TEMP_TIMEOUT > 600000) LOW_TEMP_TIMEOUT = 600000; // Max 10 minūtes
            
            // Noapaļojam uz tuvāko minūti (60000ms)
            LOW_TEMP_TIMEOUT = ((LOW_TEMP_TIMEOUT + 30000) / 60000) * 60000;
            
            lv_slider_set_value(low_temp_timeout_slider, LOW_TEMP_TIMEOUT, LV_ANIM_OFF);
            
            lv_obj_t * low_temp_timeout_value = lv_obj_get_child(tab_damper, 11); // LOW_TEMP_TIMEOUT value label
            if (low_temp_timeout_value) {
                lv_label_set_text_fmt(low_temp_timeout_value, "%d min", (int)(LOW_TEMP_TIMEOUT/60000));
            }
        }
    }
    
    // Atjauninām servo cilnes vērtības
    if (tab_servo) {
        // servoAngle slider
        update_slider_value(tab_servo, 1, servoAngle, "%d°");
        
        // servoOffset slider
        update_slider_value(tab_servo, 4, servoOffset, "%d°");
        
        // servoStepInterval slider
        update_slider_value(tab_servo, 7, servoStepInterval, "%d ms");
    }
    
    // Atjauninām alarm cilnes warningTemperature slaideri
    if (tab_alarm) {
        // Warning temp slider
        update_slider_value(tab_alarm, 1, warningTemperature, "%d°C");
    }
    
    // Atjauninām display cilnes vērtības
    if (tab_display) {
        // Ekrāna spilgtuma slaideris
        lv_obj_t * brightness_slider = lv_obj_get_child(tab_display, 1); // Spilgtuma slaideris
        if (brightness_slider) {
            lv_slider_set_value(brightness_slider, screenBrightness, LV_ANIM_OFF); // Izmantojam faktisko vērtību
            
            lv_obj_t * brightness_value = lv_obj_get_child(tab_display, 2); // Spilgtuma vērtības apzīmējums
            if (brightness_value) {
                lv_label_set_text_fmt(brightness_value, "%d%%", (int)(screenBrightness * 100 / 255)); // Actual percentage
            }
        }
        
        // Laika atjaunināšanas intervāla slaideris
        update_slider_value(tab_display, 4, timeUpdateIntervalMs / 1000, "%d s");
        
        // Skārienekrāna atjaunināšanas intervāla slaideris
        update_slider_value(tab_display, 7, touchUpdateIntervalMs, "%d ms");
    }
}

// Hide settings screen
void settings_screen_hide(void)
{
    if (settings_screen) {
        lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
        is_visible = false;
        
        // SVARĪGI: Atjauninām galvenā ekrāna target temp rādījumu, kad aizveram iestatījumu ekrānu
        // Šis nodrošinās, ka galvenā ekrāna vērtība tiek sinhronizēta ar iestatījumu ekrāna vērtību
        lvgl_display_update_target_temp();
    }
}

// Check if settings screen is visible
bool settings_screen_is_visible(void)
{
    return is_visible;
}