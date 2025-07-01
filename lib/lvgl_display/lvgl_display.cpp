#include "lvgl_display.h"

// LVGL un TFT mainīgie
#define CAL_X_MIN 210
#define CAL_X_MAX 3850
#define CAL_Y_MIN 250
#define CAL_Y_MAX 3900
#define TOUCH_THRESHOLD 100

#define ALPHA 0.3f  // 0 = stingrs filtrs, 1 = bez filtra

extern TFT_eSPI tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[TFT_WIDTH * (TFT_HEIGHT/10)];
static lv_color_t buf2[TFT_WIDTH * (TFT_HEIGHT/10)];

uint16_t last_x = 0, last_y = 0;
bool last_touched = false;

float smoothed_x = 0, smoothed_y = 0;

lv_obj_t *blue_bar;
lv_obj_t *red_bar;
lv_obj_t *raw_touch_point = NULL;  // Zaļais punkts (nefiltrētais)
static lv_obj_t *temp_label = NULL;
static lv_obj_t *damper_label = NULL;
static lv_obj_t *target_temp_label = NULL; // Jauns mērķa temperatūras teksts
extern int targetTempC;

// Funkcijas prototips
void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show);

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
    return (x >= CAL_X_MIN) && (x <= CAL_X_MAX) &&
           (y >= CAL_Y_MIN) && (y <= CAL_Y_MAX);
}

void mans_pieskariens_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouchRaw(&touchX, &touchY);

    data->point.x = 0;
    data->point.y = 0;
    data->state = LV_INDEV_STATE_REL;

    if (!touched) {
        last_touched = false;
        return;
    }

    last_touched = ir_derigs_pieskariens(touchX, touchY);

    if (!last_touched) return;

    last_x = constrain(map(touchX, CAL_X_MIN, CAL_X_MAX, 0, TFT_WIDTH-1 ), 0, TFT_WIDTH-1);
    last_y = constrain(map(touchY, CAL_Y_MIN, CAL_Y_MAX, TFT_HEIGHT-1, 0), 0, TFT_HEIGHT-1);

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PR;

    Serial.printf("Pieskāriens: RAW(%u,%u) -> MAP(%u,%u) -> LVGL(%u,%u)\n",
                 touchX, touchY, last_x, last_y, data->point.x, data->point.y);
}

void process_touch() {
    if (!last_touched) return;

    float dist = sqrt(pow(last_x - smoothed_x, 2) + pow(last_y - smoothed_y, 2));
    if (dist > 50) { // ja attālums liels, pārlēkt uzreiz
        smoothed_x = last_x;
        smoothed_y = last_y;
    } else {
        smoothed_x = ALPHA * last_x + (1.0f - ALPHA) * smoothed_x;
        smoothed_y = ALPHA * last_y + (1.0f - ALPHA) * smoothed_y;
    }
}

void lvgl_display_touch_update() {
    process_touch();

    if (last_touched) {
        lvgl_display_show_touch_point((uint16_t)smoothed_x, (uint16_t)smoothed_y, true);
    } else {
        lvgl_display_show_touch_point(0, 0, false);
    }
}

void lvgl_display_update_bars(int temp, int targetTempC) {
    lv_bar_set_range(blue_bar, 5, targetTempC);
    lv_bar_set_value(blue_bar, (temp > targetTempC / 2.5) ? (targetTempC / 2.43) : temp, LV_ANIM_OFF);

    if(temp > 30) {
        lv_bar_set_range(red_bar, targetTempC / 2.5, targetTempC + 3);
        lv_bar_set_value(red_bar, temp, LV_ANIM_OFF);
        lv_obj_clear_flag(red_bar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(red_bar, LV_OBJ_FLAG_HIDDEN);
    }

    // Atjauno temperatūras tekstu
    if(temp_label) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "Temp: %d°C", temp);
        lv_label_set_text(temp_label, buf);
    }
}

void lvgl_display_init() {
    tft.init();
    tft.setRotation(4);
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
    lv_obj_align(rect, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_radius(rect, 20, 0);
    lv_obj_set_style_bg_color(rect, lv_color_hex(0xfcfdfd), 0);
    lv_obj_set_style_border_color(rect, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(rect, 2, 0);

    // Melna vertikāla līnija pa vidu
    static lv_point_t line_points[] = { {150, 0}, {150, 400} }; // 150 ir puse no 300 (rect platums)
    lv_obj_t * line = lv_line_create(lv_scr_act());
    lv_line_set_points(line, line_points, 2);
    lv_obj_align(line, LV_ALIGN_TOP_MID, -85, 15);
    //lv_obj_set_pos(line, (tft.width() - 300)/2, tft.height() - 400 -30); // rect kreisais augšējais stūris
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(0x000000), 0);

    blue_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(blue_bar, 80, 300);
    lv_obj_set_pos(blue_bar, 40, 40);
    //lv_bar_set_range(blue_bar, 5, targetTempC);
    lv_obj_set_style_radius(blue_bar, 40, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(blue_bar, true, LV_PART_MAIN);
    lv_obj_set_style_bg_color(blue_bar, lv_color_hex(0x7DD0F2), LV_PART_INDICATOR);
    lv_obj_set_style_radius(blue_bar, 0, LV_PART_INDICATOR);

    red_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(red_bar, 80, 175);
    lv_obj_align(red_bar, LV_ALIGN_LEFT_MID, 40, -80);
    //lv_bar_set_range(red_bar, 30, targetTempC+3);
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
    lv_obj_set_pos(circle, 20, 480 - 200);
    lv_obj_set_style_radius(circle, 60, 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0x7DD0F2), 0);
    lv_obj_set_style_border_width(circle, 0, 0);

    // Zaļais punkts (nefiltrētais/raw)
    raw_touch_point = lv_obj_create(lv_scr_act());
    lv_obj_set_size(raw_touch_point, 20, 20);
    lv_obj_set_style_radius(raw_touch_point, 10, 0);
    lv_obj_set_style_bg_color(raw_touch_point, lv_color_hex(0x00FF00), 0); // Zaļš
    lv_obj_set_style_border_width(raw_touch_point, 0, 0);
    lv_obj_add_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);

    // Temperatūras label (augšā, piemēram, y = 30)
    temp_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_28, 0);
    lv_obj_align(temp_label, LV_ALIGN_TOP_RIGHT, -30, 30); // Novieto augšējā labajā stūrī
    lv_label_set_text(temp_label, "Temp: --°C");

    // Damper label (zem temperatūras, piemēram, y = 70)
    damper_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(damper_label, &lv_font_montserrat_28, 0);
    lv_obj_align(damper_label, LV_ALIGN_TOP_RIGHT, -30, 70);
    lv_label_set_text(damper_label, "Amortizators: --");

    // Jauns mērķa temperatūras teksts (zem damper label)
    target_temp_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(target_temp_label, &lv_font_montserrat_28, 0);
    lv_obj_align(target_temp_label, LV_ALIGN_TOP_RIGHT, -30, 110); // Novieto zem damper label
    lv_label_set_text(target_temp_label, "Target: --°C");
}

// Atstāj tikai šo funkciju, kas tagad strādā tikai ar zaļo punktu
void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show) {
    if (!raw_touch_point) return;
    if (show) {
        lv_obj_set_pos(raw_touch_point, x - 10, y - 10);
        lv_obj_clear_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);
    }
}

void lvgl_display_update_damper(int damper) {
    if(damper_label) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "Damper: %d", damper);
        lv_label_set_text(damper_label, buf);
    }
}

void lvgl_display_update_target_temp() {
    if(target_temp_label) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "Target: %d°C", targetTempC);
        lv_label_set_text(target_temp_label, buf);
    }
}