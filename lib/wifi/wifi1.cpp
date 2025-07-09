#include <Arduino.h>
#include <FS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "wifi1.h"
#include "lvgl_display.h"  // For time display functions
#include "display_manager.h"  // For display manager notifications
#include "display_config.h"   // For PSRAM optimization settings

#include <ArduinoOTA.h>
#include <AsyncTelegram2.h>
#include <ArduinoJson.h>

// Import external variables
extern int temperature;
extern int targetTempC;
extern int kP;
extern int temperatureMin;
extern String messageDamp;

// PSRAM-optimized string allocation for Telegram
char* allocate_telegram_string(size_t size) {
#ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
        return (char*)ps_malloc(size);
    }
#endif
    return (char*)malloc(size);
}

void free_telegram_string(char* ptr) {
    if (ptr) {
        free(ptr);  // free() works for both malloc() and ps_malloc()
    }
}

// PSRAM-optimized Telegram message builder
String build_telegram_message_psram(const char* format, ...) {
#ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
        char* buffer = allocate_telegram_string(TELEGRAM_BUFFER_SIZE);
        if (buffer) {
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, TELEGRAM_BUFFER_SIZE, format, args);
            va_end(args);
            
            String result = String(buffer);
            free_telegram_string(buffer);
            return result;
        }
    }
#endif
    // Fallback for smaller messages in internal RAM
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return String(buffer);
}

#define BOT_TOKEN "7495409709:AAFuPnpwo0RJOmQZ3qX9ZjgXHKtjrNpvNFw"
//#define chatId "6966768770"                                   // Aizstāt ar savu chat ID

WiFiClientSecure client;
AsyncTelegram2 myBot(client);

// PSRAM-optimized Telegram client setup
void setup_telegram_psram() {
#ifdef BOARD_HAS_PSRAM
    if (psramFound()) {
        // Configure Telegram for PSRAM usage
        // Note: WiFiClientSecure doesn't have setBufferSizes in ESP32 Arduino Core
        // We'll optimize through other means (larger message buffers)
    }
#endif
}

// Telegram state variables
bool waitingForTemp = false;
bool waitingForKP = false;
bool waitingForTempMin = false;

// Flags for display updates
bool targetTempChanged = false;


const char* ssid = "HUAWEI-B525-90C8";         // <-- Šeit ieraksti savu WiFi nosaukumu
const char* password = "BTT6F1EA171";   // <-- Šeit ieraksti savu WiFi paroli
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 2 * 3600;    // Latvija: GMT+2
const int daylightOffset_sec = 3600;    // Vasaras laiks

void wifi_connect() {
    Serial.printf("Connecting to WiFi: %s\n", ssid);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        attempts++;
        if (attempts % 10 == 0) {
            Serial.printf("WiFi connecting... (%d seconds)\n", attempts / 2);
        }
    }
    
    Serial.println("WiFi connected successfully!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
}

void sync_time() {
    Serial.println("Starting NTP time synchronization...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Wait for NTP sync with timeout
    int attempts = 0;
    const int max_attempts = 20; // 10 seconds timeout
    
    while (attempts < max_attempts) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            Serial.println("NTP time synchronized successfully!");
            
            // Notify display manager and force immediate time update
            display_manager_notify_time_synced();  // Notify display manager
            show_time_reset_cache();               // Clear time cache
            show_time_on_display();                // Force immediate update
            Serial.println("Time display updated after NTP sync");
            
            return; // Success
        }
        
        delay(500);
        attempts++;
        if (attempts % 4 == 0) {
            Serial.printf("Waiting for NTP sync... (%d/%d)\n", attempts, max_attempts);
        }
    }
    
    Serial.println("Warning: NTP synchronization timeout!");
}

String get_time_str() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "--:--";
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
    return String(buf);
}

void ota_setup() {
    ArduinoOTA.setHostname("esp32-ota");
    ArduinoOTA.begin();
}

void ota_loop() {
    ArduinoOTA.handle();
}

void telegram_setup() {
    // Setup PSRAM optimization for Telegram
    setup_telegram_psram();
    
    client.setCACert(telegram_cert); // Izmanto iebūvēto Telegram sertifikātu
    myBot.setUpdateTime(2000); // Atjaunināt ik pēc 2 sekundēm
    myBot.setTelegramToken(BOT_TOKEN);
}

void handleTelegramMessages() {
    handleTelegramMessages(myBot);
}

void handleTelegramMessages(AsyncTelegram2 &myBot) {
    TBMessage msg;
    if (myBot.getNewMessage(msg)) {
        if (msg.messageType == MessageText) {
            // Ja lietotājs sūta /info, rāda inline pogas
            if (msg.text == "/info") {
                // Izveido inline pogu izvēlni
                InlineKeyboard myInlineKeyboard;
                myInlineKeyboard.addButton("🔄 Krāsns info", "refresh", KeyboardButtonQuery);
                myInlineKeyboard.addRow(); // Pievieno jaunu rindu
                myInlineKeyboard.addButton("🌡️ Mainīt mērķa temperatūru", "change_temp", KeyboardButtonQuery);
                myInlineKeyboard.addButton("🎚️ Mainīt kP", "change_kp", KeyboardButtonQuery);
                myInlineKeyboard.addRow(); // Pievieno jaunu rindu
                myInlineKeyboard.addButton("❄️ Mainīt minimālo temperatūru", "change_temp_min", KeyboardButtonQuery);

                // Sūta ziņojumu ar inline pogām
                myBot.sendMessage(msg, "Izvēlies darbību:", myInlineKeyboard);
            }
            // Time sync and display test commands
            else if (msg.text == "/time") {
                String current_time = get_time_str();
                myBot.sendMessage(msg, "🕐 Pašreizējais laiks: " + current_time);
            }
            else if (msg.text == "/sync_time") {
                myBot.sendMessage(msg, "🔄 Sinhronizē laiku ar NTP serveri...");
                sync_time();
                String new_time = get_time_str();
                myBot.sendMessage(msg, "✅ Laiks sinhronizēts: " + new_time);
            }
            else if (msg.text == "/reset_time_display") {
                show_time_reset_cache();
                display_manager_force_update_time();
                show_time_on_display();
                String current_time = get_time_str();
                myBot.sendMessage(msg, "🔄 Laika attēlošana pārstartēta: " + current_time);
            }
            // Ja lietotājs ievada skaitli un gaida temperatūras ievadi
            else if (waitingForTemp && isDigit(msg.text[0])) {
                targetTempC = msg.text.toInt(); // Konvertē ievadi uz int
                targetTempChanged = true; // Iezīmē, ka temperatūra ir mainīta
                display_manager_notify_target_temp_changed(); // Notificē displeja pārvaldnieku par izmaiņām
                myBot.sendMessage(msg, "✅ Mērķa temperatūra atjaunināta uz: " + String(targetTempC) + " °C");
                waitingForTemp = false; // Pārtrauc gaidīt temperatūras ievadi
            }
            // Ja lietotājs ievada skaitli un gaida kP ievadi
            else if (waitingForKP && isDigit(msg.text[0])) {
                kP = msg.text.toInt(); // Konvertē ievadi uz int
                myBot.sendMessage(msg, "✅ kP vērtība atjaunināta uz: " + String(kP));
                waitingForKP = false; // Pārtrauc gaidīt kP ievadi
            }
            // Ja lietotājs ievada skaitli un gaida minimālās temperatūras ievadi
            else if (waitingForTempMin && isDigit(msg.text[0])) {
                temperatureMin = msg.text.toInt(); // Konvertē ievadi uz int
                myBot.sendMessage(msg, "✅ Minimālā temperatūra atjaunināta uz: " + String(temperatureMin) + " °C");
                waitingForTempMin = false; // Pārtrauc gaidīt minimālās temperatūras ievadi
            }
            else {
                myBot.sendMessage(msg, "❌ Nederīga ievade. Lūdzu, ievadiet veselu skaitli.");
            }
        }
        else if (msg.messageType == MessageQuery) {
            String callback_data = msg.callbackQueryData;

            if (callback_data == "refresh") {
                // Simple status message with only furnace parameters
                String status_msg = build_telegram_message_psram(
                    "🔥 KRĀSNS STATUS 🔥\n\n"
                    "🌡️ Temperatūra: %d °C\n"
                    "🎯 Mērķis: %d °C\n"
                    "⚙️ kP vērtība: %d\n"
                    "❄️ Minimālā: %d °C\n"
                    "🎚️ Damper: %s",
                    temperature, targetTempC, kP, temperatureMin,
                    messageDamp.c_str()
                );
                myBot.sendMessage(msg, status_msg);
            }
            else if (callback_data == "change_temp") {
                // Lūdz lietotājam ievadīt jaunu temperatūru, izmantojot ForceReply
                myBot.sendMessage(msg, "Lūdzu, ievadiet jauno mērķa temperatūru (°C):", (char*)"", true);
                waitingForTemp = true; // Sāk gaidīt temperatūras ievadi
                waitingForKP = false;  // Pārtrauc gaidīt kP ievadi
                waitingForTempMin = false; // Pārtrauc gaidīt minimālās temperatūras ievadi
            }
            else if (callback_data == "change_kp") {
                // Lūdz lietotājam ievadīt jaunu kP vērtību, izmantojot ForceReply
                myBot.sendMessage(msg, "Lūdzu, ievadiet jauno kP vērtību:", (char*)"", true);
                waitingForKP = true;  // Sāk gaidīt kP ievadi
                waitingForTemp = false; // Pārtrauc gaidīt temperatūras ievadi
                waitingForTempMin = false; // Pārtrauc gaidīt minimālās temperatūras ievadi
            }
            else if (callback_data == "change_temp_min") {
                // Lūdz lietotājam ievadīt jaunu minimālo temperatūru, izmantojot ForceReply
                myBot.sendMessage(msg, "Lūdzu, ievadiet jauno minimālo temperatūru (°C):", (char*)"", true);
                waitingForTempMin = true; // Sāk gaidīt minimālās temperatūras ievadi
                waitingForTemp = false; // Pārtrauc gaidīt temperatūras ievadi
                waitingForKP = false; // Pārtrauc gaidīt kP ievadi
            }

            // Apstiprina callback (obligāti, lai Telegram zinātu, ka poga ir apstrādāta)
            //myBot.endQuery(msg, "Darbība veikta!", true);
        }
    }
}

bool isTargetTempChanged() {
    if (targetTempChanged) {
        targetTempChanged = false; // Atiestatām flag
        return true;
    }
    return false;
}