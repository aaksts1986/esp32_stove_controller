#pragma once

void damperControlInit();
void damperControlLoop();
int average(const int* arr, int start, int count);
bool WoodFilled(int CurrentTemp);
void moveServoToDamper();
void startDamperControlTask();

extern int damper;
extern int minDamper;
extern int maxDamper;
extern int zeroDamper;
extern int endTrigger;
extern int refillTrigger;
