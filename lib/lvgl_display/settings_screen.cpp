#include "settings_screen.h"
#include "lvgl_display.h"
#include "temperature.h"
#include "damper_control.h"

// Global objects
static lv_obj_t * settings_screen = NULL;
static lv_obj_t * settings_container = NULL;
static bool is_visible = false;
static lv_obj_t * temp_roller = NULL;
static lv_obj_t * kp_roller = NULL;
static lv_obj_t * min_temp_roller = NULL;
static lv_obj_t * max_temp_roller = NULL;
static lv_obj_t * end_trigger_roller = NULL;
static lv_obj_t * servo_angle_roller = NULL;
static lv_obj_t * servo_step_roller = NULL;
static lv_obj_t * warning_temp_roller = NULL; // Jauns: brīdinājuma temperatūras rolleris
static bool roller_initialized = false;

// Temperature mapping arrays
static const int temp_values[] = {64, 66, 68, 70, 72, 74, 76, 78, 80};
static const int temp_count = 9;

// Brīdinājuma temperatūras vērtības
static const int warning_temp_values[] = {81, 83, 85, 87, 90};
static const int warning_temp_count = 5;

// kP mapping array
static const int kp_values[] = {5, 10, 15, 20, 25, 30, 35, 40, 45};
static const int kp_count = 9;

// Min temperature mapping array
static const int min_temp_values[] = {40, 45, 50, 55, 60, 65};
static const int min_temp_count = 6;

// Max temperature mapping array
static const int max_temp_values[] = {80, 85, 90, 95, 100, 105, 110};
static const int max_temp_count = 7;

// End trigger mapping array
static const int end_trigger_values[] = {5000, 7500, 10000, 12500, 15000, 17500, 20000};
static const int end_trigger_count = 7;

// Servo angle mapping array
static const int servo_angle_values[] = {25, 30, 35, 40, 45, 50};
static const int servo_angle_count = 6;

// Servo step interval mapping array (ms)
static const int servo_step_values[] = {20, 30, 50, 75, 100};
static const int servo_step_count = 5;

// Atrod temperature indeksu array
static int get_temp_index(int temperature)
{
    for (int i = 0; i < temp_count; i++) {
        if (temp_values[i] == temperature) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (temperature < 64) return 0;
    if (temperature > 80) return 8;
    
    // Atrod tuvāko
    for (int i = 0; i < temp_count - 1; i++) {
        if (temperature >= temp_values[i] && temperature < temp_values[i + 1]) {
            return i;
        }
    }
    return 4; // Default 70°C
}

// Atrod kP indeksu array
static int get_kp_index(int kp_value)
{
    for (int i = 0; i < kp_count; i++) {
        if (kp_values[i] == kp_value) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (kp_value < 5) return 0;
    if (kp_value > 45) return 8;
    
    // Atrod tuvāko
    for (int i = 0; i < kp_count - 1; i++) {
        if (kp_value >= kp_values[i] && kp_value < kp_values[i + 1]) {
            return i;
        }
    }
    return 2; // Default 15
}

// Atrod min temperaturas indeksu array
static int get_min_temp_index(int min_temp)
{
    for (int i = 0; i < min_temp_count; i++) {
        if (min_temp_values[i] == min_temp) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (min_temp < 40) return 0;
    if (min_temp > 65) return min_temp_count - 1;
    
    // Atrod tuvāko
    for (int i = 0; i < min_temp_count - 1; i++) {
        if (min_temp >= min_temp_values[i] && min_temp < min_temp_values[i + 1]) {
            return i;
        }
    }
    return 2; // Default 50°C
}

// Atrod max temperaturas indeksu array
static int get_max_temp_index(int max_temp)
{
    for (int i = 0; i < max_temp_count; i++) {
        if (max_temp_values[i] == max_temp) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (max_temp < 80) return 0;
    if (max_temp > 110) return max_temp_count - 1;
    
    // Atrod tuvāko
    for (int i = 0; i < max_temp_count - 1; i++) {
        if (max_temp >= max_temp_values[i] && max_temp < max_temp_values[i + 1]) {
            return i;
        }
    }
    return 2; // Default 90°C
}

// Atrod end trigger indeksu array
static int get_end_trigger_index(float trigger)
{
    for (int i = 0; i < end_trigger_count; i++) {
        if (end_trigger_values[i] == (int)trigger) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (trigger < 5000) return 0;
    if (trigger > 20000) return end_trigger_count - 1;
    
    // Atrod tuvāko
    for (int i = 0; i < end_trigger_count - 1; i++) {
        if (trigger >= end_trigger_values[i] && trigger < end_trigger_values[i + 1]) {
            return i;
        }
    }
    return 2; // Default 10000
}

// Atrod servo angle indeksu array
static int get_servo_angle_index(int angle)
{
    for (int i = 0; i < servo_angle_count; i++) {
        if (servo_angle_values[i] == angle) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (angle < 25) return 0;
    if (angle > 50) return servo_angle_count - 1;
    
    // Atrod tuvāko
    for (int i = 0; i < servo_angle_count - 1; i++) {
        if (angle >= servo_angle_values[i] && angle < servo_angle_values[i + 1]) {
            return i;
        }
    }
    return 2; // Default 35
}

// Atrod servo step interval indeksu array
static int get_servo_step_index(int step)
{
    for (int i = 0; i < servo_step_count; i++) {
        if (servo_step_values[i] == step) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (step < 20) return 0;
    if (step > 100) return servo_step_count - 1;
    
    // Atrod tuvāko
    for (int i = 0; i < servo_step_count - 1; i++) {
        if (step >= servo_step_values[i] && step < servo_step_values[i + 1]) {
            return i;
        }
    }
    return 2; // Default 50ms
}

// Atrod brīdinājuma temperatūras indeksu array
static int get_warning_temp_index(int temp)
{
    for (int i = 0; i < warning_temp_count; i++) {
        if (warning_temp_values[i] == temp) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (temp < 81) return 0;
    if (temp > 90) return warning_temp_count - 1;
    
    // Atrod tuvāko
    for (int i = 0; i < warning_temp_count - 1; i++) {
        if (temp >= warning_temp_values[i] && temp < warning_temp_values[i + 1]) {
            return i;
        }
    }
    return 2; // Default 85
}

// Roller event handler
static void temp_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != temp_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= temp_count) {
        return;
    }
    
    int old_temp = targetTempC;
    int new_temp = temp_values[selected];
    
    targetTempC = new_temp;
    lvgl_display_update_target_temp();
}

// kP roller event handler
static void kp_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != kp_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= kp_count) {
        return;
    }
    
    int old_kp = kP;
    int new_kp = kp_values[selected];
    
    kP = new_kp;
    
    // Atjaunojam atkarīgos parametrus
    kI = kP / tauI;
    kD = kP * tauD;
}

// Min temperature roller event handler
static void min_temp_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != min_temp_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= min_temp_count) {
        return;
    }
    
    int old_temp = temperatureMin;
    int new_temp = min_temp_values[selected];
    
    temperatureMin = new_temp;
}

// Max temperature roller event handler
static void max_temp_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != max_temp_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= max_temp_count) {
        return;
    }
    
    int old_temp = maxTemp;
    int new_temp = max_temp_values[selected];
    
    maxTemp = new_temp;
}

// End trigger roller event handler
static void end_trigger_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != end_trigger_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= end_trigger_count) {
        return;
    }
    
    float old_trigger = endTrigger;
    float new_trigger = end_trigger_values[selected];
    
    endTrigger = new_trigger;
}

// Servo angle roller event handler
static void servo_angle_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != servo_angle_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= servo_angle_count) {
        return;
    }
    
    int old_angle = servoAngle;
    int new_angle = servo_angle_values[selected];
    
    servoAngle = new_angle;
}

// Servo step interval roller event handler
static void servo_step_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != servo_step_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= servo_step_count) {
        return;
    }
    
    int old_step = servoStepInterval;
    int new_step = servo_step_values[selected];
    
    servoStepInterval = new_step;
}

// Brīdinājuma temperatūras roller event handler
static void warning_temp_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != warning_temp_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu
    if (selected >= warning_temp_count) {
        return;
    }
    
    int new_temp = warning_temp_values[selected];
    
    // Atjaunojam mainīgo temperatūras modulī
    extern int warningTemperature;
    warningTemperature = new_temp;
}

// Animation callback for hiding screen
static void hide_animation_ready_cb(lv_anim_t * a)
{
    if (settings_screen) {
        lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
        is_visible = false;
    }
}

// Create settings screen
void settings_screen_create(void)
{
    if (settings_screen) {
        return;
    }
    
    // Izveidojam pilnekrāna konteineru
    settings_screen = lv_obj_create(lv_scr_act());
    if (!settings_screen) {
        return;
    }
    
    lv_obj_set_size(settings_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(settings_screen, 0, 0);
    lv_obj_set_style_bg_color(settings_screen, SETTINGS_BG_COLOR, 0);
    lv_obj_set_style_border_width(settings_screen, 0, 0);
    lv_obj_set_style_pad_all(settings_screen, 0, 0);
    lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
    
    // Galvenais konteiners
    settings_container = lv_obj_create(settings_screen);
    if (!settings_container) {
        return;
    }
    
    lv_obj_set_size(settings_container, LV_HOR_RES - 20, LV_VER_RES - 20);
    lv_obj_center(settings_container);
    lv_obj_set_style_bg_color(settings_container, SETTINGS_CARD_COLOR, 0);
    lv_obj_set_style_border_width(settings_container, 0, 0);
    lv_obj_set_style_radius(settings_container, 12, 0);
    lv_obj_set_style_shadow_width(settings_container, 10, 0);
    lv_obj_set_style_shadow_opa(settings_container, LV_OPA_10, 0);
    lv_obj_set_style_pad_all(settings_container, 20, 0);
    
    // Header
    lv_obj_t * header = lv_obj_create(settings_container);
    lv_obj_set_size(header, LV_PCT(100), 50);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    
    // Back button
    lv_obj_t * back_btn = lv_btn_create(header);
    lv_obj_set_size(back_btn, 50, 40);
    lv_obj_set_pos(back_btn, 0, 5);
    lv_obj_set_style_bg_color(back_btn, SETTINGS_ACCENT_COLOR, 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        lvgl_display_close_settings();
    }, LV_EVENT_CLICKED, NULL);
    
    // Title
    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "Iestatijumi");
    lv_obj_set_style_text_color(title, SETTINGS_TEXT_COLOR, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(title, 70, 12);
    
    // Target Temperature label
    lv_obj_t * temp_label = lv_label_create(settings_container);
    lv_label_set_text(temp_label, "Target temp:");
    lv_obj_set_pos(temp_label, 10, 80);
    lv_obj_set_style_text_color(temp_label, SETTINGS_TEXT_COLOR, 0);
    
    // Temperature roller
    temp_roller = lv_roller_create(settings_container);
    if (!temp_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(temp_roller, 
        "64°C\n66°C\n68°C\n70°C\n72°C\n74°C\n76°C\n78°C\n80°C", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(temp_roller, 140, 70);
    lv_obj_set_size(temp_roller, 60, 40);  // Mazāks augstums
    lv_roller_set_visible_row_count(temp_roller, 1);  // Tikai 1 rinda
    lv_obj_set_style_text_font(temp_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller
    lv_obj_set_style_bg_color(temp_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(temp_roller, 1, 0);
    lv_obj_set_style_bg_opa(temp_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(temp_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(temp_roller, 8, 0);
    
    // Set initial value BEFORE adding event callback
    int current_index = get_temp_index(targetTempC);
    
    lv_roller_set_selected(temp_roller, current_index, LV_ANIM_OFF);
    
    // Verify the set value
    uint16_t actual_index = lv_roller_get_selected(temp_roller);
    
    // NOW add event callback
    lv_obj_add_event_cb(temp_roller, temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Min Temp label
    lv_obj_t * min_temp_label = lv_label_create(settings_container);
    lv_label_set_text(min_temp_label, "Min temp:");
    lv_obj_set_pos(min_temp_label, 10, 120);
    lv_obj_set_style_text_color(min_temp_label, SETTINGS_TEXT_COLOR, 0);
    
    // Min Temp roller
    min_temp_roller = lv_roller_create(settings_container);
    if (!min_temp_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(min_temp_roller, 
        "40°C\n45°C\n50°C\n55°C\n60°C\n65°C", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(min_temp_roller, 140, 110);
    lv_obj_set_size(min_temp_roller, 60, 40);
    lv_roller_set_visible_row_count(min_temp_roller, 1);
    lv_obj_set_style_text_font(min_temp_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller - same style as temp roller
    lv_obj_set_style_bg_color(min_temp_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(min_temp_roller, 1, 0);
    lv_obj_set_style_bg_opa(min_temp_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(min_temp_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(min_temp_roller, 8, 0);
    
    // Set initial value
    int min_temp_index = get_min_temp_index(temperatureMin);
    
    lv_roller_set_selected(min_temp_roller, min_temp_index, LV_ANIM_OFF);
    
    // Add event callback
    lv_obj_add_event_cb(min_temp_roller, min_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Max Temp label
    lv_obj_t * max_temp_label = lv_label_create(settings_container);
    lv_label_set_text(max_temp_label, "Max temp:");
    lv_obj_set_pos(max_temp_label, 10, 160);
    lv_obj_set_style_text_color(max_temp_label, SETTINGS_TEXT_COLOR, 0);
    
    // Max Temp roller
    max_temp_roller = lv_roller_create(settings_container);
    if (!max_temp_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(max_temp_roller, 
        "80°C\n85°C\n90°C\n95°C\n100°C\n105°C\n110°C", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(max_temp_roller, 140, 150);
    lv_obj_set_size(max_temp_roller, 60, 40);
    lv_roller_set_visible_row_count(max_temp_roller, 1);
    lv_obj_set_style_text_font(max_temp_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller - same style as temp roller
    lv_obj_set_style_bg_color(max_temp_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(max_temp_roller, 1, 0);
    lv_obj_set_style_bg_opa(max_temp_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(max_temp_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(max_temp_roller, 8, 0);
    
    // Set initial value
    int max_temp_index = get_max_temp_index(maxTemp);
    
    lv_roller_set_selected(max_temp_roller, max_temp_index, LV_ANIM_OFF);
    
    // Add event callback
    lv_obj_add_event_cb(max_temp_roller, max_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // kP label
    lv_obj_t * kp_label = lv_label_create(settings_container);
    lv_label_set_text(kp_label, "kP vertiba:");
    lv_obj_set_pos(kp_label, 10, 200);
    lv_obj_set_style_text_color(kp_label, SETTINGS_TEXT_COLOR, 0);
    
    // kP roller
    kp_roller = lv_roller_create(settings_container);
    if (!kp_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(kp_roller, 
        "5\n10\n15\n20\n25\n30\n35\n40\n45", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(kp_roller, 140, 190);
    lv_obj_set_size(kp_roller, 60, 40);
    lv_roller_set_visible_row_count(kp_roller, 1);
    lv_obj_set_style_text_font(kp_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller - same style as temp roller
    lv_obj_set_style_bg_color(kp_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(kp_roller, 1, 0);
    lv_obj_set_style_bg_opa(kp_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(kp_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(kp_roller, 8, 0);
    
    // Set initial value
    int kp_index = get_kp_index(kP);
    
    lv_roller_set_selected(kp_roller, kp_index, LV_ANIM_OFF);
    
    // Add event callback
    lv_obj_add_event_cb(kp_roller, kp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // End trigger label
    lv_obj_t * end_trigger_label = lv_label_create(settings_container);
    lv_label_set_text(end_trigger_label, "End trigger:");
    lv_obj_set_pos(end_trigger_label, 10, 240);
    lv_obj_set_style_text_color(end_trigger_label, SETTINGS_TEXT_COLOR, 0);
    
    // End trigger roller
    end_trigger_roller = lv_roller_create(settings_container);
    if (!end_trigger_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(end_trigger_roller, 
        "5000\n7500\n10000\n12500\n15000\n17500\n20000", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(end_trigger_roller, 140, 230);
    lv_obj_set_size(end_trigger_roller, 80, 40);
    lv_roller_set_visible_row_count(end_trigger_roller, 1);
    lv_obj_set_style_text_font(end_trigger_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller - same style as other rollers
    lv_obj_set_style_bg_color(end_trigger_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(end_trigger_roller, 1, 0);
    lv_obj_set_style_bg_opa(end_trigger_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(end_trigger_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(end_trigger_roller, 8, 0);
    
    // Set initial value
    int end_trigger_index = get_end_trigger_index(endTrigger);
    
    lv_roller_set_selected(end_trigger_roller, end_trigger_index, LV_ANIM_OFF);
    
    // Add event callback
    lv_obj_add_event_cb(end_trigger_roller, end_trigger_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Servo angle label
    lv_obj_t * servo_angle_label = lv_label_create(settings_container);
    lv_label_set_text(servo_angle_label, "Servo lenkis:");
    lv_obj_set_pos(servo_angle_label, 10, 280);
    lv_obj_set_style_text_color(servo_angle_label, SETTINGS_TEXT_COLOR, 0);
    
    // Servo angle roller
    servo_angle_roller = lv_roller_create(settings_container);
    if (!servo_angle_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(servo_angle_roller, 
        "25°\n30°\n35°\n40°\n45°\n50°", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(servo_angle_roller, 140, 270);
    lv_obj_set_size(servo_angle_roller, 60, 40);
    lv_roller_set_visible_row_count(servo_angle_roller, 1);
    lv_obj_set_style_text_font(servo_angle_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller - same style as other rollers
    lv_obj_set_style_bg_color(servo_angle_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(servo_angle_roller, 1, 0);
    lv_obj_set_style_bg_opa(servo_angle_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(servo_angle_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(servo_angle_roller, 8, 0);
    
    // Set initial value
    int servo_angle_index = get_servo_angle_index(servoAngle);
    
    lv_roller_set_selected(servo_angle_roller, servo_angle_index, LV_ANIM_OFF);
    
    // Add event callback
    lv_obj_add_event_cb(servo_angle_roller, servo_angle_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Servo step interval label
    lv_obj_t * servo_step_label = lv_label_create(settings_container);
    lv_label_set_text(servo_step_label, "Servo atrums:");
    lv_obj_set_pos(servo_step_label, 10, 320);
    lv_obj_set_style_text_color(servo_step_label, SETTINGS_TEXT_COLOR, 0);
    
    // Servo step interval roller
    servo_step_roller = lv_roller_create(settings_container);
    if (!servo_step_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(servo_step_roller, 
        "20ms\n30ms\n50ms\n75ms\n100ms", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(servo_step_roller, 140, 310);
    lv_obj_set_size(servo_step_roller, 70, 40);
    lv_roller_set_visible_row_count(servo_step_roller, 1);
    lv_obj_set_style_text_font(servo_step_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller - same style as other rollers
    lv_obj_set_style_bg_color(servo_step_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(servo_step_roller, 1, 0);
    lv_obj_set_style_bg_opa(servo_step_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(servo_step_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(servo_step_roller, 8, 0);
    
    // Set initial value
    int servo_step_index = get_servo_step_index(servoStepInterval);
    
    lv_roller_set_selected(servo_step_roller, servo_step_index, LV_ANIM_OFF);
    
    // Add event callback
    lv_obj_add_event_cb(servo_step_roller, servo_step_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Brīdinājuma temperatūras slieksnis - label
    lv_obj_t * warning_temp_label = lv_label_create(settings_container);
    lv_label_set_text(warning_temp_label, "Brid. temp.:");
    lv_obj_set_pos(warning_temp_label, 10, 360);
    lv_obj_set_style_text_color(warning_temp_label, SETTINGS_TEXT_COLOR, 0);
    
    // Brīdinājuma temperatūras rolleris
    warning_temp_roller = lv_roller_create(settings_container);
    if (!warning_temp_roller) {
        return;
    }
    
    // Set roller options
    lv_roller_set_options(warning_temp_roller, 
        "81°C\n83°C\n85°C\n87°C\n90°C", 
        LV_ROLLER_MODE_NORMAL);
    
    // Position and style roller
    lv_obj_set_pos(warning_temp_roller, 140, 350);
    lv_obj_set_size(warning_temp_roller, 70, 40);
    lv_roller_set_visible_row_count(warning_temp_roller, 1);
    lv_obj_set_style_text_font(warning_temp_roller, &lv_font_montserrat_18, LV_PART_SELECTED | LV_STATE_DEFAULT);

    // Style the roller - same style as other rollers
    lv_obj_set_style_bg_color(warning_temp_roller, lv_color_white(), 0);
    lv_obj_set_style_border_width(warning_temp_roller, 1, 0);
    lv_obj_set_style_bg_opa(warning_temp_roller, 0, LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(warning_temp_roller, lv_color_hex(0xff000000), LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(warning_temp_roller, 8, 0);
    
    // Iestatam sākotnējo vērtību
    extern int warningTemperature;
    int warning_temp_index = get_warning_temp_index(warningTemperature);
    
    lv_roller_set_selected(warning_temp_roller, warning_temp_index, LV_ANIM_OFF);
    
    // Pievienojam event callback
    lv_obj_add_event_cb(warning_temp_roller, warning_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    roller_initialized = true;
}

// Update rollers to match current values
void settings_screen_update_roller(void)
{
    if (!roller_initialized) {
        return;
    }
    
    // Update temperature roller
    if (temp_roller) {
        int current_index = get_temp_index(targetTempC);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(temp_roller, temp_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(temp_roller, current_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(temp_roller, temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Update kP roller
    if (kp_roller) {
        int kp_index = get_kp_index(kP);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(kp_roller, kp_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(kp_roller, kp_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(kp_roller, kp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Update min temperature roller
    if (min_temp_roller) {
        int min_temp_index = get_min_temp_index(temperatureMin);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(min_temp_roller, min_temp_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(min_temp_roller, min_temp_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(min_temp_roller, min_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Update max temperature roller
    if (max_temp_roller) {
        int max_temp_index = get_max_temp_index(maxTemp);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(max_temp_roller, max_temp_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(max_temp_roller, max_temp_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(max_temp_roller, max_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Update end trigger roller
    if (end_trigger_roller) {
        int end_trigger_index = get_end_trigger_index(endTrigger);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(end_trigger_roller, end_trigger_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(end_trigger_roller, end_trigger_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(end_trigger_roller, end_trigger_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Update servo angle roller
    if (servo_angle_roller) {
        int servo_angle_index = get_servo_angle_index(servoAngle);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(servo_angle_roller, servo_angle_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(servo_angle_roller, servo_angle_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(servo_angle_roller, servo_angle_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Update servo step interval roller
    if (servo_step_roller) {
        int servo_step_index = get_servo_step_index(servoStepInterval);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(servo_step_roller, servo_step_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(servo_step_roller, servo_step_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(servo_step_roller, servo_step_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Update warning temperature roller
    if (warning_temp_roller) {
        extern int warningTemperature;
        int warning_temp_index = get_warning_temp_index(warningTemperature);
        
        // Temporarily remove event callback to prevent triggering
        lv_obj_remove_event_cb(warning_temp_roller, warning_temp_roller_event_handler);
        
        // Update roller
        lv_roller_set_selected(warning_temp_roller, warning_temp_index, LV_ANIM_OFF);
        
        // Re-add event callback
        lv_obj_add_event_cb(warning_temp_roller, warning_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
}

// Show settings screen
void settings_screen_show(void)
{
    if (!settings_screen) {
        settings_screen_create();
    } else {
        // Update roller if screen already exists
        settings_screen_update_roller();
    }
    
    if (settings_screen) {
        is_visible = true;
        lv_obj_clear_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
    }
}

// Hide settings screen
void settings_screen_hide(void)
{
    if (!settings_screen || !is_visible) return;
    
    lv_obj_add_flag(settings_screen, LV_OBJ_FLAG_HIDDEN);
    is_visible = false;
}

// Check if settings screen is visible
bool settings_screen_is_visible(void)
{
    return is_visible;
}

// Cleanup function (optional)
void settings_screen_cleanup(void)
{
    if (settings_screen) {
        lv_obj_del(settings_screen);
        settings_screen = NULL;
        settings_container = NULL;
        temp_roller = NULL;
        kp_roller = NULL;
        min_temp_roller = NULL;
        max_temp_roller = NULL;
        end_trigger_roller = NULL;
        servo_angle_roller = NULL;
        servo_step_roller = NULL;
        warning_temp_roller = NULL; // Jauns: brīdinājuma temperatūras rolleris
        roller_initialized = false;
        is_visible = false;
    }
}