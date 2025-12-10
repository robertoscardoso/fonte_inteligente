#include "ServidorWeb.h"

const char *SERVER_URL_COMPLETA = "http://auere.com.br/ledax/receber_log_mysqli.php";
const char *CONFIG_URL_COMPLETA = "http://auere.com.br/ledax/receber_fonte_mysqli.php";

ServidorWeb::ServidorWeb(
    int porta,
    GerenciadorDeLogs *logs,
    Bateria *bateria,
    RedeExterna *rede) : server(porta)
{
    this->logs = logs;
    this->bateria = bateria;
    this->rede = rede;
    this->idDispositivo = "";
    this->apelido = "Fonte Inteligente";
    this->ssidSta = "";
    this->passwordSta = "";

    tensaoBateria = 0;
    tensaoRede = 0;
    porcentagem = 0;
    redeAtiva = false;
    redeAtivaAnterior = false;
}

void ServidorWeb::verificarConfiguracao(String dataHoraAtual, long EpochTime)
{
    bool idInvalido = (idDispositivo == "");
    // 1. Carrega do arquivo se o ID estiver vazio na memória RAM
    if (idInvalido)
    {
        if (LittleFS.exists("/config.json"))
        {
            File file = LittleFS.open("/config.json", "r");
            if (file)
            {
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, file);
                if (!error)
                {
                    // CORREÇÃO DOS AVISOS DO ARDUINOJSON
                    if (doc["id"].is<String>())
                        idDispositivo = doc["id"].as<String>();
                    if (doc["apelido"].is<String>())
                        apelido = doc["apelido"].as<String>();
                }
                file.close();
            }
        }
    }
    idInvalido = (idDispositivo == "");

    if (idInvalido && EpochTime > 1704067200)
    {
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
void ServidorWeb::handleResetConfig()
{
    LittleFS.remove("/config.json");
    server.send(200, "text/plain", "Configuracao apagada! Reiniciando...");
    delay(1000);
    ESP.restart();
}

void ServidorWeb::salvarConfigNoArquivo()
{
    JsonDocument doc;
    doc["id"] = idDispositivo;
    doc["apelido"] = apelido;
    // SALVAR DADOS DA REDE
    doc["ssid"] = ssidSta;
    doc["pass"] = passwordSta;

    File file = LittleFS.open("/config.json", "w");
    if (file)
    {
        serializeJson(doc, file);
        file.close();
    }
}

void ServidorWeb::iniciar(const char *ssidAP, const char *senhaAP, const char *hostname)
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssidAP, senhaAP);

    if (!MDNS.begin(hostname))
    {
        Serial.println("Erro mDNS");
    }

    if (LittleFS.exists("/config.json"))
    {
        File file = LittleFS.open("/config.json", "r");
        if (file)
        {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, file);
            if (!error)
            {
                // CORREÇÃO DOS AVISOS DO ARDUINOJSON
                if (doc["id"].is<String>())
                    idDispositivo = doc["id"].as<String>();
                if (doc["apelido"].is<String>())
                    apelido = doc["apelido"].as<String>();

                // CARREGAR DADOS DA REDE
                if (doc["ssid"].is<String>())
                    ssidSta = doc["ssid"].as<String>();
                if (doc["pass"].is<String>())
                    passwordSta = doc["pass"].as<String>();
            }
            file.close();
        }
    }
    // TENTATIVA DE CONEXÃO AUTOMÁTICA
    if (ssidSta.length() > 0) {
        WiFi.begin(ssidSta.c_str(), passwordSta.c_str());
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
    redeAtiva = (tensaoRede > 4.0);

    server.handleClient();
}

void ServidorWeb::handleSalvarApelido()
{
    if (server.hasArg("apelido"))
    {
        String novoApelido = server.arg("apelido");
        if (novoApelido.length() > 0)
        {
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
    if (!file)
    {
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
        // ATUALIZA E SALVA SE CONECTOU COM SUCESSO
        this->ssidSta = ssid;
        this->passwordSta = password;
        salvarConfigNoArquivo();
        
        String resposta = "Conectado: " + ssid + "\nIP: " + WiFi.localIP().toString();
        server.send(200, "text/plain", resposta);
    }
    else
    {
        server.send(200, "text/plain", "Falha conexao");
    }
}

void ServidorWeb::enviarLogParaServidor(String data_hora, String tipo, float percentual_bateria)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("ERRO: WiFi não conectado, ignorando envio de log externo.");
        return;
    }

    HTTPClient http;

    // Obter o ID do dispositivo
    String serie_number = this->idDispositivo;

    // --- 1. Criar o Payload JSON ---
    // Use o tamanho adequado para seu JsonDocument (ex: 200 bytes)
    const size_t capacity = JSON_OBJECT_SIZE(4);
    JsonDocument doc;

    doc["serie"] = serie_number;
    doc["data_hora"] = data_hora;
    doc["tipo"] = tipo;
    doc["percentual"] = percentual_bateria;

    String jsonPayload;
    // Serializar o JSON para a string que será enviada
    serializeJson(doc, jsonPayload);
    // ---------------------------------

    // --- OBTENDO A URL:
    const char *urlParaEnvio = SERVER_URL_COMPLETA;

    // Inicializa a requisição usando a URL completa
    http.begin(urlParaEnvio);

    // --- 2. Configurar o Header para JSON ---
    http.addHeader("Content-Type", "application/json");

    // --- 3. Enviar a requisição POST com o Payload ---
    int httpCode = http.POST(jsonPayload);

    if (httpCode > 0)
    {
        // httpCode 201 (Created) ou 200 (OK) indica sucesso.
        Serial.printf("ServidorWeb: Log Enviado (Code %d). Resposta: %s\n", httpCode, http.getString().c_str());
    }
    else
    {
        Serial.printf("ServidorWeb: Falha no Envio HTTP: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

void ServidorWeb::enviarConfiguracaoParaServidor()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("ERRO: WiFi não conectado, ignorando envio de configuração externa.");
        return;
    }

    HTTPClient http;

    // Obter os dados da RAM da classe
    String serie_number = this->idDispositivo;
    String apelido_dispositivo = this->apelido;

    const size_t capacity = JSON_OBJECT_SIZE(2);
    JsonDocument doc;

    doc["serie"] = serie_number;
    doc["apelido"] = apelido_dispositivo;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    const char *urlParaEnvio = CONFIG_URL_COMPLETA;
    http.begin(urlParaEnvio);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonPayload);

    if (httpCode > 0)
    {
        Serial.printf("ServidorWeb: Configuração Enviada (Code %d). Resposta: %s\n", httpCode, http.getString().c_str());
    }
    else
    {
        Serial.printf("ServidorWeb: Falha no Envio HTTP (Config): %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}