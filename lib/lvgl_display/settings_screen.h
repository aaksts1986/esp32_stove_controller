#pragma once

#include "lvgl.h"
#include "damper_control.h"

// Settings screen functions
void settings_screen_create(void);
void settings_screen_show(void);
void settings_screen_hide(void);
bool settings_screen_is_visible(void);

// Animation settings
#define SETTINGS_ANIM_TIME_MS 100  // Animācijas ilgumsb
#define SETTINGS_SLIDE_DISTANCE 320  // Cik pikseļi slide (displeja platums)

// Settings screen colors (Modern Minimalist style)
#define SETTINGS_BG_COLOR       lv_color_hex(0xF8F9FA)  // Gaišs fons
#define SETTINGS_ACCENT_COLOR   lv_color_hex(0x007BFF)  // Zils akcents
#define SETTINGS_TEXT_COLOR     lv_color_hex(0x212529)  // Tumšs teksts
#define SETTINGS_CARD_COLOR     lv_color_hex(0xFFFFFF)  // Balts kartīšu fons