/**
 * @file lv_conf.h
 * Configuration file for LVGL
 */

 #ifndef LV_CONF_H
 #define LV_CONF_H
 
 /*====================
  * Graphical settings
  *====================*/
 
 /* Maximal horizontal and vertical resolution to support by the library. */
 #define LV_HOR_RES_MAX          (320)  // Pielāgojiet savam TFT displeja platumam
 #define LV_VER_RES_MAX          (240)  // Pielāgojiet savam TFT displeja augstumam
 
 /* Color depth:
  * - 1:  1 byte per pixel (monochrome)
  * - 8:  RGB332
  * - 16: RGB565
  * - 32: ARGB8888
  */
 #define LV_COLOR_DEPTH         16  // RGB565 formāts (parasti izmanto TFT displejiem)
 
 /* Size of the memory used for graphics (in bytes).
  * Adjust this based on your available RAM.
  */
 #define LV_MEM_SIZE            (64U * 1024U)  // 32 KB atmiņas LVGL
 
 /* Use a custom allocator function for memory management. */
 #define LV_MEM_CUSTOM          0
 #if LV_MEM_CUSTOM == 0
 #  define LV_MEMCPY_MEMSET_STD 1  // Izmantot standarta memcpy un memset
 #endif
 
 /* Enable anti-aliasing (lines and curves will be smoothed). */
 #define LV_ANTIALIAS           1
 
 /* Enable GPU acceleration (if supported by your hardware). */
 #define LV_USE_GPU             0
 #define LV_USE_ST7789        1
 #define LV_USE_TFT_ESPI         1
 
 /*===================
  * Log settings
  *===================*/
 
 /* Enable logging for LVGL. */
 #define LV_USE_LOG             1
 #if LV_USE_LOG
 #  define LV_LOG_LEVEL         LV_LOG_LEVEL_WARN  // Log līmenis (piemēram, brīdinājumi)
 #  define LV_LOG_PRINTF        1  // Izmantot printf loggēšanai
 #endif
 
 /*===================
  * Feature usage
  *===================*/
 
 /* Enable/disable features as needed. */
 #define LV_USE_ASSERT          1  // Aktivizēt asertācijas
 #define LV_USE_USER_DATA       1  // Atļaut lietotāja datus objektos
 
 /* Enable additional widgets. */
#define LV_USE_LABEL           1  // Atbalsts tekstam (label)
#define LV_USE_BTN             1  // Atbalsts pogām (button)
#define LV_USE_IMG             1  // Atbalsts attēliem
#define LV_USE_ARC             1  // Atbalsts lokiem (arc)
#define LV_USE_BAR             1  // Atbalsts progresa joslām (bar)
#define LV_USE_SLIDER          1  // Atbalsts slīdņiem (slider)
#define LV_USE_SWITCH          1  // Atbalsts slēdžiem (switch)
#define LV_USE_CHECKBOX        1  // Atbalsts izvēles rūtiņām (checkbox)
#define LV_USE_ROLLER          1  // Atbalsts ritināšanas sarakstiem (roller)
#define LV_USE_DROPDOWN        1  // Atbalsts nolaižamajiem sarakstiem (dropdown)
#define LV_USE_TEXTAREA        1  // Atbalsts teksta ievades laukiem (textarea)
#define LV_USE_CANVAS          1  // Atbalsts zīmēšanai uz kanva (canvas)
#define LV_USE_TABLE           1  // Atbalsts tabulām (table)
#define LV_USE_TABVIEW         1  // Atbalsts cilnēm (tabview)
#define LV_USE_WIN             1  // Atbalsts logiem (window)
#define LV_FONT_MONTSERRAT_48 1
 /*===================
  * Input device settings
  *===================*/
 
 /* Enable touchpad support (if your display has a touchpad). */
 #define LV_USE_INDEV           1
 #if LV_USE_INDEV
 #  define LV_INDEV_DEF_READ_PERIOD 30  // Touchpad nolasīšanas periods
 #endif
 
 /*===================
  * Others
  *===================*/
 
 /* Use standard `memcpy` and `memset` functions. */
 #define LV_MEMCPY_MEMSET_STD    1
 
 /* Enable the built-in themes. */
 #define LV_USE_THEME_DEFAULT    1  // Noklusējuma tēma
 #define LV_USE_THEME_MONO       0  // Monotona tēma
 
 /* Enable object grouping. */
 #define LV_USE_GROUP           1
 
 /* Enable file system support (if needed). */
 #define LV_USE_FS              0
 
 /* Enable animation support. */
 #define LV_USE_ANIMATION       1
 
 #endif /* LV_CONF_H */