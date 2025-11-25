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
}

// ====================================================================
//  INICIAR SERVIDOR
// ====================================================================
void ServidorWeb::iniciar(const char *ssidAP, const char *senhaAP, const char *hostname)
{
    // Configura modo AP
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssidAP, senhaAP);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP iniciado. IP: ");
    Serial.println(IP);

    // mDNS
    if (!MDNS.begin(hostname))
    {
        Serial.println("Erro ao iniciar mDNS!");
    }

    // Rotas do servidor
    server.on("/", std::bind(&ServidorWeb::handleRoot, this));
    server.on("/data", HTTP_GET, std::bind(&ServidorWeb::handleData, this));
    server.on("/historico", HTTP_GET, std::bind(&ServidorWeb::handleHistorico, this));
    server.on("/historico/new", HTTP_GET, std::bind(&ServidorWeb::handleHistoricoNew, this));
    server.on("/connect", HTTP_POST, std::bind(&ServidorWeb::handleConnect, this));

    server.begin();
    Serial.println("Servidor HTTP iniciado.");
}

// ====================================================================
//  LOOP / ATUALIZAÇÃO
// ====================================================================
void ServidorWeb::atualizar()
{
    // Atualiza os valores lidos
    porcentagem = bateria->getPorcentagem();
    tensaoBateria = bateria->getTensao();
    tensaoRede = rede->getTensao();

    redeAtiva = (tensaoRede > 0.5);

    // Trata requisições
    server.handleClient();
}

// ====================================================================
//  HANDLER: Página principal
// ====================================================================
void ServidorWeb::handleRoot()
{
    File file = LittleFS.open("/index.html", "r");

    if (!file)
    {
        Serial.println("Falha ao abrir index.html");
        server.send(404, "text/plain", "404: Arquivo nao encontrado.");
        return;
    }

    server.streamFile(file, "text/html");
    file.close();
}

// ====================================================================
//  HANDLER: /data
// ====================================================================
void ServidorWeb::handleData()
{
    JsonDocument doc;

    doc["tensaoRede"] = tensaoRede;
    doc["tensaoBateria"] = tensaoBateria;
    doc["porcentagemBateria"] = porcentagem;
    doc["redeAtiva"] = redeAtiva ? "REDE EXTERNA" : "BATERIA";

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

// ====================================================================
//  HANDLER: /historico
// ====================================================================
void ServidorWeb::handleHistorico()
{
    Serial.println("Requisição recebida em /historico");

    String logsJson = logs->getLogsAsJson();

    server.send(200, "application/json", logsJson);
}

// ====================================================================
//  HANDLER: /historico/new
// ====================================================================
void ServidorWeb::handleHistoricoNew()
{
    Serial.println("Requisição recebida em /historico/new");

    int ultimoID = 0;

    if (server.hasArg("sinceID"))
        ultimoID = server.arg("sinceID").toInt();

    String logsJson = logs->getNewLogsAsJson(ultimoID);

    server.send(200, "application/json", logsJson);
}

// ====================================================================
//  HANDLER: /connect (recebe SSID e senha)
// ====================================================================
void ServidorWeb::handleConnect()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    Serial.println("\n=== Requisição em /connect ===");
    Serial.println("SSID: " + ssid);
    Serial.println("Senha: " + password);

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
        Serial.print(".");
    }
    Serial.println();

    if (conectado)
    {
        Serial.println("Conectado ao WiFi!");
        String resposta = "Conectado com sucesso a " + ssid +
                          "\nIP: " + WiFi.localIP().toString();
        server.send(200, "text/plain", resposta);
    }
    else
    {
        Serial.println("Falha na conexão.");
        server.send(200, "text/plain", "Falha ao conectar à rede.");
    }
}
