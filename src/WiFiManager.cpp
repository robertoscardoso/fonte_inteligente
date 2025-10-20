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

bool WiFiManager::conectar(const char* ssid, const char* password){ // Nome melhor
    // 1. Tenta Conectar ao Wi-Fi Externo (Station Mode)
    Serial.print("Tentando conectar a rede: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password); // ESSENCIAL: Inicia a conexão de cliente (Station)

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        delay(500);
        Serial.print(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado com sucesso!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("\nFalha ao conectar.");
        return false;
    }
}

IPAddress WiFiManager::getIP(){
    return WiFi.softAPIP();
}