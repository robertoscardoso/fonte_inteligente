#include <WiFi.h> 
#include <WebServer.h>
#include "WiFiManager.h" // Inclui sua classe
// Note: Você pode não precisar mais do #include <WiFi.h> aqui se
// todas as chamadas à API WiFi forem encapsuladas na sua classe.

WebServer server(80);
WiFiManager wifiManager; // 2. Instancia um objeto da sua classe

// Conteúdo da página web a ser exibida.
const char* HTML_CONTENT = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Hello World POO - ESP32</title>
</head>
<body>
    <h1>Hello, World! (Orientado a Objetos)</h1>
    <p>Servidor rodando em modo Access Point.</p>
</body>
</html>
)rawliteral";


void handlerRoot(){
    server.send(200, "text/html", HTML_CONTENT);
}

void setup(){
    Serial.begin(115200);
    delay(2000); // Dá tempo para o Serial Monitor abrir

    const char* ssid = "BORGES";
    const char* password = "gomugomu";

    // 1. Inicia o Access Point (AP) usando o objeto da sua classe
    if(wifiManager.iniciarAP(ssid, password)){ // 3. Chama o método da sua classe
        // O IP já foi impresso dentro de iniciarAP(), mas podemos reimprimi-lo:
        Serial.print("Access Point IP (via getIP()): ");
        Serial.println(wifiManager.getIP()); 

        // 2. Configura e inicia o servidor
        server.on("/", handlerRoot);
        server.begin();
        Serial.println("Servidor iniciado!");

    }else{
        Serial.println("Falha ao iniciar o wifi (softAP) via WiFiManager");
    }
}


void loop(){
    server.handleClient();
}