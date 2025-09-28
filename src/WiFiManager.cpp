#include "WiFiManager.h"
#include "Arduino.h"

WiFiManager::WiFiManager(){

}

bool WiFiManager::iniciarAP(const char* ssid, const char* password){
    _ssid = ssid;
    _password = password;

    Serial.print("Iniciando Acess Point...");
    
    WiFi.softAP(_ssid, _password);

    delay(5000);

    Serial.print("SSID Deu certo: ");
    Serial.println(_ssid);
    Serial.print("IP do servidor: ");
    Serial.println(getIP());

    return true;
}

IPAddress WiFiManager::getIP(){
    return WiFi.softAPIP();
}