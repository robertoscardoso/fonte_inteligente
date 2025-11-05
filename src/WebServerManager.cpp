#include "WebServerManager.h"
#include <Arduino.h>
#include "SaudeBateria.h" // Inclui a definição da classe SaudeBateria
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "GerenciadorDeLogs.h" // Inclui a definição da classe GerenciadorDeLogs

// Avisa ao compilador que estas variáveis globais existem em outros arquivos
extern GerenciadorDeLogs gerenciadorDeLogs;
extern SaudeBateria saudeBateria; 

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

    // Rota para obter o HISTÓRICO "/historico"
    server.on("/historico", HTTP_GET, [this](AsyncWebServerRequest *request){
        this->handleHistorico(request);
    });

    // Rota para ZERAR OS CICLOS
    server.on("/resetar-ciclos", HTTP_GET, [this](AsyncWebServerRequest *request){
        saudeBateria.resetarCiclos(); // Chama a função da classe SaudeBateria
        Serial.println("Contagem de ciclos zerada via web!");
        request->send(200, "text/plain", "Contagem de ciclos zerada com sucesso!");
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

    // Lê o contador de ciclos fracionário
    float ciclos = saudeBateria.getCiclosCumulativos(); //

    String json = "{";
    json += "\"percBateria\":" + String(percBateria, 1) + ",";    
    json += "\"statusRede\":\"" + statusRedeStr + "\","; 
    // Formata o float com 2 casas decimais
    json += "\"ciclos\":" + String(ciclos, 2); 
    json += "}";
    
    request->send(200, "application/json", json); 
}

// HANDLER para "/historico" - Envia JSON do BANCO DE DADOS
void WebServerManager::handleHistorico(AsyncWebServerRequest *request) {
    // Busca o JSON da classe GerenciadorDeLogs
    String jsonHistorico = gerenciadorDeLogs.getLogsAsJson(); //
    request->send(200, "application/json", jsonHistorico);
}

// Handler para página não encontrada (404)
void WebServerManager::handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "404: Pagina nao encontrada");
}