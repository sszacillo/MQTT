#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>

class WiFiManager {
public:
    WiFiManager(const char* ssid, const char* password);
    void connect();

private:
    const char* ssid;
    const char* password;
};

#endif
