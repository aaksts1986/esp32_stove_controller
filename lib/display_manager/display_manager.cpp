#include "display_manager.h"
#include "display_config.h"
#include "lvgl_display.h"
#include "wifi1.h"
#include "temperature.h"  // Add temperature for change detection

// External variables
extern int targetTempC;

// Internal state structure
typedef struct {
    // Timing variables
    unsigned long last_temp_update;
    unsigned long last_damper_update;
    unsigned long last_time_update;
    unsigned long last_touch_update;
    
    // Update intervals (configurable)
    uint32_t temp_update_interval;
    uint32_t damper_update_interval;
    uint32_t time_update_interval;
    uint32_t touch_update_interval;
    
    // Event flags for forced updates
    bool force_temp_update;
    bool force_damper_update;
    bool force_damper_status_update;
    bool force_time_update;
    bool force_all_update;
    
    // Initialization flag
    bool initialized;
} display_manager_state_t;

// Global state
static display_manager_state_t dm_state = {0};

void display_manager_init() {
    // Initialize timing
    unsigned long now = millis();
    dm_state.last_temp_update = now;
    dm_state.last_damper_update = now;
    dm_state.last_time_update = now;
    dm_state.last_touch_update = now;
    
    // Set default intervals
    dm_state.temp_update_interval = DEFAULT_TEMP_UPDATE_INTERVAL;
    dm_state.damper_update_interval = DEFAULT_DAMPER_UPDATE_INTERVAL;
    dm_state.time_update_interval = DEFAULT_TIME_UPDATE_INTERVAL;
    dm_state.touch_update_interval = DEFAULT_TOUCH_UPDATE_INTERVAL;
    
    // Reset flags
    dm_state.force_temp_update = false;
    dm_state.force_damper_update = false;
    dm_state.force_damper_status_update = false;
    dm_state.force_time_update = false;
    dm_state.force_all_update = false;
    
    dm_state.initialized = true;
}

void display_manager_update() {
    if (!dm_state.initialized) {
        display_manager_init();
    }
    
    unsigned long now = millis();
    
    // High priority: Touch updates (responsive UI)
    if ((now - dm_state.last_touch_update >= dm_state.touch_update_interval) || 
        dm_state.force_all_update) {
        
        lvgl_display_touch_update();
        dm_state.last_touch_update = now;
    }
    
    // Medium priority: Temperature and bars (ONLY when temperature changes OR forced)
    bool should_update_temp = dm_state.force_temp_update ||
                              dm_state.force_all_update ||
                              hasTemperatureChanged() ||        // NEW: Only when temp actually changes!
                              isTargetTempChanged();
    
    // Pure event-driven: if interval is 0, no fallback updates at all!
    bool temp_fallback_needed = false;
    if (dm_state.temp_update_interval > 0) {
        temp_fallback_needed = (now - dm_state.last_temp_update) > (dm_state.temp_update_interval * 3);
    }
    
    if (should_update_temp || temp_fallback_needed) {
        lvgl_display_update_bars();
        
        // Check if target temperature changed OR if forced update requested
        if (isTargetTempChanged() || dm_state.force_temp_update) {
            lvgl_display_update_target_temp();
        }
        
        dm_state.last_temp_update = now;
        dm_state.force_temp_update = false;
    }
    
    // Medium priority: Damper updates (ONLY when damper changes OR forced)
    bool should_update_damper = dm_state.force_damper_update ||
                                dm_state.force_all_update;
    
    // Pure event-driven: if interval is 0, no fallback updates at all!
    bool damper_fallback_needed = false;
    if (dm_state.damper_update_interval > 0) {
        damper_fallback_needed = (now - dm_state.last_damper_update) > (dm_state.damper_update_interval * 5);
    }
    
    if (should_update_damper || damper_fallback_needed) {
        lvgl_display_update_damper();
        dm_state.last_damper_update = now;
        dm_state.force_damper_update = false;
    }
    
    // Separate handling for damper status (only when messageDamp changes)
    if (dm_state.force_damper_status_update || dm_state.force_all_update) {
        lvgl_display_update_damper_status();
        dm_state.force_damper_status_update = false;
    }
    
    // Low priority: Time updates - Optimized with internal minute-change detection
    // The show_time_on_display() function now only updates display when minute actually changes
    // This interval serves as a fallback check and reduces CPU usage between checks
    bool should_update_time = (now - dm_state.last_time_update >= dm_state.time_update_interval) ||
                              dm_state.force_time_update ||
                              dm_state.force_all_update;
    
    if (should_update_time) {
        show_time_on_display();
        dm_state.last_time_update = now;
        dm_state.force_time_update = false;
    }
    
    // Reset force all flag
    if (dm_state.force_all_update) {
        dm_state.force_all_update = false;
    }
}

void display_manager_force_update_all() {
    dm_state.force_all_update = true;
}

void display_manager_force_update_temperature() {
    dm_state.force_temp_update = true;
}

void display_manager_force_update_damper() {
    dm_state.force_damper_update = true;
}

void display_manager_force_update_time() {
    dm_state.force_time_update = true;
}

void display_manager_set_update_intervals(
    uint32_t temp_interval_ms,
    uint32_t damper_interval_ms,
    uint32_t time_interval_ms,
    uint32_t touch_interval_ms) {
    
    dm_state.temp_update_interval = temp_interval_ms;
    dm_state.damper_update_interval = damper_interval_ms;
    dm_state.time_update_interval = time_interval_ms;
    dm_state.touch_update_interval = touch_interval_ms;
}

void display_manager_notify_temperature_changed() {
    dm_state.force_temp_update = true;
}

void display_manager_notify_damper_changed() {
    dm_state.force_damper_status_update = true;
}

void display_manager_notify_damper_position_changed() {
    dm_state.force_damper_update = true;
}

void display_manager_notify_target_temp_changed() {
    dm_state.force_temp_update = true;
}

void display_manager_notify_time_synced() {
    // Called when NTP time is synchronized - force immediate time update
    dm_state.force_time_update = true;
}

void display_manager_apply_config_preset() {
    // Apply configuration from display_config.h
    dm_state.temp_update_interval = TEMP_UPDATE_INTERVAL_MS;
    dm_state.damper_update_interval = DAMPER_UPDATE_INTERVAL_MS;
    dm_state.time_update_interval = TIME_UPDATE_INTERVAL_MS;
    dm_state.touch_update_interval = TOUCH_UPDATE_INTERVAL_MS;
}

const char* display_manager_get_config_name() {
    return CONFIG_NAME;
}
