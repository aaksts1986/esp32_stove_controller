#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <lv_conf.h>

// Temperatūras sensora inicializācija
OneWire oneWire(6);
DallasTemperature ds(&oneWire);
DeviceAddress sensor1 = {0x28, 0xFC, 0x70, 0x96, 0xF0, 0x01, 0x3C, 0xC0};
int temperature = 0;


TFT_eSPI tft = TFT_eSPI();  // Izveido TFT objektu

// Globālais mainīgais etiķetei
lv_obj_t *label;

// Iekļauj konvertēto attēlu (xxlxx.c)
#include "xxlxx.c"

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void setup() {
    // Inicializē Serial Monitor
    Serial.begin(115200);
    Serial.println("Sākums!");

    // Inicializē TFT displeju
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    // Inicializē LVGL
    lv_init();

    // Pievieno displeja draiveri LVGL
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[TFT_WIDTH * 50];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * 50);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Izveido LVGL ekrānu un attēlu fonam
    lv_obj_t *bg_image = lv_img_create(lv_scr_act());  // Izveido attēla objektu
    lv_img_set_src(bg_image, &xxlxx);  // Iestata attēlu no struktūras

    // Pielāgo attēla novietojumu (centrēts)
    lv_obj_align(bg_image, LV_ALIGN_CENTER, 0, 0);

    // Izveido LVGL etiķeti virs attēla
    label = lv_label_create(lv_scr_act());  // Globālā etiķete
    lv_label_set_text(label, " --.- °C");  // Sākotnējais teksts

    // Iestata teksta izmēru (3x lielāks)
    static lv_style_t label_style;
    lv_style_init(&label_style);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_48);  // Lielāks fonts
    lv_style_set_text_color(&label_style, lv_color_hex(0xFF0000));  // Sarkana krāsa
    lv_obj_add_style(label, &label_style, 0);

    // Centrē etiķeti
    
    lv_obj_set_pos(label, 150, 65);  // x un y ir koordinātas, kurās vēlaties novietot tekstu

    // Inicializē temperatūras sensoru
    ds.begin();
    ds.setResolution(sensor1, 10);  // Iestatiet sensoram izšķirtspēju (9-12 biti)

    Serial.println("Etiķete un sensors inicializēti!");
}

void loop() {
    static unsigned long lastUpdate = 0;  // Laika atzīme pēdējai temperatūras atjaunināšanai

    // Atjaunina temperatūru ik pēc 1 sekundes
    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();

        // Nolasa temperatūru no sensora
        ds.requestTemperatures();
        temperature = ds.getTempC(sensor1);

        // Atjaunina tekstu uz ekrāna
        lv_label_set_text_fmt(label ,"%d °C", temperature);

        // Izvada temperatūru seriālajā monitorā
        Serial.print("Temperatūra: ");
        Serial.print(temperature);
        Serial.println(" °C");
    }

    lv_timer_handler();  // Apstrādā LVGL notikumus
    lv_tick_inc(5);      // Pievieno LVGL 5 ms, lai notiktu laika atjauninājumi
    delay(5);            // Neliels aizkaves laiks
}

// 8.4 versija