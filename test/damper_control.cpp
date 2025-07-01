#include "damper_control.h"
#include <Arduino.h>
#include <ESP32Servo.h>

// --- Parametri un mainīgie ---
namespace {
    const int minDamper = 0;
    const int maxDamper = 180;
    const int zeroDamper = 0;
    const int endTrigger = 1000;
    const int temperatureMin = 30;
    const float kP = 1.0;
    const float kI = 0.1;
    const float kD = 0.01;

    int TempHist[10] = {0};
    float errP = 0;
    float errI = 0;
    float errD = 0;
    float errOld = 0;
}

void damperControlInit() {
    // Ja nepieciešama inicializācija nākotnē
}

int average(const int* arr, int start, int count) {
    int sum = 0;
    for (int i = start; i < start + count; ++i) {
        sum += arr[i];
    }
    return sum / count;
}

bool WoodFilled(int CurrentTemp) {
    for (int i = 9; i > 0; --i) {
        TempHist[i] = TempHist[i - 1];
    }
    TempHist[0] = CurrentTemp;

    int recent_sum = 0, older_sum = 0;
    for(int i = 0; i < 5; i++) recent_sum += TempHist[i];
    for(int i = 5; i < 10; i++) older_sum += TempHist[i];

    if (recent_sum > older_sum) {
        Serial.println("Malka pievienota");
        return true;
    }
    Serial.println("Malka nav pievienota");
    return false;
}

void damperControlLoop(float targetTempC, float temperature, int &damper) {
    if (errI < endTrigger) {
        errP = targetTempC - temperature;
        errI = errI + errP;
        errD = errP - errOld;
        errOld = errP;
        WoodFilled(temperature);

        int sum_recent = 0, sum_old = 0;
        for(int i=0; i<5; i++) sum_recent += TempHist[i];
        for(int i=5; i<10; i++) sum_old += TempHist[i];

        if (sum_recent > sum_old) {
            errI = 0;
            Serial.println("reset12");
        }
        damper = kP * errP + kI * errI + kD * errD;
        if (damper < minDamper) damper = minDamper;
        if (damper > maxDamper) damper = maxDamper;
    } else {
        if (temperature < temperatureMin) {
            damper = zeroDamper;
        }
    }
}