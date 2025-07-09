# Display Manager

Vienkāršs un efektīvs displeja pārvaldības modulis ESP32 projektiem.

## Galvenās funkcijas

- **Event-driven atjaunināšana** - displejs atjaunojas tikai kad dati mainās
- **PSRAM optimizācija** - izmanto PSRAM lieliem buferiem
- **Vienkārša konfigurācija** - tikai 2 režīmi: PURE_EVENT un CUSTOM

## Lietošana

1. **Inicializācija:**
```cpp
display_manager_init();
```

2. **Konfigurācija (ieteicamā):**
```cpp
// Event-driven režīms (maksimāla efektivitāte)
display_manager_set_update_intervals(0, 0, 30000, 50);
```

3. **Atjaunināšana loop() funkcijā:**
```cpp
display_manager_update();
```

4. **Paziņošana par izmaiņām:**
```cpp
display_manager_notify_target_temp_changed();
display_manager_notify_damper_position_changed();
```

## Konfigurācija

Rediģējiet `display_config.h`:

- **DISPLAY_MODE_PURE_EVENT** - atjaunina tikai kad dati mainās (ieteicamais)
- **DISPLAY_MODE_CUSTOM** - pielāgojamas vērtības

## Debug

Lai redzētu konfigurāciju:
```cpp
display_manager_print_config();
```
