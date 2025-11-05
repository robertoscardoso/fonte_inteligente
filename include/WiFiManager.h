#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <Wifi.h>

class WiFiManager{
    public:
    WiFiManager();

    bool iniciarAP(const char* ssid, const char* password);
    bool conectar(const char* ssid, const char* password);

    IPAddress getIP();


    private:
    const char* _ssid;
    const char* _password;
};




#endif 
