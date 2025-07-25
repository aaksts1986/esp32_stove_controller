#ifndef TELNET_H
#define TELNET_H

#include <Arduino.h>
#include <WiFi.h>

#define MAX_TELNET_CLIENTS 2

class TelnetClass {
public:
    // Inicializācija
    void begin(uint16_t port = 23);
    
    // Telnet servera apstrāde
    void handle();
    
    // Datu rakstīšana uz telnet klientiem
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    
    // Print metodes
    size_t print(const String &s);
    size_t print(const char* s);
    size_t print(char c);
    size_t print(int n, int base = DEC);
    size_t print(unsigned int n, int base = DEC);
    size_t print(long n, int base = DEC);
    size_t print(unsigned long n, int base = DEC);
    size_t print(double n, int digits = 2);
    
    // Println metodes
    size_t println(const String &s);
    size_t println(const char* s);
    size_t println(char c);
    size_t println(int n, int base = DEC);
    size_t println(unsigned int n, int base = DEC);
    size_t println(long n, int base = DEC);
    size_t println(unsigned long n, int base = DEC);
    size_t println(double n, int digits = 2);
    size_t println(void);

private:
    WiFiServer server;
    WiFiClient clients[MAX_TELNET_CLIENTS];
    bool isConnected[MAX_TELNET_CLIENTS];
};

extern TelnetClass Telnet;

#endif // TELNET_H
