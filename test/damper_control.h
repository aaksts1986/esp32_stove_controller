#pragma once

void damperControlInit();
void damperControlLoop(float targetTempC, float temperature, int &damper);
int average(const int* arr, int start, int count);
bool WoodFilled(int CurrentTemp);