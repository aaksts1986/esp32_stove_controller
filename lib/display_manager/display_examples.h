/**
 * Display Manager - Vienkārši konfigurācijas piemēri
 */

#pragma once
#include "display_manager.h"

// ========================================
// VIENKĀRŠI PIEMĒRI
// ========================================

// Piemērs 1: Event-driven režīms (ieteicamais)
inline void setup_display_event_driven() {
    display_manager_set_update_intervals(
        0,      // Temperatūra: tikai kad mainās
        0,      // Damper: tikai kad mainās  
        30000,  // Laiks: 30 sekundes
        50      // Touch: 50ms (20 FPS)
    );
}

// Piemērs 2: Testēšanas režīms (ātrāki atjauninājumi)
inline void setup_display_for_testing() {
    display_manager_set_update_intervals(
        1000,   // Temperatūra: 1 sekunde
        500,    // Damper: 0.5 sekundes
        10000,  // Laiks: 10 sekundes
        25      // Touch: 25ms (40 FPS)
    );
}
