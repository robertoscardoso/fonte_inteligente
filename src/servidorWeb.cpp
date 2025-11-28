#include "ServidorWeb.h"

ServidorWeb::ServidorWeb(
    int porta,
    GerenciadorDeLogs *logs,
    Bateria *bateria,
    RedeExterna *rede
) : server(porta)
{
    this->logs = logs;
    this->bateria = bateria;
    this->rede = rede;

    tensaoBateria = 0;
    tensaoRede = 0;
    porcentagem = 0;
    redeAtiva = false;
    redeAtivaAnterior = false;
    
    this->idDispositivo = ""; 
    this->apelido = "Fonte Inteligente";
}

void ServidorWeb::verificarConfiguracao(String dataHoraAtual)
{
    // 1. Carrega do arquivo se o ID estiver vazio na memória RAM
    if (idDispositivo == "") {
        if (LittleFS.exists("/config.json")) {
            File file = LittleFS.open("/config.json", "r");
            if (file) {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, file);
                if (!error) {
                    // CORREÇÃO DOS AVISOS DO ARDUINOJSON
                    if (doc["id"].is<String>()) idDispositivo = doc["id"].as<String>();
                    if (doc["apelido"].is<String>()) apelido = doc["apelido"].as<String>();
                }
                file.close();
            }
        }
    }

    // --- LÓGICA DE AUTO-CORREÇÃO ---
    // Verifica se é vazio ou se é um ID antigo gerado com data de 1969
    bool idInvalido = (idDispositivo == "") || (idDispositivo.startsWith("31121969"));
    
    if (idInvalido) {
        String idGerado = dataHoraAtual; 
        idGerado.replace("/", ""); 
        idGerado.replace(":", "");
        idGerado.replace(" ", "");
        
        // GERA APENAS 2 DÍGITOS ALEATÓRIOS (10 a 99)
        idGerado += String(random(10, 100)); 
        
        this->idDispositivo = idGerado;
        salvarConfigNoArquivo();
    }
}

// Handler para o botão de Reset
void ServidorWeb::handleResetConfig() {
    LittleFS.remove("/config.json");
    server.send(200, "text/plain", "Configuracao apagada! Reiniciando...");
    delay(1000);
    ESP.restart();
}

void ServidorWeb::salvarConfigNoArquivo() {
    JsonDocument doc;
    doc["id"] = idDispositivo;
    doc["apelido"] = apelido;
    
    File file = LittleFS.open("/config.json", "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
    }
}

void ServidorWeb::iniciar(const char *ssidAP, const char *senhaAP, const char *hostname)
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssidAP, senhaAP);

    if (!MDNS.begin(hostname)) {
        Serial.println("Erro mDNS");
    }

    server.on("/", std::bind(&ServidorWeb::handleRoot, this));
    server.on("/data", HTTP_GET, std::bind(&ServidorWeb::handleData, this));
    server.on("/historico", HTTP_GET, std::bind(&ServidorWeb::handleHistorico, this));
    server.on("/historico/new", HTTP_GET, std::bind(&ServidorWeb::handleHistoricoNew, this));
    server.on("/connect", HTTP_POST, std::bind(&ServidorWeb::handleConnect, this));
    server.on("/salvar-apelido", HTTP_POST, std::bind(&ServidorWeb::handleSalvarApelido, this));
    
    // Rota de Reset
    server.on("/reset-config", HTTP_POST, std::bind(&ServidorWeb::handleResetConfig, this));

    server.begin();
}

void ServidorWeb::atualizar()
{
    porcentagem = bateria->getPorcentagem();
    tensaoBateria = bateria->getTensao();
    tensaoRede = rede->getTensao();
    redeAtiva = (tensaoRede > 0.5);

    server.handleClient();
}

void ServidorWeb::handleSalvarApelido() {
    if (server.hasArg("apelido")) {
        String novoApelido = server.arg("apelido");
        if (novoApelido.length() > 0) {
            this->apelido = novoApelido;
            salvarConfigNoArquivo();
            server.send(200, "text/plain", "Apelido atualizado!");
            return;
        }
    }
    server.send(400, "text/plain", "Erro");
}

void ServidorWeb::handleRoot()
{
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        server.send(404, "text/plain", "404");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void ServidorWeb::handleData()
{
    JsonDocument doc;

    doc["tensaoRede"] = tensaoRede;
    doc["tensaoBateria"] = tensaoBateria;
    doc["porcentagemBateria"] = porcentagem;
    doc["redeAtiva"] = redeAtiva ? "REDE EXTERNA" : "BATERIA";
    doc["id"] = idDispositivo;
    doc["apelido"] = apelido;

    if (redeAtiva && porcentagem < 100)
        doc["estado"] = "Carregando";
    else if (!redeAtiva && porcentagem == 0)
        doc["estado"] = "Descarregada";
    else if (!redeAtiva)
        doc["estado"] = "Descarregando";
    else
        doc["estado"] = "Carga Completa";

    String jsonResponse;
    serializeJson(doc, jsonResponse);

    server.send(200, "application/json", jsonResponse);
}

void ServidorWeb::handleHistorico()
{
    String logsJson = logs->getLogsAsJson();
    server.send(200, "application/json", logsJson);
}

void ServidorWeb::handleHistoricoNew()
{
    int ultimoID = 0;
    if (server.hasArg("sinceID"))
        ultimoID = server.arg("sinceID").toInt();

    String logsJson = logs->getNewLogsAsJson(ultimoID);
    server.send(200, "application/json", logsJson);
}

void ServidorWeb::handleConnect()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long inicio = millis();
    bool conectado = false;

    while (millis() - inicio < 10000)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            conectado = true;
            break;
        }
        delay(300);
    }

    if (conectado)
    {
        String resposta = "Conectado: " + ssid + "\nIP: " + WiFi.localIP().toString();
        server.send(200, "text/plain", resposta);
    }
    else
    {
        server.send(200, "text/plain", "Falha conexao");
    }
}