#pragma once
#include <Arduino.h>
#include <AsyncTelegram2.h>

void wifi_connect();
void sync_time();
String get_time_str();

// OTA funkcijas
void ota_setup();
void ota_loop();

// Telegram funkcijas
void telegram_setup();
void handleTelegramMessages();
void handleTelegramMessages(AsyncTelegram2 &myBot);
bool isTargetTempChanged(); // Pārbauda vai mērķa temperatūra ir mainīta

// Telegram mainīgie
extern bool waitingForTemp;
extern bool waitingForKP;
extern bool waitingForTempMin;