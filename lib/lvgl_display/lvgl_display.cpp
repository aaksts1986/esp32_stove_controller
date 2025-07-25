#include "lvgl_display.h"
//#include <TFT_eSPI.h>  // Include TFT_eSPI here instead of in header
#include "temperature.h"
#include "damper_control.h"
#include "wifi1.h"
#include "lvgl.h"
#include "display_config.h"  // For buffer size configuration
#include "settings_screen.h"
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

class LGFX : public lgfx::LGFX_Device {
private:
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;
public:
  LGFX(void) {
    auto bus_cfg = _bus_instance.config();
    bus_cfg.spi_host = SPI2_HOST;
    bus_cfg.spi_mode = 0;
    bus_cfg.freq_write = 40000000;
    bus_cfg.freq_read = 16000000;
    bus_cfg.pin_sclk = 12;
    bus_cfg.pin_mosi = 11;
    bus_cfg.pin_miso = 13;
    bus_cfg.pin_dc = 18;
    bus_cfg.dma_channel = 1;
    _bus_instance.config(bus_cfg);
    _panel_instance.setBus(&_bus_instance);

    auto panel_cfg = _panel_instance.config();
    panel_cfg.pin_cs = 10;
    panel_cfg.pin_rst = 40;
    panel_cfg.panel_width = 320;
    panel_cfg.panel_height = 480;
    _panel_instance.config(panel_cfg);

    auto light_cfg = _light_instance.config();
    light_cfg.pin_bl = 8;
    light_cfg.invert = false;
    _light_instance.config(light_cfg);
    _panel_instance.setLight(&_light_instance);

    setPanel(&_panel_instance);
  }
};

LGFX display;

#define CAL_X_MIN 210
#define CAL_X_MAX 3850
#define CAL_Y_MIN 250
#define CAL_Y_MAX 3900
#define TOUCH_THRESHOLD 100

#ifndef PSRAM_LVGL_BUFFER_FRACTION
    #define PSRAM_LVGL_BUFFER_FRACTION 3
#endif

static lv_disp_draw_buf_t draw_buf;

#ifdef BOARD_HAS_PSRAM
    DRAM_ATTR static lv_color_t *buf1 = nullptr;
    DRAM_ATTR static lv_color_t *buf2 = nullptr;
    #define BUFFER_SIZE (WIDTH * (HEIGHT/PSRAM_LVGL_BUFFER_FRACTION))
#else
    static lv_color_t buf1[WIDTH * (HEIGHT/PSRAM_LVGL_BUFFER_FRACTION)];
    static lv_color_t buf2[WIDTH * (HEIGHT/PSRAM_LVGL_BUFFER_FRACTION)];
    #define BUFFER_SIZE (WIDTH * (HEIGHT/PSRAM_LVGL_BUFFER_FRACTION))
#endif

uint16_t last_x = 0, last_y = 0;
bool last_touched = false;
static uint32_t last_touch_time = 0;

lv_obj_t *blue_bar;
lv_obj_t *raw_touch_point = NULL;
static lv_obj_t *temp_label = NULL;
static lv_obj_t *temp0 = NULL;
static lv_obj_t *damper_label = NULL;
static lv_obj_t *target_temp_label = NULL;  // Vairs netiek izmantots
static lv_obj_t *target = NULL;
static lv_obj_t *damper_status_label = NULL;
static lv_obj_t *time_label = NULL;

// JAUNS: Main screen roller globālais objekts
static lv_obj_t *main_target_temp_roller = NULL;
// JAUNS: Damper roller globālais objekts
static lv_obj_t *damper_roller = NULL;

// JAUNS: Režīma pārslēgšanas mainīgie
static bool manual_mode = false;
static int saved_damper = 0; // Lai saglabātu vērtību pirms manuālā režīma

extern int targetTempC;
extern int damper;
extern String messageDamp;

void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show);
LV_FONT_DECLARE(ekstra);
LV_FONT_DECLARE(ekstra1);
LV_FONT_DECLARE(eeet);

// JAUNS: Temperature mapping arrays (main screen)
static const int main_temp_values[] = {64, 66, 68, 70, 72, 74, 76, 78, 80};
static const int main_temp_count = 9;

// JAUNS: Damper mapping arrays
static const int damper_values[] = {100, 80, 60, 40, 20, 0};
static const int damper_count = 6;

// JAUNS: Atrod damper indeksu array
static int get_damper_index(int damper_value)
{
    for (int i = 0; i < damper_count; i++) {
        if (damper_values[i] == damper_value) {
            return i;
        }
    }
    
    // Ja nav atrasts, atgriez tuvāko
    if (damper_value >= 100) return 0;
    if (damper_value <= 0) return damper_count - 1;
    
    // Atrod tuvāko
    for (int i = 0; i < damper_count - 1; i++) {
        if (damper_value <= damper_values[i] && damper_value > damper_values[i + 1]) {
            return i;
        }
    }
    return 0; // Default 100%
}

// JAUNS: Damper roller event handler - optimizēts
static void damper_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != damper_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu (bez Serial.println, lai neaizkavētu)
    if (selected >= damper_count) {
        return;
    }
    
    // Uzstāda jauno vērtību
    damper = damper_values[selected];
    
    // Atjauno damper vērtību displejā - tieši
    static char buf[16];
    snprintf(buf, sizeof(buf), "%d %%", damper);
    lv_label_set_text(damper_label, buf);
}

// JAUNS: Pārslēgšanās starp auto un manuālo režīmu
static void toggle_damper_mode(void)
{
    // Pārslēgšanās starp auto un manuālo režīmu - prioritizēta pareizi
    manual_mode = !manual_mode;
    
    if (manual_mode) {
        saved_damper = damper;  // Saglabājam pašreizējo vērtību
        
        // Uzstādam tekstu uz "MANUAL" - prioritārā darbība
        lv_label_set_text(damper_status_label, "MANUAL");
        
        // Teksta maiņa nekavējas, tāpēc mainām tieši
        lv_label_set_text(target, "Set Damper");
        
        // Paslēpjam temperatūras rolleri un parādam damper rolleri
        lv_obj_add_flag(main_target_temp_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(damper_roller, LV_OBJ_FLAG_HIDDEN);
        
        // Iestatām damper roller pozīciju uzreiz
        int damper_index = get_damper_index(damper);
        lv_roller_set_selected(damper_roller, damper_index, LV_ANIM_OFF);
    } else {
        // SVARĪGI: Šeit ir problēma ar damper_status_label, tāpēc mēs atvienojam šo un darām to atsevišķi
        // NEiestata tekstu uzreiz
        // lv_label_set_text(damper_status_label, messageDamp.c_str());
        
        // Teksta maiņa nekavējas, tāpēc mainām tieši
        lv_label_set_text(target, "MAX temp.");
        
        // Paslēpjam damper rolleri un parādam temperatūras rolleri
        lv_obj_add_flag(damper_roller, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(main_target_temp_roller, LV_OBJ_FLAG_HIDDEN);
        
        // Atjaunojam iepriekšējo damper vērtību
        damper = saved_damper;
        
        // SVARĪGI: Atjaunojam damper vērtību un statusu ATSEVIŠĶI
        static char buf[16];
        snprintf(buf, sizeof(buf), "%d %%", damper);
        lv_label_set_text(damper_label, buf);
        
        // SVARĪGI: Tagad atjaunojam status ar nelielu aizturi
        // Šeit bija kļūda - mēs izmantojām messageDamp, kura varēja būt novecojusi
        // Tā vietā, iekodējam fiksētu vērtību "AUTO", jo tikko esam pārslēguši uz AUTO režīmu
        lv_label_set_text(damper_status_label, "AUTO");
        
        // Forsa redraw
        lv_obj_invalidate(damper_status_label);
    }
    
    // Forsa atjaunināšanu
    lv_refr_now(NULL);
}

// JAUNS: Atrod temperature indeksu array (main screen)
static int main_get_temp_index(int temperature)
{
    for (int i = 0; i < main_temp_count; i++) {
        if (main_temp_values[i] == temperature) {
            return i;
        }
    }
    // Ja nav atrasts, atgriez tuvāko
    if (temperature < 64) return 0;
    if (temperature > 80) return 8;
    
    // Atrod tuvāko
    for (int i = 0; i < main_temp_count - 1; i++) {
        if (temperature >= main_temp_values[i] && temperature < main_temp_values[i + 1]) {
            return i;
        }
    }
    return 4; // Default 70°C
}

// JAUNS: Main screen roller event handler - optimizēts
static void main_temp_roller_event_handler(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
        return;
    }
    
    lv_obj_t * roller = lv_event_get_target(e);
    if (roller != main_target_temp_roller) {
        return;
    }
    
    uint16_t selected = lv_roller_get_selected(roller);
    
    // Validē indeksu (bez Serial.println, lai neaizkavētu)
    if (selected >= main_temp_count) {
        return;
    }
    
    // Tieši uzstāda jauno vērtību
    targetTempC = main_temp_values[selected];
}

// JAUNS: Update main roller function
void lvgl_display_update_main_roller() {
    if (!main_target_temp_roller) {
        return;
    }
    
    int current_index = main_get_temp_index(targetTempC);
    
    // Temporarily remove event callback to prevent triggering
    lv_obj_remove_event_cb(main_target_temp_roller, main_temp_roller_event_handler);
    
    // Update roller
    lv_roller_set_selected(main_target_temp_roller, current_index, LV_ANIM_OFF);
    
    // Re-add event callback
    lv_obj_add_event_cb(main_target_temp_roller, main_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
}

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
    if (ts.tirqTouched()) {
        TS_Point p = ts.getPoint();
        touchX = p.x;
        touchY = p.y;
        touched = true;
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
}

static bool settings_open = false;

void lvgl_display_show_settings() {
    if (settings_open) return;
    settings_open = true;
    settings_screen_show();
}

void lvgl_display_close_settings() {
    if (!settings_open) return;
    settings_open = false;
    settings_screen_hide();
}

// IZMAINĪTS: process_touch vairs neatver settings
void process_touch() {
    // Process_touch tagad nedara neko
    // Settings atveras TIKAI ar label_one click event
    return;
}

// JAUNS: Brīdinājuma popup mainīgie
static lv_obj_t *warning_popup = NULL;
static bool warning_popup_visible = false;
static bool buzzer_muted = false; // Mainīgais, lai sekotu klusuma režīmam

// Šī funkcija vairs nav nepieciešama, jo OK pogas nav un 
// brīdinājums aizveras automātiski, kad temperatūra normalizējas
// Tomēr atstājam to (bet neizmantojam), lai saglabātu koda struktūru
static void warning_popup_close_event_handler(lv_event_t * e) {
    // Tukša funkcija - vairs netiek izmantota
}

// JAUNS: Funkcija brīdinājuma popup parādīšanai (bez OK pogas, automātiski aizveras)
void lvgl_display_show_warning(const char* title, const char* message) {
    // Atskaņojam brīdinājuma signālu, ja nav ieslēgts klusuma režīms
    if (!buzzer_muted) {
        playWarningSound();
    }
    
    // Ja jau ir atvērts popup, nav nepieciešams to dzēst
    if (warning_popup) {
        // Jau ir aktīvs brīdinājums - atjaunojam tikai tekstu, ja nepieciešams
        lv_obj_t * message_label = lv_obj_get_child(warning_popup, 1); // Otrais bērns ir ziņojuma etiķete
        if (message_label) {
            lv_label_set_text(message_label, message);
        }
        
        // Ja nav ieslēgts klusums, atskaņojam brīdinājuma signālu, jo tas var būt cits brīdinājums
        if (!buzzer_muted) {
            playWarningSound();
        }
        
        return; // Izejam, jo brīdinājums jau tiek rādīts
    }
    
    // Izveidojam jaunu popup
    warning_popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(warning_popup, 300, 180); // Mazliet mazāks augstums, jo nav pogas
    lv_obj_center(warning_popup);
    lv_obj_set_style_bg_color(warning_popup, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_radius(warning_popup, 15, 0);
    lv_obj_set_style_shadow_width(warning_popup, 20, 0);
    lv_obj_set_style_shadow_color(warning_popup, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(warning_popup, LV_OPA_30, 0);
    
    // Virsraksts ar lielāku brīdinājuma ikonu
    lv_obj_t * title_label = lv_label_create(warning_popup);
    lv_label_set_text_fmt(title_label, LV_SYMBOL_WARNING " %s", title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0); // Palielinām fontu
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFF0000), 0); // Spilgti sarkans
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 15);
    
    // Brīdinājuma ziņojums - lielāks un treknāks
    lv_obj_t * message_label = lv_label_create(warning_popup);
    lv_label_set_text(message_label, message);
    lv_obj_set_style_text_font(message_label, &lv_font_montserrat_22, 0); // Palielinām fontu
    lv_obj_set_style_text_color(message_label, lv_color_hex(0x000000), 0);
    // Iestatām maksimālo platumu, lai teksts automātiski ietītos
    lv_obj_set_width(message_label, 260);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    // Centrējam ziņojumu
    lv_obj_align(message_label, LV_ALIGN_CENTER, 0, 10);
    
    // Pievienojam automātiskas izslēgšanās norādi
    lv_obj_t * auto_close_label = lv_label_create(warning_popup);
    lv_label_set_text(auto_close_label, "Pazudīs automātiski kad temp. normalizēsies");
    lv_obj_set_style_text_font(auto_close_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(auto_close_label, lv_color_hex(0x888888), 0);
    lv_obj_align(auto_close_label, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Pievienojam klusuma (mute) pogu
    lv_obj_t * mute_btn = lv_btn_create(warning_popup);
    lv_obj_set_size(mute_btn, 40, 40);
    lv_obj_align(mute_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_radius(mute_btn, 20, 0); // Apaļa poga
    
    // Mute pogas ikona (simbols)
    lv_obj_t * mute_label = lv_label_create(mute_btn);
    lv_label_set_text(mute_label, buzzer_muted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
    lv_obj_center(mute_label);
    
    // Pievienojam event handleri
    lv_obj_add_event_cb(mute_btn, [](lv_event_t * e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            buzzer_muted = !buzzer_muted;
            
            // Atjaunojam pogas ikonu
            lv_obj_t * btn = lv_event_get_target(e);
            lv_obj_t * label = lv_obj_get_child(btn, 0);
            if (label) {
                lv_label_set_text(label, buzzer_muted ? LV_SYMBOL_MUTE : LV_SYMBOL_VOLUME_MAX);
            }
            
            // Izslēdzam vai ieslēdzam skaņu
            if (buzzer_muted) {
                stopBuzzerSound();
            } else {
                playWarningSound();
            }
        }
    }, LV_EVENT_CLICKED, NULL);
    
    warning_popup_visible = true;
}

void lvgl_display_touch_update() {
    static bool prev_touched = false;
    
    process_touch();
    
    // Pārbaudām vai pieskāriena stāvoklis ir mainījies
    if (last_touched != prev_touched) {
        prev_touched = last_touched;
    }
    
    if (last_touched) {
        lvgl_display_show_touch_point((uint16_t)last_x, (uint16_t)last_y, true);
    } else {
        lvgl_display_show_touch_point(0, 0, false);
    }
}

void lvgl_display_update_bars() {
   // JAUNS: Pārbaudam vai temperatura ir kritiska
   static bool warning_shown = false;
   static bool high_temp_warning = false; // Vai brīdinājums ir par augstu temperatūru
   static bool low_temp_warning = false;  // Vai brīdinājums ir par zemu temperatūru
   
   if (temperature > warningTemperature && !warning_shown) {
       // Ja temperatūra ir par augstu un brīdinājums vēl nav parādīts
       static char warning_message[40];
       snprintf(warning_message, sizeof(warning_message), "Temp. parsniedz %d!", warningTemperature);
       lvgl_display_show_warning("Bridinajums!", warning_message);
       warning_shown = true;
       high_temp_warning = true;
       low_temp_warning = false;
   } else if (temperature <= 3 && !warning_shown) {
       // Sensora kļūda un brīdinājums vēl nav parādīts
       lvgl_display_show_warning("Bridinajums!", "Sensor error!");
       warning_shown = true;
       high_temp_warning = false;
       low_temp_warning = true;
   } else if ((high_temp_warning && temperature <= warningTemperature) || 
              (low_temp_warning && temperature > 3)) {
       // Temperatūra ir normalizējusies - notīram brīdinājumu
       // - Ja bija augstās temperatūras brīdinājums un temperatura nokrita zem sliekšņa
       // - VAI ja bija zemas temperatūras brīdinājums un temperatura paaugstinājās virs 3°C
       warning_shown = false;
       high_temp_warning = false;
       low_temp_warning = false;
       
       // Ja ir aktīvs brīdinājums, aizveram to
       if (warning_popup) {
           stopBuzzerSound(); // Apstādinām skaņu
           lv_obj_del(warning_popup);
           warning_popup = NULL;
           warning_popup_visible = false;
           buzzer_muted = false; // Atiestatām klusuma statusu, lai nākamreiz atkal skanētu
       }
   }
   
   lv_bar_set_range(blue_bar, 0, targetTempC*1.22);
   lv_bar_set_value(blue_bar, temperature, LV_ANIM_OFF);
   if(temp_label) {
       static char buf[32];
       snprintf(buf, sizeof(buf), "%d", temperature);
       lv_label_set_text(temp_label, buf);
   }
}

void lvgl_display_init() {
   ts.begin();
   ts.setRotation(0);
   display.setSwapBytes(true);
   display.init();
   display.setBrightness(128);
   display.setRotation(0);
   
   lv_init();
   bool psram_success = init_psram_buffers();
   
#ifdef BOARD_HAS_PSRAM
   if (psram_success && buf1 && buf2) {
       lv_disp_draw_buf_init(&draw_buf, buf1, buf2, BUFFER_SIZE);
   } else {
       static lv_color_t fallback_buf1[WIDTH * (HEIGHT/20)];
       static lv_color_t fallback_buf2[WIDTH * (HEIGHT/20)];
       lv_disp_draw_buf_init(&draw_buf, fallback_buf1, fallback_buf2, WIDTH * (HEIGHT/20));
   }
#else
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


   time_label = lv_label_create(lv_scr_act());
   lv_obj_set_style_text_font(time_label, &lv_font_montserrat_26, 0);
   lv_obj_set_style_text_color(time_label, lv_color_hex(0x7997a3), 0);
   lv_obj_set_pos(time_label, 180, 25);
   lv_label_set_text(time_label, LV_SYMBOL_WIFI " --:--");


   blue_bar = lv_bar_create(lv_scr_act());
   lv_obj_set_size(blue_bar, 110, 350);
   lv_obj_set_pos(blue_bar, 40, 40);
   lv_obj_set_style_radius(blue_bar, 55, LV_PART_MAIN);
   lv_obj_set_style_clip_corner(blue_bar, true, LV_PART_MAIN);
   lv_obj_set_style_bg_color(blue_bar, lv_color_hex(0x7DD0F2), LV_PART_MAIN | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_color(blue_bar, lv_color_hex(0xff9090), LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_grad_dir(blue_bar, LV_GRAD_DIR_VER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_grad_color(blue_bar, lv_color_hex(0x7DD0F2), LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_opa(blue_bar, 225, LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_grad_stop(blue_bar, 205, LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_main_stop(blue_bar, 101, LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_radius(blue_bar, 0, LV_PART_INDICATOR);


   lv_obj_t * circle = lv_obj_create(lv_scr_act());
   lv_obj_set_size(circle, 150, 150);
   lv_obj_set_pos(circle, 20, 520 - 200);
   lv_obj_set_style_radius(circle, 75, 0);
   lv_obj_set_style_bg_color(circle, lv_color_hex(0x7DD0F2), 0);
   lv_obj_set_style_border_width(circle, 0, 0);
  

   raw_touch_point = lv_obj_create(lv_scr_act());
   lv_obj_set_size(raw_touch_point, 20, 20);
   lv_obj_set_style_radius(raw_touch_point, 10, 0);
   lv_obj_set_style_bg_color(raw_touch_point, lv_color_hex(0x00FF00), 0);
   lv_obj_set_style_border_width(raw_touch_point, 0, 0);
   lv_obj_add_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);


   temp_label = lv_label_create(lv_scr_act());
   lv_obj_set_style_text_font(temp_label, &ekstra, 0);
   lv_obj_set_style_text_color(temp_label, lv_color_hex(0xF3F4F3), 0);
   lv_obj_set_pos(temp_label, 45, 360);
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


   damper_label = lv_label_create(lv_scr_act());
   lv_obj_set_style_text_font(damper_label, &lv_font_montserrat_28, 0);
   lv_obj_set_style_text_color(damper_label, lv_color_hex(0x7997a3), 0);
   lv_obj_set_style_bg_color(damper_label, lv_color_hex(0x7997a3), 0);
   lv_obj_set_pos(damper_label, 200, 290);
   lv_label_set_text(damper_label, "-- %");


   // IZMAINĪTS: label_one ar click event handler
   lv_obj_t *label_one = lv_label_create(lv_scr_act());
   lv_label_set_text(label_one, "2");
   lv_obj_set_style_text_font(label_one, &eeet, 0);
   lv_obj_set_style_text_color(label_one, lv_color_hex(0x7997a3), 0);
   lv_obj_set_style_bg_color(label_one, lv_color_hex(0x7997a3), 0);
   lv_obj_set_pos(label_one, 240, 390);
   
   // JAUNS: Pievienojam click event uz label_one lai atvērtu settings
   lv_obj_add_flag(label_one, LV_OBJ_FLAG_CLICKABLE); // Darām clickable
   lv_obj_add_event_cb(label_one, [](lv_event_t * e) {
       if (lv_event_get_code(e) == LV_EVENT_CLICKED) {

           lvgl_display_show_settings();
       }
   }, LV_EVENT_CLICKED, NULL);


   damper_status_label = lv_label_create(lv_scr_act());
   lv_obj_set_style_text_font(damper_status_label, &ekstra1, 0);
   lv_obj_set_style_text_color(damper_status_label, lv_color_hex(0x7997a3), 0);
   lv_obj_set_style_bg_color(damper_status_label, lv_color_hex(0x7997a3), 0);
   lv_obj_set_pos(damper_status_label, 180, 230);
   lv_label_set_text(damper_status_label, "--");
   
   // JAUNS: Padarām damper_status_label klikšķināmu
   lv_obj_add_flag(damper_status_label, LV_OBJ_FLAG_CLICKABLE);
   
   // JAUNS: Pievienojam click event lai pārslēgtu režīmus - tagad ar PRESSED nevis CLICKED
   lv_obj_add_event_cb(damper_status_label, [](lv_event_t * e) {
       if (lv_event_get_code(e) == LV_EVENT_PRESSED) {  // IZMAINĪTS: PRESSED ir ātrāks nekā CLICKED
           toggle_damper_mode();
       }
   }, LV_EVENT_PRESSED, NULL);


   // JAUNS: AIZSTĀJ target_temp_label ar ROLLER
   main_target_temp_roller = lv_roller_create(lv_scr_act());
   if (main_target_temp_roller) {

       // Set roller options
       lv_roller_set_options(main_target_temp_roller, 
           "64°C\n66°C\n68°C\n70°C\n72°C\n74°C\n76°C\n78°C\n80°C", 
           LV_ROLLER_MODE_NORMAL);
       
       // Pozīcija TĀDA PATI kā target_temp_label (210, 140)
       lv_obj_set_pos(main_target_temp_roller, 210, 140);
       lv_obj_set_size(main_target_temp_roller, 100, 35);
       lv_roller_set_visible_row_count(main_target_temp_roller, 1);

       
       // JAUNS: Padarām roller "neredzamu" - tikai teksts
       // Galvenā daļa - transparent background, bez border
       lv_obj_set_style_bg_opa(main_target_temp_roller, LV_OPA_TRANSP, LV_PART_MAIN);
       lv_obj_set_style_border_width(main_target_temp_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_outline_width(main_target_temp_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_shadow_width(main_target_temp_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_radius(main_target_temp_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_pad_all(main_target_temp_roller, 0, LV_PART_MAIN);
       
       // Selected teksts - tāds pats kā iepriekšējais target_temp_label
       lv_obj_set_style_text_font(main_target_temp_roller, &lv_font_montserrat_28, LV_PART_SELECTED);
       lv_obj_set_style_text_color(main_target_temp_roller, lv_color_hex(0x7997a3), LV_PART_SELECTED);
       lv_obj_set_style_bg_opa(main_target_temp_roller, LV_OPA_TRANSP, LV_PART_SELECTED);
       lv_obj_set_style_border_width(main_target_temp_roller, 0, LV_PART_SELECTED);
       lv_obj_set_style_outline_width(main_target_temp_roller, 0, LV_PART_SELECTED);

       
       // Set initial value BEFORE adding event callback
       int current_index = main_get_temp_index(targetTempC);
       lv_roller_set_selected(main_target_temp_roller, current_index, LV_ANIM_OFF);

       
       // Add event callback
       lv_obj_add_event_cb(main_target_temp_roller, main_temp_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
   }
   
   // JAUNS: Damper roller (sākotnēji paslēpts)
   damper_roller = lv_roller_create(lv_scr_act());
   if (damper_roller) {
       // Set roller options
       lv_roller_set_options(damper_roller, 
           "100%\n80%\n60%\n40%\n20%\n0%", 
           LV_ROLLER_MODE_NORMAL);
       
       // Tāda pati pozīcija kā main_target_temp_roller
       lv_obj_set_pos(damper_roller, 210, 140);
       lv_obj_set_size(damper_roller, 100, 35);
       lv_roller_set_visible_row_count(damper_roller, 1);
       
       // Paslēpjam sākotnēji
       lv_obj_add_flag(damper_roller, LV_OBJ_FLAG_HIDDEN);
       
       // Tāda pati stila iestatīšana kā main_target_temp_roller
       lv_obj_set_style_bg_opa(damper_roller, LV_OPA_TRANSP, LV_PART_MAIN);
       lv_obj_set_style_border_width(damper_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_outline_width(damper_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_shadow_width(damper_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_radius(damper_roller, 0, LV_PART_MAIN);
       lv_obj_set_style_pad_all(damper_roller, 0, LV_PART_MAIN);
       
       lv_obj_set_style_text_font(damper_roller, &lv_font_montserrat_28, LV_PART_SELECTED);
       lv_obj_set_style_text_color(damper_roller, lv_color_hex(0x7997a3), LV_PART_SELECTED);
       lv_obj_set_style_bg_opa(damper_roller, LV_OPA_TRANSP, LV_PART_SELECTED);
       lv_obj_set_style_border_width(damper_roller, 0, LV_PART_SELECTED);
       lv_obj_set_style_outline_width(damper_roller, 0, LV_PART_SELECTED);
       
       // Set initial value to 100%
       lv_roller_set_selected(damper_roller, 0, LV_ANIM_OFF);
       
       // Add event callback
       lv_obj_add_event_cb(damper_roller, damper_roller_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
   } 

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
   lv_obj_t *v_dash_line = lv_line_create(lv_scr_act());
   lv_line_set_points(v_dash_line, v_line_points, 4);
   lv_obj_set_pos(v_dash_line, 40, 100);

   static lv_style_t style_v_dash;
   lv_style_init(&style_v_dash);
   lv_style_set_line_width(&style_v_dash, 2);
   lv_style_set_line_color(&style_v_dash, lv_color_hex(0x7997a3));
   lv_style_set_line_dash_gap(&style_v_dash, 7);
   lv_style_set_line_dash_width(&style_v_dash, 7);

   lv_obj_add_style(v_dash_line, &style_v_dash, LV_PART_MAIN);

}

void lvgl_display_show_touch_point(uint16_t x, uint16_t y, bool show) {
    if (!raw_touch_point) {
        return;
    }
    if (show) {
        lv_obj_set_pos(raw_touch_point, x - 10, y - 10);
        lv_obj_clear_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (!lv_obj_has_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(raw_touch_point, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void lvgl_display_update_damper() {
   if(damper_label) {
       static char buf[16];
       snprintf(buf, sizeof(buf), "%d %%", damper);
       lv_label_set_text(damper_label, buf);
   }
}

// JAUNS: Eksportējam manual_mode stāvokli, lai citas komponentes to varētu izmantot
bool is_manual_damper_mode() {
    return manual_mode;
}

void lvgl_display_update_damper_status() {
   if(damper_status_label) {
       // Izlaižam Serial.print, lai neaizkavētu atjaunināšanu
       
       // Ja esam manuālajā režīmā, nerediģējam tekstu
       if (!manual_mode) {
           lv_label_set_text(damper_status_label, messageDamp.c_str());
       }
       // Manuālajā režīmā nedarām neko - teksts paliek "MANUAL"
   }
}

// IZMAINĪTS: Atjaunojam main screen roller
void lvgl_display_update_target_temp() {
   lvgl_display_update_main_roller();
}

void lvgl_display_set_time(const char* time_str) {
   if (time_label) {
       static char buf[32];
       snprintf(buf, sizeof(buf), LV_SYMBOL_WIFI " %s", time_str);
       lv_label_set_text(time_label, buf);
   }
}

void show_time_on_display() {
   static char last_time_displayed[8] = "";
   static uint32_t last_check = 0;
   if (millis() - last_check < 10000) {
       return;
   }
   last_check = millis();
   String current_time_str = get_time_str();
   char current_time[8];
   strncpy(current_time, current_time_str.c_str(), sizeof(current_time)-1);
   current_time[sizeof(current_time)-1] = '\0';
   if (strcmp(current_time, last_time_displayed) != 0 && strcmp(current_time, "--:--") != 0) {
       strcpy(last_time_displayed, current_time);
       lvgl_display_set_time(current_time);
   }
}

void show_time_reset_cache() {
   String current_time = get_time_str();
   if (current_time != "--:--") {
       lvgl_display_set_time(current_time.c_str());
   }
}

bool init_psram_buffers() {
#ifdef BOARD_HAS_PSRAM
    if (!psramFound()) {
        return false;
    }
    size_t psram_free = ESP.getFreePsram();
    size_t required_size = BUFFER_SIZE * sizeof(lv_color_t) * 2;
    if (psram_free < required_size + 10000) {
        return false;
    }
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