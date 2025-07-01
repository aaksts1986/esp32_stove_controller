#include "damper_control.h"
#include <Arduino.h>
#include <ESP32Servo.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern Servo mansServo; // Pieejams no main.cpp
extern int servoPort;
extern int targetTempC;
extern int temperature;
extern int damper;
TaskHandle_t damperTaskHandle = NULL;

// --- Parametri un mainīgie ---
namespace {
    const int minDamper = 0;
    const int maxDamper = 100;
    const int zeroDamper = 0;
    const int endTrigger = 100000;
    const int refillTrigger = 80000; // vai cita piemērota vērtība
    const int temperatureMin = 30;
    const int kP = 15;            // Overall gain coefficient and P coefficient of the PID regulation
    const float tauI = 1000;         // Integral time constant (sec/repeat)
    const float tauD = 5;            // Derivative time constant (sec/reapeat)
    const float kI =  kP/tauI;        // I coefficient of the PID regulation
    const float kD = kP/tauD;  
    int oldDamper = 0;
    const int servoAngle = 35;
    const int maxDamperx = 100;
    const float servoCalibration = 1.5;
    const int servoOffset = 29;      // D coefficient of the PID regulation

    int TempHist[10] = {0};
    float errP = 0;
    float errI = 0;
    float errD = 0;
    float errOld = 0;
    int minUs = 500;    // servo minimālais impulsa platums
    int maxUs = 2500;   // servo maksimālais impulsa platums
    int minAngle = 0;   // parasti 0
    int maxAngle = 180; // parasti 180
}

void damperControlInit() {
    mansServo.attach(servoPort);
    mansServo.write(50);
    mansServo.detach();
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
        
        return true;
    }
    
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

        int wood = sum_recent / 5;
        int wood2 = sum_old / 5;
        //Serial.printf("Malka: %d, Vecā malka: %d\n", wood, wood2);
        if (wood >= wood2) {
            errI = 0;
            
        }
        damper = kP * errP + kI * errI + kD * errD;
        if (damper < minDamper) damper = minDamper;
        if (damper > maxDamper) damper = maxDamper;
    } else {
        if (temperature < temperatureMin) {
            damper = zeroDamper;
        }
    }

    if (errI < 0) {
        errI = 0;
    }
}



void moveServoToDamper(int damper) {
    int diff = damper - oldDamper;
    if (abs(diff) > 8) {
        mansServo.attach(servoPort);
        if (diff > 0) {
            for (int i = 0; i < diff; i++) {
                int angle = (oldDamper + i + 1) * servoAngle / (maxDamperx * servoCalibration) + servoOffset;
                int us = minUs + (angle - minAngle) * (maxUs - minUs) / (maxAngle - minAngle);
                mansServo.writeMicroseconds(us);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        } else if (diff < 0) {
            for (int i = 0; i < abs(diff); i++) {
                int angle = (oldDamper - i - 1) * servoAngle / (maxDamperx * servoCalibration) + servoOffset;
                int us = minUs + (angle - minAngle) * (maxUs - minUs) / (maxAngle - minAngle);
                mansServo.writeMicroseconds(us);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
        mansServo.detach();
        oldDamper = damper;
    }
}



void DamperTask(void *pvParameters) {
    while (1) {
        damperControlLoop(targetTempC, temperature, damper);
        moveServoToDamper(damper);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void startDamperControlTask() {
    xTaskCreatePinnedToCore(
        DamperTask,
        "DamperTask",
        2048,
        NULL,
        1,
        &damperTaskHandle,
        1
    );
}