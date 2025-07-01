#include <ESP32Servo.h>
#include <OneWire.h>

// --- Konstantes un mainīgie ---
const int minDamper = 0;
const int maxDamper = 180;
const int zeroDamper = 0;
const int endTrigger = 1000;      // piemērs, pielāgo pēc vajadzības
const int temperatureMin = 30;    // piemērs, pielāgo pēc vajadzības
const float kP = 1.0;             // PID proportional koeficients
const float kI = 0.1;             // PID integral koeficients
const float kD = 0.01;            // PID derivative koeficients

int wood = 0;
int wood2 = 0;
int damper = 0;
int damper2 = 0;
int raw2[10] = {0};
int TempHist[10] = {0}; // Temperatūras vēsture

// PID mainīgie
float errP = 0;
float errI = 0;
float errD = 0;
float errOld = 0;
float targetTempC = 70;   // Mērķa temperatūra, piemērs
float temperature = 0;    // Pašreizējā temperatūra

Servo myservo;

// --- Palīgfunkcija vidējās vērtības aprēķinam ---
int average(const int* arr, int start, int count) {
    int sum = 0;
    for (int i = start; i < start + count; ++i) {
        sum += arr[i];
    }
    return sum / count;
}

// --- Atjauno temperatūras vēsturi un pārbauda, vai pievienota malka ---
bool WoodFilled(int CurrentTemp) {
    // Pārbīda vēsturi (efektīvāk ar --i)
    for (int i = 9; i > 0; --i) {
        TempHist[i] = TempHist[i - 1];
    }
    TempHist[0] = CurrentTemp;

    // Efektīvs vidējā aprēķins
    int recent_sum = 0, older_sum = 0;
    for(int i = 0; i < 5; i++) recent_sum += TempHist[i];
    for(int i = 5; i < 10; i++) older_sum += TempHist[i];

    // Atkļūdošanas iespējas
    if (recent_sum > older_sum) {
        Serial.println("Malka pievienota");
        return true;
    }
    Serial.println("Malka nav pievienota");
    return false;
}

void setup(void) 
{
    Serial.begin(115200);
    analogReadResolution(9);
    // analogSetAttenuation(ADC_0db); 
    adcAttachPin(9);

    myservo.attach(5);  // attach servo object to pin 5
    myservo.write(50); 
    myservo.detach();    // detach servo object from its pin
}

void loop() {
    // Piemērs: nolasīt temperatūru (pielāgo pēc savas shēmas)
    // temperature = analogRead(9); // Atkomentē, ja nepieciešams

    if (errI < endTrigger) {
        // Automatic PID regulation
        errP = targetTempC - temperature;  // set proportional term
        errI = errI + errP;                // update integral term
        errD = errP - errOld;              // update derivative term
        errOld = errP;
        WoodFilled(temperature);  // Atjauno TempHist

        // Optimizēts vidējo vērtību aprēķins
        int sum_recent = 0, sum_old = 0;
        for(int i=0; i<5; i++) sum_recent += TempHist[i];
        for(int i=5; i<10; i++) sum_old += TempHist[i];

        if (sum_recent > sum_old) {  // Nav nepieciešams dalīt, jo salīdzina tās pašas izmēra grupas
            errI = 0;
            Serial.println("reset12");
        }
        // set damper position and limit damper values to physical constraints
        damper = kP * errP + kI * errI + kD * errD;
        if (damper < minDamper) damper = minDamper;
        if (damper > maxDamper) damper = maxDamper;
    } else {
        // End of combustion condition for errI >= endTrigger
        if (temperature < temperatureMin) {
            damper = zeroDamper;
        }
    }

    delay(200);
}
