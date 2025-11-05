#include "WebServerManager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "GerenciadorDeLogs.h" // 1. Inclui a definição da classe

// 2. Avisa ao compilador que esta variável global existe em outro arquivo
extern GerenciadorDeLogs gerenciadorDeLogs;

// Construtor
WebServerManager::WebServerManager(Energia &rede, Bateria &bat) : 
    _redeExterna(rede), 
    _bateria(bat),
    server(80) 
{}

// Método para configurar e iniciar o servidor
void WebServerManager::iniciar() {
    // Inicializa o LittleFS
    if(!LittleFS.begin(false)){
        Serial.println("❌ Erro ao montar o LittleFS!");
        return; 
    }
    Serial.println("✅ LittleFS montado com sucesso.");

    // --- Configura as Rotas (URLs) ---

    // Rota para a página principal "/" - Envia o index.html
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if(LittleFS.exists("/index.html")){
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "Erro: index.html nao encontrado.");
        }
    });

    // Rota para o arquivo JavaScript "/script.js"
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        if(LittleFS.exists("/script.js")){
            request->send(LittleFS, "/script.js", "text/javascript");
        } else {
            request->send(404, "text/plain", "Erro: script.js nao encontrado.");
        }
    });

    // Rota para o arquivo Gauge.js "/gauge.min.js"
    server.on("/gauge.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
        if(LittleFS.exists("/gauge.min.js")){
            request->send(LittleFS, "/gauge.min.js", "text/javascript");
        } else {
            request->send(404, "text/plain", "Erro: gauge.min.js nao encontrado.");
        }
    });
    
    // Rota para obter os dados em TEMPO REAL "/dados"
    server.on("/dados", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleDados(request); 
    });

    // NOVA ROTA para obter o HISTÓRICO "/historico"
    server.on("/historico", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleHistorico(request);
    });

    // Rota para páginas não encontradas (404)
    server.onNotFound([this](AsyncWebServerRequest *request){
        this->handleNotFound(request);
    });

    // --- Inicia o Servidor ---
    server.begin();

    Serial.println("✅ Servidor Web iniciado!");
    Serial.print("   Acesse em: http://");
    Serial.println(WiFi.localIP()); 
}

// --- Implementação dos Handlers (Métodos que respondem às rotas) ---

// (handleRoot não é mais necessário pois enviamos o arquivo)
void WebServerManager::handleRoot(AsyncWebServerRequest *request) {}

// Handler para "/dados" - Envia JSON em TEMPO REAL
void WebServerManager::handleDados(AsyncWebServerRequest *request) {
    float tensaoRede = _redeExterna.getTensao();
    float percBateria = _bateria.getPorcentagem();
    bool redeAtiva = (tensaoRede > 1.0); 
    String statusRedeStr = redeAtiva ? "ATIVADA" : "DESATIVADA";

    String json = "{";
    json += "\"percBateria\":" + String(percBateria, 1) + ",";    
    json += "\"statusRede\":\"" + statusRedeStr + "\""; 
    json += "}";
    
    request->send(200, "application/json", json); 
}

// NOVO HANDLER para "/historico" - Envia JSON do BANCO DE DADOS
void WebServerManager::handleHistorico(AsyncWebServerRequest *request) {
    // Busca o JSON da classe GerenciadorDeLogs (passada no main.cpp)
    // Nota: Esta chamada pode ser lenta se o DB for grande.
    String jsonHistorico = gerenciadorDeLogs.getLogsAsJson(); 
    request->send(200, "application/json", jsonHistorico);
}

// Handler para página não encontrada (404)
void WebServerManager::handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404: Pagina nao encontrada");
}