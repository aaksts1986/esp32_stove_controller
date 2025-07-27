#include "damper_control.h"
#include <Arduino.h>
#include <ESP32Servo.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "temperature.h" 
#include "touch_button.h"
#include "display_manager.h"
#include "telnet.h"

// Servo un kontroles mainīgie
Servo mansServo;
String messageDamp = "MANUAL";

// Servo konfigurācija
static bool servoAttached = false;
int servoPort = 5;
int servoAngle = 35;
float servoCalibration = 1.5;
int servoOffset = 29;
int minUs = 500;
int maxUs = 2500;
int minAngle = 0;
int maxAngle = 180;

// Damper pozīcijas mainīgie
int damper = 100;
int minDamper = 0;
int maxDamper = 100;
int zeroDamper = 0;
int oldDamper = 0;

// Uzdevuma rokturis
TaskHandle_t damperTaskHandle = NULL;

// PID kontroliera parametri
float refillTrigger = 5000;
float endTrigger = 10000;
int kP = 25;
float tauI = 1000;
float tauD = 5;
float kI = kP / tauI;
float kD = kP / tauD;

// Temperatūras vēstures masīvs un PID mainīgie
int TempHist[10] = {0};
float errP = 0;
float errI = 0;
float errD = 0;
float errOld = 0;

// Asinhronās servo kustības mainīgie
static int targetDamper = 0;
static int currentDamper = 0;
bool servoMoving = false;
static TickType_t lastMoveTime = 0;
int servoStepInterval = 50;  // Servo kustības solis milisekundēs (var mainīt no iestatījumiem)

// Zemas temperatūras pārbaudes mainīgie
static unsigned long lowTempStartTime = 0;
bool lowTempCheckActive = false;
static int initialTemperature = 0; // Sākotnējā temperatūra, kad sākas pārbaude
unsigned long LOW_TEMP_TIMEOUT = 240000; // 4 minūtes (4*60*1000 ms)

int buzzer = 14;

// Buzzer kontroles mainīgie
bool isWarningActive = false;
TaskHandle_t buzzerTaskHandle = NULL;

// Buzzer kontroles funkcijas
void initBuzzer() {
    pinMode(buzzer, OUTPUT);
    digitalWrite(buzzer, LOW);
}

// Buzzer uzdevuma funkcija, kas laiž nepārtrauktu skaņu
void buzzerTask(void *pvParameters) {
    while (1) {
        if (isWarningActive) {
            // Mainīgais signāls - 100ms skaņa, 100ms pauze
            digitalWrite(buzzer, HIGH);
            delay(100);  
            digitalWrite(buzzer, LOW);
            delay(100);
        } else {
            // Ja brīdinājums ir izslēgts, apstādinam signālu un gaidām
            digitalWrite(buzzer, LOW);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

void startBuzzerSound() {
    isWarningActive = true;
}

void stopBuzzerSound() {
    isWarningActive = false;
}

void playWarningSound() {
    startBuzzerSound();
}

// Aprēķina vidējo vērtību masīva daļai
int average(const int* arr, int start, int count) {
    if (count <= 0) return 0;
    
    int sum = 0;
    for (int i = start; i < start + count; ++i) {
        sum += arr[i];
    }
    return sum / count;
}

// Pārbauda, vai tikko ir pievienota jauna malka
// Atgriež true, ja pēdējo temperatūru vidējā vērtība ir augstāka nekā iepriekšējo
bool WoodFilled(int CurrentTemp) {
    // Pavirza vēstures datus uz priekšu
    for (int i = 9; i > 0; --i) {
        TempHist[i] = TempHist[i - 1];
    }
    TempHist[0] = CurrentTemp;

    // Aprēķina jaunāko un vecāko mērījumu vidējās vērtības
    int recent_avg = average(TempHist, 0, 5);
    int older_avg = average(TempHist, 5, 5);

    // Ja jaunākās temperatūras ir augstākas, tad malka ir papildināta
    return recent_avg > older_avg;
}

void ieietDeepSleepArTouch() {
    touchSleepWakeUpEnable (2, thresholds);
    delay(500);
    esp_deep_sleep_start();
}

#include "lvgl_display.h" // Pievienojam, lai varētu piekļūt is_manual_damper_mode() funkcijai

void damperControlLoop() {
    int oldDamperValue = damper;
    String oldMessage = messageDamp;
    
    // Izvadam pasreizejo temperaturu un minimalo temperaturu ik pec 30 sekundem
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 30000) {
        Telnet.print("DEBUG: Pasreizeja temperatura: ");
        Telnet.print(temperature);
        Telnet.print(" C, Minimala temperatura: ");
        Telnet.print(temperatureMin);
        Telnet.print(" C, lowTempCheckActive: ");
        Telnet.println(lowTempCheckActive ? "true" : "false");
        lastDebugTime = millis();
    }
    
    // JAUNS: Pārbaudām vai esam manuālajā režīmā
    if (is_manual_damper_mode()) {
        // Manuālajā režīmā damper vērtība tiek uzstādīta no UI roller,
        // tāpēc šeit neveicam nekādas izmaiņas
        messageDamp = "MANUAL";
    } 
    else if (errI < endTrigger) {
        // PID regulators - TIKAI ja nav zemas temperatūras režīms
        if (!lowTempCheckActive) {
            errP = targetTempC - temperature;
            errI = errI + errP;
            errD = errP - errOld;
            errOld = errP;
        }
        
        // Pārbauda, vai ir pievienota jauna malka
        bool woodAdded = WoodFilled(temperature);
        
        // Ja ir pievienota jauna malka, atiestatām integrālo kļūdu
        if (woodAdded) {
            errI = 0;
        }
        
        // Uzlabota kontroles loģika ar precīziem nosacījumiem:
        // - Ja temperatūra ir vienāda vai lielāka par mērķi (targetTempC), damper = 0 (pilnībā aizvērts)
        // - Ja temperatūra ir mazāka par minimālo (temperatureMin), damper = 0 (pilnībā aizvērts)
        // - PID aprēķins tiek veikts TIKAI diapazonā: temperatureMin < temperature < targetTempC
        
        if (temperature >= targetTempC) {
            // Temperatūra ir virs vai vienāda ar mērķi - pilnībā aizveram damper
            damper = minDamper; // 0%
            messageDamp = "AUTO"; // Statuss, kas norāda, ka sasniegta max temperatūra
            // Nepārtraucam zemas temperatūras pārbaudi šeit - tas tiek darīts tikai optimālajā diapazonā
        } 
        else if (temperature <= temperatureMin) {
            // Temperatūra ir zem vai vienāda ar minimālo - pilnībā atveram damper
            damper = maxDamper; // 100%
            messageDamp = "AUTO";
            
            // Aktivizējam 4 minūšu pārbaudi, ja tā vēl nav aktīva
            if (!lowTempCheckActive) {
                lowTempStartTime = millis();
                initialTemperature = temperature; // Saglabājam sākotnējo temperatūru
                lowTempCheckActive = true;
                
                // Sakam jaunu rindu un pievienojam laiku, lai butu redzams, kad tiesi zinojums paradijas
                Telnet.println("");
                Telnet.println("*****************************************************************");
                Telnet.print("AKTIVIZETS [");
                Telnet.print(millis()/1000);
                Telnet.println(" s]: Zemas temperaturas parbaudes rezims");
                Telnet.println("INFO: Zemas temperaturas parbaude sakta. Sakuma temperatura: " + String(temperature) + " C");
                Telnet.println("INFO: Ja 4 minutu laika temperatura nepaaugstinasies par 3 C, ESP paries deep sleep rezima");
                Telnet.println("*****************************************************************");
                
                // Parbaudam, vai varam ari izvadit uz Telneto portu (diagnostikai)
                Telnet.println("INFO: Zemas temperaturas parbaude sakta. Sakuma temperatura: " + String(temperature) + " C");
            } 
            // Ja ir pagājušas 4 minūtes un temperatūra NAV palielinājusies vismaz par 3 grādiem
            else if (millis() - lowTempStartTime > LOW_TEMP_TIMEOUT && temperature < (initialTemperature + 3)) {
                damper = minDamper; // Iestatām damper uz aizvērtu pozīciju
                messageDamp = "END!";
                display_manager_notify_damper_changed();
                
                // Telnet zinojums par deep sleep sagatavosanu
                Telnet.println("BRIDINAJUMS: 4 minutes pagajusas, bet temperatura nav paaugstinajusies par 3 C");
                Telnet.println("Sakotneja temp: " + String(initialTemperature) + " C, Pasreizeja temp: " + String(temperature) + " C");
                Telnet.println("Gatavojamies pariet deep sleep rezima...");
                
                // Gaidām līdz servo beidz kustību
                if (!servoMoving) {
                    // Telnet zinojums par deep sleep pareju
                    Telnet.println("INFORMACIJA: Servo kustiba pabeigta. Damper pozicija: " + String(currentDamper) + "%");
                    Telnet.println("BRIDINAJUMS: Temperatura nav pieaugusi pietiekami. Parejam deep sleep rezima pec 0.5s");
                    delay(1500); // Ļaujam lietotājam redzēt ziņojumu
                    ieietDeepSleepArTouch(); // Aizejam dziļajā miegā
                }
            }
            // Ja temperatūra ir paaugstinājusies vismaz par 3 grādiem, atceļam zemas temperatūras pārbaudi
            else if (temperature >= (initialTemperature + 3)) {
                // Telnet zinojums par veiksmīgu temperaturas paaugstinasanos
                Telnet.println("");
                Telnet.println("*****************************************************************");
                Telnet.print("ATCELTS [");
                Telnet.print(millis()/1000);
                Telnet.println(" s]: Zemas temperaturas parbaudes rezims");
                Telnet.println("INFORMACIJA: Temperatura veiksmigi paaugstinajusies par 3 C vai vairak!");
                Telnet.println("Sakotneja temp: " + String(initialTemperature) + " C, Pasreizeja temp: " + String(temperature) + " C");
                Telnet.println("Zemas temperaturas parbaude atcelta. Krasns darbojas normali.");
                Telnet.println("*****************************************************************");
                
                // Atcelam zemas temperaturas parbaudi
                lowTempCheckActive = false;
                
                // Dublejam zinojumu Telnetaja porta
                Telnet.println("INFORMACIJA: Temperatura veiksmigi paaugstinajusies. Parbaude atcelta.");
            }
        } 
        else {
            // PID aprēķins TIKAI ja nav zemas temperatūras režīms
            if (!lowTempCheckActive) {
                // Šeit aprēķinam optimālo damper vērtību ar PID algoritmu
                damper = kP * errP + kI * errI + kD * errD;
                Telnet.println("Damper apreikina erP: " +String(errI));
                // Ierobežojam vērtību noteiktajā diapazonā
                damper = constrain(damper, minDamper, maxDamper);
                messageDamp = "AUTO"; // Normāls automātiskais režīms
            }
        }
        
        // Papildu statusa ziņojuma atjaunināšana, ja nepieciešams papildināt malku
        // Šis pārraksta iepriekš iestatītos statusus, ja integrālā kļūda ir pārāk liela
        if (errI > refillTrigger) { 
            messageDamp = "FILL!";
        }
    } else {
        // Sistēma ir beigusi darboties (sasniegta maksimālā integrālā kļūda)
        if (temperature < temperatureMin) {
            damper = zeroDamper;
            messageDamp = "END!";
            
            // Paziņojam par izmaiņām un ieslēdzam deep sleep
            if (damper != oldDamperValue) {
                display_manager_notify_damper_position_changed();
            }
            
            if (messageDamp != oldMessage) {
                display_manager_notify_damper_changed();
            }
            
            delay(1500);
            ieietDeepSleepArTouch(); // Aizejam dziļajā miegā
        }
    }
    
    // Nodrošinam, ka integrālā kļūda nav negatīva
    if (errI < 0) {
        errI = 0;
    }
    
    // Paziņojam par izmaiņām tikai ja tādas ir notikušas
    if (damper != oldDamperValue) {
        display_manager_notify_damper_position_changed();
    }
    
    if (messageDamp != oldMessage) {
        display_manager_notify_damper_changed();
    }
}

/**
 * Iestata jaunu mērķa pozīciju servo motoram
 * @param newTarget jaunā pozīcija (0-100%)
 */
void setDamperTarget(int newTarget) {
    // Mainām mērķi tikai ja tas ir atšķirīgs no pašreizējā
    if (newTarget != targetDamper) {
        targetDamper = newTarget;
        servoMoving = true;
        
        // Pieslēdzam servo tikai tad, kad tas ir nepieciešams
        if (!servoAttached) {
            mansServo.attach(servoPort);
            servoAttached = true;
        }
    }
}

/**
 * Servo motora kustības apstrādes funkcija
 * Pakāpeniski kustina servo uz mērķa pozīciju
 */
void moveServoToDamper() {
    // Ja ir jauns mērķis un servo nav kustībā, sākam jaunu kustību
    if ((damper != targetDamper) && !servoMoving && (currentDamper != damper)) {
        // Telnet zinojums par servo kustibas sakumu
        Telnet.println("INFO: Sakam servo kustibu no " + String(currentDamper) + "% uz " + String(damper) + "% poziciju");
        setDamperTarget(damper);
    }
    
    // Kustinām servo, ievērojot soļu intervālu
    if (servoMoving && (xTaskGetTickCount() - lastMoveTime > pdMS_TO_TICKS(servoStepInterval))) {
        int diff = targetDamper - currentDamper;
        
        if (abs(diff) > 0) {
            // Vienmērīga kustība ar vienu soli vienā reizē
            currentDamper += (diff > 0) ? 1 : -1;
            
            // Pārveidojam procentuālo pozīciju par leņķi
            int angle = map(currentDamper, 0, 100, 
                           servoOffset, 
                           servoOffset + servoAngle / servoCalibration);
            
            // Pārveidojam leņķi mikrosekundēs
            int us = map(angle, minAngle, maxAngle, minUs, maxUs);
            
            // Iestatām servo pozīciju
            mansServo.writeMicroseconds(us);
            lastMoveTime = xTaskGetTickCount();
        } else {
            // Mērķis sasniegts
            servoMoving = false;
            oldDamper = currentDamper;
            display_manager_notify_damper_position_changed();
            
            // Telnet zinojums par servo kustibas pabeigsanu
            Telnet.println("INFO: Servo kustiba pabeigta. Damper pozicija: " + String(currentDamper) + "%");
            
            // Atslēdzam servo, lai taupītu enerģiju
            if (servoAttached) {
                mansServo.detach();
                servoAttached = false;
            }
        }
    }
}

/**
 * FreeRTOS uzdevuma funkcija, kas pastāvīgi apstrādā servo kustības
 * Darbojas atsevišķā kodolā ar zemu prioritāti
 */
void DamperTask(void *pvParameters) {
    while (1) {
        moveServoToDamper();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms = 100Hz refresh rate
    }
}

/**
 * Sāk servo kontroles uzdevumu atsevišķā kodolā
 * Šo funkciju vajadzētu izsaukt setup() funkcijā
 */
void startDamperControlTask() {
    // Inicializējam buzzer
    initBuzzer();
    
    // Pievienojam uzdevumu otrajam kodolam ar zemu prioritāti
    xTaskCreatePinnedToCore(
        DamperTask,          // Uzdevuma funkcija
        "DamperTask",        // Uzdevuma nosaukums
        2048,                // Steka izmērs
        NULL,                // Parametri (nav)
        1,                   // Prioritāte (zema)
        &damperTaskHandle,   // Uzdevuma rokturis
        1                    // Kodola numurs (1 = otrs kodols)
    );
    
    // Pievienojam buzzer uzdevumu otrajam kodolam
    xTaskCreatePinnedToCore(
        buzzerTask,          // Uzdevuma funkcija
        "BuzzerTask",        // Uzdevuma nosaukums
        1024,                // Steka izmērs
        NULL,                // Parametri (nav)
        1,                   // Prioritāte (zema)
        &buzzerTaskHandle,   // Uzdevuma rokturis
        1                    // Kodola numurs (1 = otrs kodols)
    );
}

/**
 * Pārbauda, vai ir pagājušas 4 minūtes ar zemu temperatūru un ieej deep sleep, ja nepieciešams
 * Šo funkciju izsauc updateTemperature() ik pēc 3 sekundēm
 */
void checkLowTempDeepSleep() {
    // Pārbaudām vai ir aktīvs zemas temperatūras režīms
    if (lowTempCheckActive) {
        // Pārbaudām, vai ir pagājušas 4 minūtes un temperatūra nav paaugstinājusies
        if (millis() - lowTempStartTime > LOW_TEMP_TIMEOUT && 
            temperature < (initialTemperature + 3) && !servoMoving) {
            // Telnet ziņojumi pirms deep sleep
            Telnet.println("");
            Telnet.println("*****************************************************************");
            Telnet.print("IZPILDAS [");
            Telnet.print(millis()/1000);
            Telnet.println(" s]: Zemas temperaturas parbaudes timeout");
            Telnet.println("BRIDINAJUMS: 4 minutes pagajusas, bet temperatura nav paaugstinajusies par 3 C");
            Telnet.println("Sakotneja temp: " + String(initialTemperature) + " C, Pasreizeja temp: " + String(temperature) + " C");
            Telnet.println("BRIDINAJUMS: Temperatura nav pieaugusi pietiekami. Parejam deep sleep rezima pec 0.5s");
            Telnet.println("*****************************************************************");
            
            // Ļaujam lietotājam redzēt ziņojumu
            delay(500);
            
            // Aizejam dziļajā miegā
            ieietDeepSleepArTouch();
        }
    }
}