#pragma once

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>

void lvgl_display_init();
void lvgl_display_update_bars();
void lvgl_display_touch_update();
void lvgl_display_update_damper();
void lvgl_display_update_target_temp();