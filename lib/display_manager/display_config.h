#pragma once

/**
 * Display Manager Configuration
 * 
 * Vienkārša konfigurācija ar diviem režīmiem:
 * - PURE EVENT: Atjaunina tikai kad dati mainās (maksimāla efektivitāte)
 * - CUSTOM: Pielāgojamas vērtības
 */

// ========================================
// IZVĒLIETIES DISPLAY MODE (tikai vienu!)
// ========================================

#define DISPLAY_MODE_PURE_EVENT     // EVENT-DRIVEN - atjaunina tikai kad mainās!
// #define DISPLAY_MODE_CUSTOM      // Pielāgots - rediģējiet CUSTOM sadaļu zemāk

// ========================================
// KONFIGURĀCIJAS PROFILI
// ========================================

#ifdef DISPLAY_MODE_PURE_EVENT
    // Pure event-driven režīms - atjaunina TIKAI kad dati mainās
    #define TEMP_UPDATE_INTERVAL_MS     0       // 0 = tikai kad mainās
    #define DAMPER_UPDATE_INTERVAL_MS   0       // 0 = tikai kad mainās
    #define TIME_UPDATE_INTERVAL_MS     30000   // 30 sekundes - laika atjaunošana
    #define TOUCH_UPDATE_INTERVAL_MS    50      // 50ms - responsive touch
    
    #define CONFIG_NAME "Pure Event-Driven Mode"
    #define CONFIG_DESCRIPTION "Atjaunina tikai kad dati mainās"

#elif defined(DISPLAY_MODE_CUSTOM)
    // Custom režīms - mainiet šos parametrus pēc vajadzības
    #define TEMP_UPDATE_INTERVAL_MS     1000    // Jūsu izvēle (ms)
    #define DAMPER_UPDATE_INTERVAL_MS   500     // Jūsu izvēle (ms)
    #define TIME_UPDATE_INTERVAL_MS     30000   // Jūsu izvēle (ms)
    #define TOUCH_UPDATE_INTERVAL_MS    50      // Jūsu izvēle (ms)
    
    #define CONFIG_NAME "Custom Mode"
    #define CONFIG_DESCRIPTION "Pielāgota konfigurācija"

#else
    // Default fallback
    #define TEMP_UPDATE_INTERVAL_MS     0
    #define DAMPER_UPDATE_INTERVAL_MS   0
    #define TIME_UPDATE_INTERVAL_MS     30000
    #define TOUCH_UPDATE_INTERVAL_MS    50
    
    #define CONFIG_NAME "Default Mode"
    #define CONFIG_DESCRIPTION "Standarta konfigurācija"
#endif

// ========================================
// DEFAULT KONSTANTES
// ========================================

// Default intervals (izmanto display_manager_init())
#define DEFAULT_TEMP_UPDATE_INTERVAL    TEMP_UPDATE_INTERVAL_MS
#define DEFAULT_DAMPER_UPDATE_INTERVAL  DAMPER_UPDATE_INTERVAL_MS
#define DEFAULT_TIME_UPDATE_INTERVAL    TIME_UPDATE_INTERVAL_MS
#define DEFAULT_TOUCH_UPDATE_INTERVAL   TOUCH_UPDATE_INTERVAL_MS

// ========================================
// PSRAM OPTIMIZATION (vienkāršots)
// ========================================

// PSRAM buffer sizes
#ifdef BOARD_HAS_PSRAM
    #define TELEGRAM_BUFFER_SIZE    4096   // Liels buferis Telegram ziņojumiem
    #define LVGL_BUFFER_FRACTION    15     // LVGL bufera izmērs
#else
    #define TELEGRAM_BUFFER_SIZE    1024   // Mazāks buferis bez PSRAM
    #define LVGL_BUFFER_FRACTION    10     // Standarta bufera izmērs
#endif

// Citas konstantes
#define MAX_UPDATE_DELAY_MS         5000   // Maksimālais laiks starp atjauninājumiem
#define SHOW_CONFIG_INFO            false  // Nērādīt konfigurācijas info (production)
#define DISPLAY_DEBUG_ENABLED       false  // Debug ziņojumi (false = production)

// Helper makros (ar aizsardzību pret dalīšanu ar 0)
#define MS_TO_SEC_TEXT(ms) ((ms) / 1000.0f)  // Konvertē ms uz sekundēm
#define TEMP_FPS    (TEMP_UPDATE_INTERVAL_MS > 0 ? (1000.0f / TEMP_UPDATE_INTERVAL_MS) : 0.0f)
#define DAMPER_FPS  (DAMPER_UPDATE_INTERVAL_MS > 0 ? (1000.0f / DAMPER_UPDATE_INTERVAL_MS) : 0.0f)
#define TOUCH_FPS   (TOUCH_UPDATE_INTERVAL_MS > 0 ? (1000.0f / TOUCH_UPDATE_INTERVAL_MS) : 0.0f)
