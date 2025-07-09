#pragma once
#include <Arduino.h>
#include "display_config.h"  // Include configuration

/**
 * Display Manager - Centralizēta displeja atjaunošanas pārvaldība
 * 
 * Šis modulis pārvalda visus displeja atjauninājumus ar optimizētiem intervaliem
 * un event-driven architecture, lai samazinātu CPU slodzi un uzlabotu performance.
 * 
 * Konfigurāciju var mainīt display_config.h failā.
 */

// Initialization functions
void display_manager_init();

// Main update function - call this from main loop
void display_manager_update();

// Force update functions (useful for debugging/testing)
void display_manager_force_update_all();
void display_manager_force_update_temperature();
void display_manager_force_update_damper();
void display_manager_force_update_damper_status(); // Force only status update
void display_manager_force_update_time();

// Configuration functions
void display_manager_set_update_intervals(
    uint32_t temp_interval_ms,
    uint32_t damper_interval_ms,
    uint32_t time_interval_ms,
    uint32_t touch_interval_ms
);

// Event notification functions (call when data changes)
void display_manager_notify_temperature_changed();
void display_manager_notify_damper_changed();          // For messageDamp changes
void display_manager_notify_damper_position_changed();  // For damper position changes
void display_manager_notify_target_temp_changed();
void display_manager_notify_time_synced();             // Call when NTP time is synchronized

// Configuration and info functions
const char* display_manager_get_config_name();
void display_manager_apply_config_preset();

// Statistics and debugging
typedef struct {
    uint32_t total_updates;
    uint32_t temp_updates;
    uint32_t damper_updates;
    uint32_t touch_updates;
    uint32_t skipped_updates;
    unsigned long last_update_time;
} display_stats_t;

display_stats_t display_manager_get_stats();
void display_manager_get_efficiency_stats(float* efficiency, uint32_t* total_updates, 
                                         uint32_t* skipped_updates, uint32_t* uptime_minutes);
void display_manager_reset_stats();

// Default update intervals (from config file)
#define DEFAULT_TEMP_UPDATE_INTERVAL    TEMP_UPDATE_INTERVAL_MS
#define DEFAULT_DAMPER_UPDATE_INTERVAL  DAMPER_UPDATE_INTERVAL_MS
#define DEFAULT_TIME_UPDATE_INTERVAL    TIME_UPDATE_INTERVAL_MS
#define DEFAULT_TOUCH_UPDATE_INTERVAL   TOUCH_UPDATE_INTERVAL_MS
