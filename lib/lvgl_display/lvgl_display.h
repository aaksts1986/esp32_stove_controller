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