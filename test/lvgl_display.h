#pragma once

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>

void lvgl_display_init();
void lvgl_display_update_bars(int temp);
void lvgl_display_touch_update();