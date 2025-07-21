#pragma once

#include <lvgl.h>
#include <Arduino.h>
// Note: TFT_eSPI.h is included in the .cpp file to avoid dependency issues

void lvgl_display_init();
void lvgl_display_update_bars();
void lvgl_display_touch_update();
void lvgl_display_update_damper();
void lvgl_display_update_target_temp();
void lvgl_display_update_damper_status();
void lvgl_display_set_time(const char* time_str);
void show_time_on_display();
void show_time_reset_cache(); // Reset time cache (call when NTP time changes)

// PSRAM optimization functions
bool init_psram_buffers();
void cleanup_psram_buffers();

// ...existing code...
void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show);
void lvgl_display_show_settings(); // Jauna funkcija, kas atver iestatījumu logu
void lvgl_display_close_settings(); // Jauna funkcija, kas aizver iestatījumu logu
void lvgl_display_show_warning(const char* title, const char* message); // Jauna funkcija brīdinājuma popup rādīšanai
bool is_manual_damper_mode(); // Jauna funkcija, kas ļauj pārbaudīt vai ir manuālais režīms
// ...existing code...