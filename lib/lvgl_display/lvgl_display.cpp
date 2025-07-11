#include "lvgl_display.h"
//#include <TFT_eSPI.h>  // Include TFT_eSPI here instead of in header
#include "temperature.h"
#include "damper_control.h"
#include "wifi1.h"
#include "lvgl.h"
#include "display_config.h"  // For buffer size configuration

#include <LovyanGFX.hpp>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#define TOUCH_CS 9
#define TOUCH_IRQ  7
#define TOUCH_SCLK 12
#define TOUCH_MOSI 11
#define TOUCH_MISO 13

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);




#define WIDTH 320
#define HEIGHT 480

// Define custom LGFX class for your display configuration
class LGFX : public lgfx::LGFX_Device {
private:
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;
  

public:
  LGFX(void) {
    // Bus configuration
    auto bus_cfg = _bus_instance.config();
    bus_cfg.spi_host = SPI2_HOST;
    bus_cfg.spi_mode = 0;
    bus_cfg.freq_write = 40000000;  // Palielināts ātrums
    bus_cfg.freq_read = 16000000;
    bus_cfg.pin_sclk = 12;
    bus_cfg.pin_mosi = 11;
    bus_cfg.pin_miso = 13;
    bus_cfg.pin_dc = 18;
    bus_cfg.dma_channel = 1;  // Ieslēgts DMA režīms
    _bus_instance.config(bus_cfg);
    _panel_instance.setBus(&_bus_instance);

    // Panel configuration
    auto panel_cfg = _panel_instance.config();
    panel_cfg.pin_cs = 10;
    panel_cfg.pin_rst = 40;
    panel_cfg.panel_width = 320;
    panel_cfg.panel_height = 480;
    
    _panel_instance.config(panel_cfg);

    // Backlight configuration
    auto light_cfg = _light_instance.config();
    light_cfg.pin_bl = 8;
    light_cfg.invert = false;
    _light_instance.config(light_cfg);
    _panel_instance.setLight(&_light_instance);


    setPanel(&_panel_instance);
  }
};

// Create display instance
LGFX display;

// LVGL un TFT mainīgie - OPTIMIZED BUFFER SIZE
#define CAL_X_MIN 210
#define CAL_X_MAX 3850
#define CAL_Y_MIN 250
#define CAL_Y_MAX 3900
#define TOUCH_THRESHOLD 100



// Buffer optimization: Use configurable buffer size based on display mode and PSRAM
// PSRAM available: Use 3x larger buffers for better performance 
// PURE_EVENT = 1/5 (3x larger), POWER_SAVE = 1/7, others = 1/3 when PSRAM available
// Without PSRAM: Standard sizes (1/15, 1/20, 1/10)

#ifndef PSRAM_LVGL_BUFFER_FRACTION
    #define PSRAM_LVGL_BUFFER_FRACTION 3  // Default fallback
#endif


static lv_disp_draw_buf_t draw_buf;

// Conditional buffer allocation: PSRAM vs internal RAM
#ifdef BOARD_HAS_PSRAM
    // Use PSRAM for large buffers when available
    DRAM_ATTR static lv_color_t *buf1 = nullptr;
    DRAM_ATTR static lv_color_t *buf2 = nullptr;
    #define BUFFER_SIZE (WIDTH * (HEIGHT/PSRAM_LVGL_BUFFER_FRACTION))
#else
    // Use internal RAM for smaller buffers
    static lv_color_t buf1[TFT_WIDTH * (TFT_HEIGHT/PSRAM_LVGL_BUFFER_FRACTION)];
    static lv_color_t buf2[TFT_WIDTH * (TFT_HEIGHT/PSRAM_LVGL_BUFFER_FRACTION)];
    #define BUFFER_SIZE (TFT_WIDTH * (TFT_HEIGHT/PSRAM_LVGL_BUFFER_FRACTION))
#endif

uint16_t last_x = 0, last_y = 0;
bool last_touched = false;
static uint32_t last_touch_time = 0;  // Pievienojiet šo rindu

lv_obj_t *blue_bar;
lv_obj_t *red_bar;
lv_obj_t *raw_touch_point = NULL;  // Zaļais punkts (nefiltrētais)
static lv_obj_t *temp_label = NULL;
static lv_obj_t *temp0 = NULL;
static lv_obj_t *damper_label = NULL;
static lv_obj_t *target_temp_label = NULL; // Jauns mērķa temperatūras tekstsv
static lv_obj_t *target = NULL;
static lv_obj_t *damper_status_label = NULL; // Jauns label mainīgais
static lv_obj_t *time_label = NULL;

extern int targetTempC;

// Funkcijas prototips
void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show);
LV_FONT_DECLARE(ekstra);
LV_FONT_DECLARE(ekstra1);
LV_FONT_DECLARE(eeet);

// Displeja flush funkcija
void mans_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);



    display.startWrite();
    display.setAddrWindow(area->x1, area->y1, w, h);
    display.writePixels((uint16_t*)color_p, w * h);
    display.endWrite();

    lv_disp_flush_ready(disp);
}

bool ir_derigs_pieskariens(uint16_t x, uint16_t y) {
    return (x >= CAL_X_MIN) && (x <= CAL_X_MAX) &&
           (y >= CAL_Y_MIN) && (y <= CAL_Y_MAX);
}


void mans_pieskariens_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchX = 0, touchY = 0;
    bool touched = false;

    // Izmanto XPT2046_Touchscreen bibliotēku
    if (ts.tirqTouched()) {
        TS_Point p = ts.getPoint();
        touchX = p.x;
        touchY = p.y;
        touched = true;
        Serial.printf("XPT2046: touched=1, x=%u, y=%u, z=%u\n", p.x, p.y, p.z);
    } else {
        Serial.println("XPT2046: touched=0");
    }

    data->point.x = 0;
    data->point.y = 0;
    data->state = LV_INDEV_STATE_REL;

    if (!touched) {
        last_touched = false;
        return;
    }

    last_touched = ir_derigs_pieskariens(touchX, touchY);

    if (!last_touched) return;

    last_x = constrain(map(touchX, CAL_X_MIN, CAL_X_MAX, WIDTH-1, 0 ), 0, WIDTH-1);
    last_y = constrain(map(touchY, CAL_Y_MIN, CAL_Y_MAX, HEIGHT-1, 0), 0, HEIGHT-1);

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PR;

    Serial.printf("Pieskāriens: RAW(%u,%u) -> MAP(%u,%u) -> LVGL(%u,%u)\n",
                 touchX, touchY, last_x, last_y, data->point.x, data->point.y);
}


void process_touch() {
    if (!last_touched) {
        return;
    }

    Serial.println("Atvēršu iestatījumu logu!");
    lvgl_display_show_settings(); // Izsauc uzreiz
    last_touch_time = millis();
}

void lvgl_display_touch_update() {
    process_touch();

    if (last_touched) {
        lvgl_display_show_touch_point((uint16_t)last_x, (uint16_t)last_y, true);
    } else {
        lvgl_display_show_touch_point(0, 0, false);
    }
}

void lvgl_display_update_bars() {

    lv_bar_set_range(blue_bar, 0, targetTempC);
    lv_bar_set_value(blue_bar, (temperature > targetTempC / 3) ? (targetTempC / 3) : temperature, LV_ANIM_OFF);

    if(temperature > targetTempC / 3) {
        lv_bar_set_range(red_bar, targetTempC / 3, targetTempC + 3);
        lv_bar_set_value(red_bar, temperature, LV_ANIM_OFF);
        lv_obj_clear_flag(red_bar, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(red_bar, LV_OBJ_FLAG_HIDDEN);
    }

    // Atjauno temperatūras tekstu
    if(temp_label) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "%d", temperature);
        lv_label_set_text(temp_label, buf);
    }
}



void lvgl_display_init() {
          ts.begin();
  ts.setRotation(0);
    //tft.init();
    //tft.setRotation(4);
    //tft.fillScreen(TFT_WHITE);
    //tft.setSwapBytes(true); // Swap bytes for RGB565 format
        display.init();
    display.setBrightness(128);
    
    display.setRotation(0);
    

    lv_init();
    
    // Initialize PSRAM buffers if available, otherwise use static buffers
    bool psram_success = init_psram_buffers();
    
#ifdef BOARD_HAS_PSRAM
    if (psram_success && buf1 && buf2) {
        // Use PSRAM buffers (larger size)
        lv_disp_draw_buf_init(&draw_buf, buf1, buf2, BUFFER_SIZE);
    } else {
        // Fallback to static internal RAM buffers
        static lv_color_t fallback_buf1[WIDTH * (HEIGHT/20)]; // Smaller for safety
        static lv_color_t fallback_buf2[WIDTH * (HEIGHT/20)];
        lv_disp_draw_buf_init(&draw_buf, fallback_buf1, fallback_buf2, WIDTH * (HEIGHT/20));
    }
#else
    // No PSRAM support - use static buffers
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, BUFFER_SIZE);
#endif

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = display.width();
    disp_drv.ver_res = display.height();
    disp_drv.flush_cb = mans_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mans_pieskariens_read;
    lv_indev_drv_register(&indev_drv);

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xF3F4F3), 0);


    // Laika label (piemēram, augšējā labajā stūrī)
    time_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x7997a3), 0);
    lv_obj_set_pos(time_label, 190, 25); // Novieto pēc vajadzības
    lv_label_set_text(time_label, LV_SYMBOL_WIFI " --:--");

    //lv_bar_set_range(blue_bar, 5, targetTempC);
    blue_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(blue_bar, 110, 350);
    lv_obj_set_pos(blue_bar, 40, 40);
    lv_obj_set_style_radius(blue_bar, 55, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(blue_bar, true, LV_PART_MAIN);
    lv_obj_set_style_bg_color(blue_bar, lv_color_hex(0x7DD0F2), LV_PART_INDICATOR);
    lv_obj_set_style_radius(blue_bar, 0, LV_PART_INDICATOR);

    //lv_bar_set_range(red_bar, 30, targetTempC+3);
    red_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(red_bar, 110, 200);
    lv_obj_align(red_bar, LV_ALIGN_LEFT_MID, 40, -50);
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
    lv_obj_set_size(circle, 150, 150);
    lv_obj_set_pos(circle, 20, 520 - 200);
    lv_obj_set_style_radius(circle, 75, 0);
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
    lv_obj_set_style_text_font(temp_label, &ekstra, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xF3F4F3), 0);
    lv_obj_set_pos(temp_label, 45, 360); // Novieto augšējā labajā stūrī
    lv_label_set_text(temp_label, "--");

     temp0 = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(temp0, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(temp0, lv_color_hex(0xF3F4F3), 0);
    lv_obj_set_style_bg_color(temp0, lv_color_hex(0xF3F4F3), 0);
    lv_obj_set_pos(temp0, 140, 360);
    lv_label_set_text(temp0, "°C"); 

     target = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(target, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(target, lv_color_hex(0x7997a3), 0);
    lv_obj_set_style_bg_color(target, lv_color_hex(0x7997a3), 0);
    lv_obj_set_pos(target, 170, 110);
    lv_label_set_text(target, "MAX temp."); 


     // Damper label (vērtība)
    damper_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(damper_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(damper_label, lv_color_hex(0x7997a3), 0);
    lv_obj_set_style_bg_color(damper_label, lv_color_hex(0x7997a3), 0);
    lv_obj_set_pos(damper_label, 200, 290);
    lv_label_set_text(damper_label, "-- %");

    lv_obj_t *label_one = lv_label_create(lv_scr_act());
    lv_label_set_text(label_one, "2");
    lv_obj_set_style_text_font(label_one, &eeet, 0);
    lv_obj_set_style_text_color(label_one, lv_color_hex(0x7997a3), 0);
    lv_obj_set_style_bg_color(label_one, lv_color_hex(0x7997a3), 0);
    lv_obj_set_pos(label_one, 240,390);

    // Damper status label (AUTO/FILL)
    damper_status_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(damper_status_label, &ekstra1, 0);
    lv_obj_set_style_text_color(damper_status_label, lv_color_hex(0x7997a3), 0);
    lv_obj_set_style_bg_color(damper_status_label, lv_color_hex(0x7997a3), 0);
    lv_obj_set_pos(damper_status_label, 180, 230); // Novieto zem damper label
    lv_label_set_text(damper_status_label, "--");


     // Jauns mērķa temperatūras teksts (zem damper label)
    target_temp_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(target_temp_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(target_temp_label, lv_color_hex(0x7997a3), 0);
    lv_obj_set_style_bg_color(target_temp_label, lv_color_hex(0x7997a3), 0);
    lv_obj_set_pos(target_temp_label, 210, 140); // Novieto zem damper label
    lv_label_set_text(target_temp_label, "--°C");



    // V-veida lauzta raustīta līnija ar 135° leņķi, katra mala 110px
    static lv_point_t v_line_points[] = {
        {0, 0},
        {110, 0},
        {
            110 + (int)(80 * cosf(M_PI - M_PI * 115.0f / 180.0f)),
            (int)(80 * sinf(M_PI - M_PI * 115.0f / 180.0f))
        },
        {
            110 + (int)(80 * cosf(M_PI - M_PI * 115.0f / 180.0f)) + 100,
            (int)(80 * sinf(M_PI - M_PI * 115.0f / 180.0f))
        }
    };
    // Tagad ir 4 punkti: horizontāla, lauzta, horizontāla

    lv_obj_t *v_dash_line = lv_line_create(lv_scr_act());
    lv_line_set_points(v_dash_line, v_line_points, 4);
    lv_obj_set_pos(v_dash_line, 40, 100); // Novieto pēc vajadzības

    static lv_style_t style_v_dash;
    lv_style_init(&style_v_dash);
    lv_style_set_line_width(&style_v_dash, 2);
    lv_style_set_line_color(&style_v_dash, lv_color_hex(0x7997a3));
    lv_style_set_line_dash_gap(&style_v_dash, 7);
    lv_style_set_line_dash_width(&style_v_dash, 7);

    lv_obj_add_style(v_dash_line, &style_v_dash, LV_PART_MAIN);
}

// Pievienojiet šos mainīgos pie citiem globālajiem mainīgajiem
static lv_obj_t *settings_panel = NULL;
static bool settings_open = false;

// Funkcija, kas parāda iestatījumu logu
void lvgl_display_show_settings() {
    Serial.println("lvgl_display_show_settings() izsaukta.");
    if (settings_open || settings_panel != NULL) {
        Serial.println("Iestatījumu logs jau atvērts.");
        return;  // Logs jau ir atvērts
    }

    // Izveidojam iestatījumu paneli
    settings_panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(settings_panel, 280, 200);
    lv_obj_align(settings_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(settings_panel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(settings_panel, 15, 0);
    lv_obj_set_style_border_width(settings_panel, 2, 0);
    lv_obj_set_style_border_color(settings_panel, lv_color_hex(0x7997a3), 0);
    lv_obj_set_style_shadow_width(settings_panel, 20, 0);
    lv_obj_set_style_shadow_color(settings_panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(settings_panel, LV_OPA_30, 0);

    // Pievienojam virsrakstu
    lv_obj_t *title = lv_label_create(settings_panel);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_label_set_text(title, "SETTINGS");

    // Pievienojam aizvēršanas pogu
    lv_obj_t *close_btn = lv_btn_create(settings_panel);
    lv_obj_set_size(close_btn, 100, 40);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x7997a3), 0);
    lv_obj_add_event_cb(close_btn, [](lv_event_t *e) {
        lvgl_display_close_settings();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Close");
    lv_obj_center(close_label);

    settings_open = true;
}

// Funkcija, kas aizver iestatījumu logu
void lvgl_display_close_settings() {
    Serial.println("lvgl_display_close_settings() izsaukta.");
    if (settings_panel != NULL) {
        lv_obj_del(settings_panel);
        settings_panel = NULL;
        settings_open = false;
        Serial.println("Iestatījumu logs aizvērts.");
    }
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

// Atjauno tikai damper vērtību
void lvgl_display_update_damper() {
    if(damper_label) {
        static char buf[16];
        snprintf(buf, sizeof(buf), "%d %%", damper);
        lv_label_set_text(damper_label, buf);
    }
}

// Atjauno damper statusu (AUTO/FILL)
void lvgl_display_update_damper_status() {
    if(damper_status_label) {
        lv_label_set_text(damper_status_label, messageDamp.c_str());
    }
}

void lvgl_display_update_target_temp() {
    if(target_temp_label) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "%d°C", targetTempC);
        lv_label_set_text(target_temp_label, buf);
    }
}

void lvgl_display_set_time(const char* time_str) {
    if (time_label) {
        static char buf[32];
        snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " %s", time_str);
        lv_label_set_text(time_label, buf);
    }
}

void show_time_on_display() {
    // Use char array instead of String to save RAM
    static char last_time_displayed[8] = "";  // HH:MM + null terminator = 6 chars max
    static uint32_t last_check = 0;
    
    // Check time only every 10 seconds to save CPU (instead of every second)
    // The actual minute change detection will ensure display updates when needed
    if (millis() - last_check < 10000) {
        return;
    }
    last_check = millis();
    
    // Get current time string (HH:MM format) - returns String, but we convert to char[]
    String current_time_str = get_time_str();
    
    // Convert String to char array for comparison (save RAM)
    char current_time[8];
    strncpy(current_time, current_time_str.c_str(), sizeof(current_time)-1);
    current_time[sizeof(current_time)-1] = '\0';
    
    // Update display only if time string has actually changed (minute changed)
    if (strcmp(current_time, last_time_displayed) != 0 && strcmp(current_time, "--:--") != 0) {
        strcpy(last_time_displayed, current_time);
        lvgl_display_set_time(current_time);
    }
}

void show_time_reset_cache() {
    // Call this when NTP time changes significantly to reset the time cache
    // This ensures the display will be updated immediately on the next check
    
    // Force an immediate update without using String objects
    String current_time = get_time_str();
    if (current_time != "--:--") {
        lvgl_display_set_time(current_time.c_str());
    }
}

// PSRAM initialization and buffer allocation
bool init_psram_buffers() {
#ifdef BOARD_HAS_PSRAM
    if (!psramFound()) {
        return false;
    }
    
    size_t psram_free = ESP.getFreePsram();
    size_t required_size = BUFFER_SIZE * sizeof(lv_color_t) * 2; // 2 buffers
    
    if (psram_free < required_size + 10000) { // Keep 10KB PSRAM free
        return false;
    }
    
    // Allocate buffers in PSRAM
    buf1 = (lv_color_t*)ps_malloc(BUFFER_SIZE * sizeof(lv_color_t));
    buf2 = (lv_color_t*)ps_malloc(BUFFER_SIZE * sizeof(lv_color_t));
    
    if (!buf1 || !buf2) {
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        buf1 = buf2 = nullptr;
        return false;
    }
    
    return true;
#else
    return false;
#endif
}

void cleanup_psram_buffers() {
#ifdef BOARD_HAS_PSRAM
    if (buf1) {
        free(buf1);
        buf1 = nullptr;
    }
    if (buf2) {
        free(buf2);
        buf2 = nullptr;
    }
#endif
}

