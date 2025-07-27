#include "telnet.h"

// Telnet instance
TelnetClass Telnet;

void TelnetClass::begin(uint16_t port) {
    // Inicializējam telnet serveri
    server = WiFiServer(port);
    server.begin();
    server.setNoDelay(true);
    
    // Inicializējam klientu masīvu
    for (uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++) {
        isConnected[i] = false;
    }
}

void TelnetClass::handle() {
    // Pārbaudam, vai ir jauni klienti
    if (server.hasClient()) {
        // Meklējam brīvu vietu klientam
        uint8_t i;
        for (i = 0; i < MAX_TELNET_CLIENTS; i++) {
            if (!clients[i] || !clients[i].connected()) {
                if (clients[i]) {
                    clients[i].stop();
                }
                
                // Pievienojam jauno klientu
                clients[i] = server.available();
                clients[i].flush();
                isConnected[i] = true;
                
                // Sutam sveiciena zinojumu
                clients[i].println("Savienojums ar ESP32 telnet serveri izveidots.");
                clients[i].println("Ievadiet 'help' lai redzetu pieejamas komandas.");
                clients[i].println("----------------------------------------------");
                break;
            }
        }            // Nav brivu vietu, noraidam klientu
            if (i >= MAX_TELNET_CLIENTS) {
                WiFiClient client = server.available();
                client.println("Parak daudz klientu! Meginiet velak.");
                client.stop();
            }
    }
    
    // Apstrādājam klientu datus
    for (uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++) {
        if (clients[i] && clients[i].connected() && clients[i].available()) {
            // Lasām klienta komandu
            String command = clients[i].readStringUntil('\n');
            command.trim();
            
            // Apstradajam komandas
            if (command == "help") {
                clients[i].println("Pieejamas komandas:");
                clients[i].println("  help - Parada so palidzibu");
                clients[i].println("  info - Parada sistemas informaciju");
                clients[i].println("  exit - Aizver savienojumu");
                clients[i].println("  reset - Restarte ESP32");
            } 
            else if (command == "info") {
                clients[i].println("ESP32 sistemas informacija:");
                clients[i].print("  IP adrese: ");
                clients[i].println(WiFi.localIP());
                clients[i].print("  MAC adrese: ");
                clients[i].println(WiFi.macAddress());
                clients[i].print("  RSSI: ");
                clients[i].println(WiFi.RSSI());
                clients[i].print("  Briva atmina: ");
                clients[i].println(ESP.getFreeHeap());
            }
            else if (command == "exit") {
                clients[i].println("Atvienojamies...");
                clients[i].stop();
            }
            else if (command == "reset") {
                clients[i].println("Restartejam ESP32...");
                delay(500);
                ESP.restart();
            }
            else if (command.length() > 0) {
                clients[i].print("Nezinama komanda: ");
                clients[i].println(command);
            }
        }
    }
    
    // Pārbaudam atvienotos klientus
    for (uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++) {
        if (isConnected[i] && !clients[i].connected()) {
            clients[i].stop();
            isConnected[i] = false;
        }
    }
}

// Write metodes
size_t TelnetClass::write(uint8_t c) {
    size_t n = 0;
    for (uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            n += clients[i].write(c);
        }
    }
    return n;
}

size_t TelnetClass::write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    for (uint8_t i = 0; i < MAX_TELNET_CLIENTS; i++) {
        if (clients[i] && clients[i].connected()) {
            n += clients[i].write(buffer, size);
        }
    }
    return n;
}

// Print metodes
size_t TelnetClass::print(const String &s) {
    return write((const uint8_t*)s.c_str(), s.length());
}

size_t TelnetClass::print(const char* s) {
    return write((const uint8_t*)s, strlen(s));
}

size_t TelnetClass::print(char c) {
    return write((uint8_t)c);
}

size_t TelnetClass::print(int n, int base) {
    char buf[8 * sizeof(n) + 1]; // Pieņemot nejaukākais gadījums binārā formātā
    itoa(n, buf, base);
    return print(buf);
}

size_t TelnetClass::print(unsigned int n, int base) {
    char buf[8 * sizeof(n) + 1]; // Pieņemot nejaukākais gadījums binārā formātā
    utoa(n, buf, base);
    return print(buf);
}

size_t TelnetClass::print(long n, int base) {
    char buf[8 * sizeof(n) + 1]; // Pieņemot nejaukākais gadījums binārā formātā
    ltoa(n, buf, base);
    return print(buf);
}

size_t TelnetClass::print(unsigned long n, int base) {
    char buf[8 * sizeof(n) + 1]; // Pieņemot nejaukākais gadījums binārā formātā
    ultoa(n, buf, base);
    return print(buf);
}

size_t TelnetClass::print(double n, int digits) {
    char buf[20];
    sprintf(buf, "%.*f", digits, n);
    return print(buf);
}

// Println metodes
size_t TelnetClass::println(const String &s) {
    size_t n = print(s);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(const char* s) {
    size_t n = print(s);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(char c) {
    size_t n = print(c);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(int num, int base) {
    size_t n = print(num, base);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(unsigned int num, int base) {
    size_t n = print(num, base);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(long num, int base) {
    size_t n = print(num, base);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(unsigned long num, int base) {
    size_t n = print(num, base);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(double num, int digits) {
    size_t n = print(num, digits);
    n += print("\r\n");
    return n;
}

size_t TelnetClass::println(void) {
    return print("\r\n");
}
