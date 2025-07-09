#include "damper_control.h"
#include <Arduino.h>
#include <ESP32Servo.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "temperature.h" 
#include "touch_button.h"
#include "display_manager.h"  // Add for change notifications




Servo mansServo; // Pieejams no main.cpp

String messageDamp;

int damper = 0;
TaskHandle_t damperTaskHandle = NULL;
int servoPort = 5;
float refillTrigger = 500;
int minDamper = 0;
// --- Parametri un mainīgie ---


    int maxDamper = 100;
    int zeroDamper = 0;

    float endTrigger = 1000; // vai cita piemērota vērtība
    int kP = 15;            // Overall gain coefficient and P coefficient of the PID regulation
    float tauI = 1000;         // Integral time constant (sec/repeat)
    float tauD = 5;            // Derivative time constant (sec/reapeat)
    float kI =  kP/tauI;        // I coefficient of the PID regulation
    float kD = kP/tauD;  
    int oldDamper = 0;
    int servoAngle = 35;
    int maxDamperx = 100;
    float servoCalibration = 1.5;
    int servoOffset = 29;      // D coefficient of the PID regulation

    int TempHist[10] = {0};
    float errP = 0;
    float errI = 0;
    float errD = 0;
    float errOld = 0;
    int minUs = 500;    // servo minimālais impulsa platums
    int maxUs = 2500;   // servo maksimālais impulsa platums
    int minAngle = 0;   // parasti 0
    int maxAngle = 180; // parasti 180


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

void ieietDeepSleepArTouch() {
    Serial.println("Sagatavošanās deep sleep...");



    // Aktivē touch wakeup
    touchSleepWakeUpEnable (2, thresholds);

    Serial.println("Ej Deep Sleep... pieskaries, lai pamodinātu.");
    delay(500);

    esp_deep_sleep_start();
}

void damperControlLoop() {
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
        Serial.print("/erri:" + String(errI) +" |  " + "p:" + String(errP) + " |  " + "o:" + String(wood) + " |  " + "p2:" + String(wood2) + " |  " + "d:" + String(errD) + " |  " + "t:" + String(temperature) + "\n");
        if (wood > wood2) {
            errI = 0;
        }
        int oldDamperValue = damper;  // Save old value before calculation
        damper = kP * errP + kI * errI + kD * errD;
        if (damper < minDamper) damper = minDamper;
        if (damper > maxDamper) damper = maxDamper;
        
        // Notify display if damper value changed (even small changes)
        if (damper != oldDamperValue) {
            display_manager_notify_damper_position_changed();
            Serial.printf("Damper value changed: %d -> %d\n", oldDamperValue, damper);
        }
        
       // Serial.printf("Malka: %d", errI);
        //Refill Alarm
        String oldMessage = messageDamp;
        if (errI > refillTrigger) { 
            messageDamp = "FILL!";  
        } else {
            messageDamp = "AUTO";
        }
        
        // Notify display if message changed
        if (messageDamp != oldMessage) {
            display_manager_notify_damper_changed();  // Only status update
            Serial.printf("Damper status changed: %s -> %s\n", oldMessage.c_str(), messageDamp.c_str());
        }

    } else {
        if (temperature < temperatureMin) {
            int oldDamperValue = damper;
            damper = zeroDamper;
            
            // Notify if damper value changed
            if (damper != oldDamperValue) {
                display_manager_notify_damper_position_changed();
                Serial.printf("Damper value changed (END): %d -> %d\n", oldDamperValue, damper);
            }
            
            String oldMessage = messageDamp;
            messageDamp = "END!";
            
            // Notify display if message changed
            if (messageDamp != oldMessage) {
                display_manager_notify_damper_changed();  // Only status update
                Serial.printf("Damper status changed: %s -> %s\n", oldMessage.c_str(), messageDamp.c_str());
            }
            
            delay(500);
            ieietDeepSleepArTouch();


        }
    }

    if (errI < 0) {
        errI = 0;
    }
}



void moveServoToDamper() {
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
        
        // Notify display manager that damper position changed
        display_manager_notify_damper_position_changed();  // Position update
        Serial.printf("Damper moved: %d -> %d (diff: %d)\n", oldDamper, damper, diff);
    }
}



void DamperTask(void *pvParameters) {
    while (1) {
        moveServoToDamper();
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

