#include "temperature.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "damper_control.h"
#include "display_manager.h"  // Add display manager
#include "telnet.h"



// Karogs, kas norāda, ka nepieciešams veikt damper aprēķinu, kad servo beidzis kustību
static bool damperCalcPending = false;

int temperature = 0;
int targetTempC = 66;
int maxTemp = 80; // Maximum temperature for the target
int temperatureMin = 46; // Minimum temperature to avoid negative values
int temperaturemini2 = 64; // Maximum temperature for the sensor
int warningTemperature = 85; // Brīdinājuma temperatūras slieksnis
unsigned long tempReadIntervalMs = 4000; // Noklusējuma vērtība 4 sekundes (4000ms)
static OneWire oneWire(6);
static DallasTemperature ds(&oneWire);
static DeviceAddress sensor1 = {0x28, 0xFC, 0x70, 0x96, 0xF0, 0x01, 0x3C, 0xC0};

static unsigned long lastTempRead = 0;
static unsigned long tempRequestTime = 0;
static bool tempRequested = false;

// Change detection variables
static int lastDisplayedTemperature = -999;  // Force first update
static unsigned long lastChangeTime = 0;
static bool temperatureChanged = false;

void initTemperatureSensor() {
    ds.begin();
    ds.setResolution(sensor1, 9);
}

void updateTemperature() {
    if (!tempRequested && millis() - lastTempRead >= tempReadIntervalMs) {
        ds.requestTemperaturesByAddress(sensor1);
        tempRequestTime = millis();
        tempRequested = true;
    }

    if (tempRequested && millis() - tempRequestTime >= 100) {
        int newTemperature = ds.getTempC(sensor1);
        if (newTemperature < 0) {
            newTemperature = 5;
        }
        
        // Atjauninām temperatūru vienmēr
        temperature = newTemperature;
        
        // Check if temperature actually changed
        if (newTemperature != lastDisplayedTemperature) {
            lastDisplayedTemperature = temperature;
            lastChangeTime = millis();
            temperatureChanged = true;
            
            // Notify display manager that temperature changed
            display_manager_notify_temperature_changed();
            
            // Ja temperatūra mainījusies un servo nav kustībā, izpildām damper aprēķinu uzreiz
            if (!servoMoving) {
                damperControlLoop();
                
                // Uzlabots Telnet izvads ar skaidraku informaciju
                Telnet.print("TEMP_CHANGED - Temp: ");
                Telnet.print(temperature);
                Telnet.print("°C | Target: ");
                Telnet.print(targetTempC);
                Telnet.print("°C | Min: ");
                Telnet.print(temperatureMin);
                Telnet.print("°C | Mode: ");
                Telnet.print(messageDamp);
                Telnet.print(" | Damper: ");
                Telnet.print(damper);
                Telnet.print("%");
                Telnet.print(" | Read Interval: ");
                Telnet.print(tempReadIntervalMs);
                Telnet.print("ms");


                // Paradam aprekinu tikai ja tas ir veikts (ja temperature ir starp min un target)
                if (temperature > temperatureMin && temperature < targetTempC) {
                    Telnet.print(" = kP(");
                    Telnet.print(kP);
                    Telnet.print(") * errP(");
                    Telnet.print(errP);
                    Telnet.print(") + kI(");
                    Telnet.print(kI, 5);
                    Telnet.print(") * errI(");
                    Telnet.print(errI);
                    Telnet.print(") + kD(");
                    Telnet.print(kD);
                    Telnet.print(") * errD(");
                    Telnet.print(errD);
                    Telnet.print(")");
                }
                Telnet.println("");
                
                damperCalcPending = false; // Atiestatām gaidīšanas karogu, jo aprēķins ir veikts
            } else {
                // Ja servo ir kustībā, atzīmējam, ka aprēķins ir nepieciešams
                damperCalcPending = true;
            }
        }
        
        lastTempRead = millis();
        tempRequested = false;
    }
    
    // ĀRPUS if bloka: Ja ir zemas temperatūras režīms, izpildam damper aprēķinu ik pēc 30 sekundēm
    static unsigned long lastLowTempUpdate = 0;
    if (lowTempCheckActive && millis() - lastLowTempUpdate >= 30000) {
        damperControlLoop();
        
        Telnet.print("LOW_TEMP_30SEC - Temp: ");
        Telnet.print(temperature);
        Telnet.print("C | Target: ");
        Telnet.print(targetTempC);
        Telnet.print("C | Min: ");
        Telnet.print(temperatureMin);
        Telnet.print("C | Mode: ");
        Telnet.print(messageDamp);
        Telnet.print(" | Damper: ");
        Telnet.print(damper);
        Telnet.print("%");
        Telnet.print(" | Read Interval: ");
        Telnet.print(tempReadIntervalMs);
        Telnet.print("ms");
        
        if (temperature > temperatureMin && temperature < targetTempC) {
            Telnet.print(" = kP(");
            Telnet.print(kP);
            Telnet.print(") * errP(");
            Telnet.print(errP);
            Telnet.print(") + kI(");
            Telnet.print(kI, 5);
            Telnet.print(") * errI(");
            Telnet.print(errI);
            Telnet.print(") + kD(");
            Telnet.print(kD);
            Telnet.print(") * errD(");
            Telnet.print(errD);
            Telnet.print(")");
        }
        Telnet.println("");
        
        lastLowTempUpdate = millis(); // Atiestatām 30 sekunžu timeri
    }
    
    // Apstrādām pending damper aprēķinus (kad servo beidz kustību)
    if (damperCalcPending && !servoMoving) {
        damperControlLoop();
        
        Telnet.print("SERVO_FINISHED - Temp: ");
        Telnet.print(temperature);
        Telnet.print("°C | Target: ");
        Telnet.print(targetTempC);
        Telnet.print("°C | Min: ");
        Telnet.print(temperatureMin);
        Telnet.print("°C | Mode: ");
        Telnet.print(messageDamp);
        Telnet.print(" | Damper: ");
        Telnet.print(damper);
        Telnet.print("%");
        
        if (temperature > temperatureMin && temperature < targetTempC) {
            Telnet.print(" = kP(");
            Telnet.print(kP);
            Telnet.print(") * errP(");
            Telnet.print(errP);
            Telnet.print(") + kI(");
            Telnet.print(kI, 5);
            Telnet.print(") * errI(");
            Telnet.print(errI);
            Telnet.print(") + kD(");
            Telnet.print(kD);
            Telnet.print(") * errD(");
            Telnet.print(errD);
            Telnet.print(")");
        }
        Telnet.println("");
        
        damperCalcPending = false; // Atiestatām gaidīšanas karogu
    }

}

// New function to check if temperature has changed
bool hasTemperatureChanged() {
    if (temperatureChanged) {
        temperatureChanged = false;  // Reset flag
        return true;
    }
    return false;
}

// Get time since last temperature change
unsigned long getTimeSinceLastTempChange() {
    return millis() - lastChangeTime;
}