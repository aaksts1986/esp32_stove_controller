#pragma once
#include <Arduino.h>

void damperControlInit();
void damperControlLoop();
int average(const int* arr, int start, int count);
bool WoodFilled(int CurrentTemp);
void moveServoToDamper();
void startDamperControlTask();
void ieietDeepSleepArTouch();


extern int damper;
extern int minDamper;
extern int maxDamper;
extern int zeroDamper;
extern int kP;  // PID regulatora koeficients
extern float endTrigger;
extern float refillTrigger;
extern String messageDamp;
