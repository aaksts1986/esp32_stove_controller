#include "lvgl_display.h"

// LVGL un TFT mainīgie
#define CAL_X_MIN 200
#define CAL_X_MAX 3800
#define CAL_Y_MIN 200
#define CAL_Y_MAX 3800
#define TOUCH_THRESHOLD 100
#define FILTER_SAMPLES 5

extern TFT_eSPI tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[TFT_WIDTH * (TFT_HEIGHT/10)];
static lv_color_t buf2[TFT_WIDTH * (TFT_HEIGHT/10)];

uint16_t last_x = 0, last_y = 0;
bool last_touched = false;
uint16_t x_history[FILTER_SAMPLES] = {0};
uint16_t y_history[FILTER_SAMPLES] = {0};
uint8_t history_index = 0;

lv_obj_t *blue_bar;
lv_obj_t *red_bar;
lv_obj_t *touch_point = NULL;      // Sarkanais punkts (filtrētais)
lv_obj_t *raw_touch_point = NULL;  // Zaļais punkts (nefiltrētais)

// Function prototype for lvgl_display_show_touch_point
void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show);

// Function prototype for lvgl_display_show_touch_points
void lvgl_display_show_touch_points(uint16_t filtered_x, uint16_t filtered_y, bool show_filtered,
                                   uint16_t raw_x, uint16_t raw_y, bool show_raw);

// Displeja flush funkcija
void mans_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

bool ir_derigs_pieskariens(uint16_t x, uint16_t y) {
    Serial.printf("[ir_derigs_pieskariens] x=%u y=%u\n", x, y);
    bool derigs = !(y > (CAL_Y_MAX + TOUCH_THRESHOLD) || (x < TOUCH_THRESHOLD));
    Serial.printf("[ir_derigs_pieskariens] derigs=%d\n", derigs);
    return derigs;
}

void mans_pieskariens_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouchRaw(&touchX, &touchY);
    Serial.printf("[mans_pieskariens_read] getTouchRaw: touched=%d, X=%u, Y=%u\n", touched, touchX, touchY);

    last_touched = touched && ir_derigs_pieskariens(touchX, touchY);
    Serial.printf("[mans_pieskariens_read] last_touched=%d\n", last_touched);

    if (!last_touched) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    last_x = constrain(map(touchX, CAL_X_MIN, CAL_X_MAX, TFT_WIDTH, 0), 0, TFT_WIDTH - 1);
    last_y = constrain(map(touchY, CAL_Y_MIN, CAL_Y_MAX, TFT_HEIGHT, 0), 0, TFT_HEIGHT - 1);

    // Ja visi vēstures elementi ir 0, aizpildi visus ar pirmo vērtību
    static bool history_initialized = false;
    if (!history_initialized) {
        for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
            x_history[i] = last_x;
            y_history[i] = last_y;
        }
        history_initialized = true;
    }

    x_history[history_index] = last_x;
    y_history[history_index] = last_y;
    history_index = (history_index + 1) % FILTER_SAMPLES;

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PR;
}

uint16_t pielietot_filteri(uint16_t *history) {
    uint16_t min = history[0], max = history[0];
    uint32_t sum = history[0];

    for (uint8_t i = 1; i < FILTER_SAMPLES; i++) {
        if (history[i] < min) min = history[i];
        if (history[i] > max) max = history[i];
        sum += history[i];
    }
    uint16_t filtered = (sum - min - max) / (FILTER_SAMPLES - 2);
    Serial.printf("[pielietot_filteri] min=%u, max=%u, filtered=%u\n", min, max, filtered);
    return filtered;
}

void lvgl_display_touch_update() {
    static uint16_t prev_x = 0, prev_y = 0;

    // Neapstrādātās (raw) vērtības no pēdējā pieskāriena
    uint16_t raw_x = last_x;
    uint16_t raw_y = last_y;

    if (last_touched) {
        uint16_t filtered_x = pielietot_filteri(x_history);
        uint16_t filtered_y = pielietot_filteri(y_history);

        // Parādi abus punktus
        lvgl_display_show_touch_points(filtered_x, filtered_y, true, raw_x, raw_y, true);

        if (abs(filtered_x - prev_x) > 1 || abs(filtered_y - prev_y) > 1) {
            Serial.printf("Touch at: X: %d, Y: %d\n", filtered_y, filtered_x);
            prev_x = filtered_x;
            prev_y = filtered_y;
        }
    } else {
        // Paslēp abus punktus
        lvgl_display_show_touch_points(0, 0, false, 0, 0, false);
    }
}

void lvgl_display_update_bars(int temp) {
    lv_bar_set_value(blue_bar, (temp > 30) ? 30 : temp, LV_ANIM_OFF);

    if(temp > 30) {
        lv_obj_clear_flag(red_bar, LV_OBJ_FLAG_HIDDEN);
        lv_bar_set_value(red_bar, temp, LV_ANIM_OFF);
    } else {
        lv_obj_add_flag(red_bar, LV_OBJ_FLAG_HIDDEN);
    }
}

void lvgl_display_init() {
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(TFT_WHITE);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, TFT_WIDTH * (TFT_HEIGHT/10));

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = tft.width();
    disp_drv.ver_res = tft.height();
    disp_drv.flush_cb = mans_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mans_pieskariens_read;
    lv_indev_drv_register(&indev_drv);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xF3F4F3), 0);

    lv_obj_t * rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect, 300, 400);
    lv_obj_align(rect, LV_ALIGN_BOTTOM_MID, 0, -15);
    lv_obj_set_style_radius(rect, 20, 0);
    lv_obj_set_style_bg_color(rect, lv_color_hex(0xfcfdfd), 0);
    lv_obj_set_style_border_width(rect, 2, 0);

    blue_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(blue_bar, 80, 300);
    lv_obj_set_pos(blue_bar, 40, 100);
    lv_bar_set_range(blue_bar, 5, 80);
    lv_obj_set_style_radius(blue_bar, 40, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(blue_bar, true, LV_PART_MAIN);
    lv_obj_set_style_bg_color(blue_bar, lv_color_hex(0x7DD0F2), LV_PART_INDICATOR);
    lv_obj_set_style_radius(blue_bar, 0, LV_PART_INDICATOR);

    red_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(red_bar, 80, 175);
    lv_obj_align(red_bar, LV_ALIGN_LEFT_MID, 40, -20);
    lv_bar_set_range(red_bar, 30, 85);
    lv_obj_set_style_bg_opa(red_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(red_bar, lv_color_hex(0xFFC0C0), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(red_bar, lv_color_hex(0x7DD0F2), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(red_bar, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(red_bar, 5, LV_PART_INDICATOR);
    lv_obj_set_style_radius(red_bar, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(red_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(red_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_flag(red_bar, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * circle = lv_obj_create(lv_scr_act());
    lv_obj_set_size(circle, 120, 120);
    lv_obj_set_pos(circle, 20, 480 - 150);
    lv_obj_set_style_radius(circle, 60, 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0x7DD0F2), 0);
    lv_obj_set_style_border_width(circle, 0, 0);

    // Sarkanais punkts (filtrētais)
    touch_point = lv_obj_create(lv_scr_act());
    lv_obj_set_size(touch_point, 20, 20);
    lv_obj_set_style_radius(touch_point, 10, 0);
    lv_obj_set_style_bg_color(touch_point, lv_color_hex(0xFF0000), 0); // Sarkans
    lv_obj_set_style_border_width(touch_point, 0, 0);
    lv_obj_add_flag(touch_point, LV_OBJ_FLAG_HIDDEN);

    // Zaļais punkts (nefiltrētais/raw)
    raw_touch_point = lv_obj_create(lv_scr_act());
    lv_obj_set_size(raw_touch_point, 20, 20);
    lv_obj_set_style_radius(raw_touch_point, 10, 0);
    lv_obj_set_style_bg_color(raw_touch_point, lv_color_hex(0x00FF00), 0); // Zaļš
    lv_obj_set_style_border_width(raw_touch_point, 0, 0);
    lv_obj_add_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);
}

void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show) {
    if (!touch_point) return;
    if (show) {
        // Novieto punktu tā, lai centrs būtu pieskāriena vietā
        lv_obj_set_pos(touch_point, x - 10, y - 10);
        lv_obj_clear_flag(touch_point, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(touch_point, LV_OBJ_FLAG_HIDDEN);
    }
}

void lvgl_display_show_touch_points(uint16_t filtered_x, uint16_t filtered_y, bool show_filtered,
                                   uint16_t raw_x, uint16_t raw_y, bool show_raw) {
    if (touch_point) {
        if (show_filtered) {
            lv_obj_set_pos(touch_point, filtered_x - 10, filtered_y - 10);
            lv_obj_clear_flag(touch_point, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(touch_point, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (raw_touch_point) {
        if (show_raw) {
            lv_obj_set_pos(raw_touch_point, raw_x - 10, raw_y - 10);
            lv_obj_clear_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);
        }
    }
}