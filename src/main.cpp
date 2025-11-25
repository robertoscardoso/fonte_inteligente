// INCLUDES DO PROJETO
#include <Arduino.h>
#include "Energia.h"
#include "RedeExterna.h"
#include "Bateria.h"
#include "GerenciadorDeLogs.h"
#include <NTPClient.h>
#include <time.h>
#include <Wire.h>
#include <FS.h>
#include <LittleFS.h>
#include "ServidorWeb.h"   // <<< NOVA CLASSE DO WEBSERVER

// -------------------------------------------------------------
// DECLARAÇÕES GLOBAIS
// -------------------------------------------------------------

#define CAMINHO_DB "/littlefs/log_energia.db"

const char *hostname = "fonteinteligente";
const char *ssid = "ESP32-CONFIG";
const char *password = "12345678";
const long updateInterval = 1000;

const int pinoBuckBooster = 0;

unsigned long lastUpdateTime = 0;
bool redeExternaEstavaAtiva = false;

Bateria B_18650(0, 3.1, 0);
RedeExterna tomada(2);
GerenciadorDeLogs gerenciadorDeLogs(CAMINHO_DB);
ServidorWeb servidor(80,&gerenciadorDeLogs,&B_18650,&tomada);
// -----------------------------------------------------------------------------
// CONFIGURAÇÃO DO NTP 
// -----------------------------------------------------------------------------
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -10800;  // -3h do Brasil
const char *ntpServer = "time.google.com";
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds, 60000);

String obterDataHoraFormatada(NTPClient &client)
{
    time_t epochTime = client.getEpochTime();
    struct tm *timeInfo = localtime(&epochTime);
    char buffer[30];
    strftime(buffer, 30, "%d/%m/%Y %H:%M:%S", timeInfo);
    return String(buffer);
}




// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println("\nSistema de Monitoramento de Energia Inicializado!");

    // Inicializa LittleFS
    if (!LittleFS.begin())
    {
        Serial.println("Erro ao montar LittleFS!");
        while (1);
    }

    // Inicializa rede + webserver
    servidor.iniciar(ssid, password, hostname);

    // Inicializa os componentes
    Energia::inicializar();
    pinMode(pinoBuckBooster, OUTPUT);
    digitalWrite(pinoBuckBooster, LOW);

    // Inicializa NTP
    timeClient.begin();
    timeClient.update();

    // Inicializa o banco/logs
    if (!gerenciadorDeLogs.iniciar())
    {
        Serial.println("Falha crítica ao inicializar o Gerenciador de Logs. Sistema parado.");
        while (1);
    }
}

// -----------------------------------------------------------------------------
// LOOP PRINCIPAL
// -----------------------------------------------------------------------------
void loop()
{
    // Atualiza o WebServer (rotas, JSON, histórico etc.)
    servidor.atualizar();

    // Atualiza medições
    float porcentagem = B_18650.getPorcentagem();
    float tensaoTomada = tomada.getTensao();
    bool redeExternaAtiva = (tensaoTomada > 0.5);

    // Verifica mudanças de estado (queda/retorno)
    if (millis() - lastUpdateTime >= updateInterval)
    {
        if (redeExternaAtiva != redeExternaEstavaAtiva)
        {
            timeClient.update();
            String dataHoraAtual = obterDataHoraFormatada(timeClient);
            String tipo_evento;

            if (redeExternaAtiva)
            {
                tomada.setStatus(ATIVADO);
                B_18650.setStatus(DESATIVADA);
                tipo_evento = "RETORNO";
            }
            else
            {
                tomada.setStatus(DESATIVADO);
                B_18650.setStatus(ATIVADA);
                tipo_evento = "QUEDA";
            }

            gerenciadorDeLogs.registrarEvento(dataHoraAtual, tipo_evento, porcentagem);

            redeExternaEstavaAtiva = redeExternaAtiva;
            lastUpdateTime = millis();
        }

        timeClient.update();
    }
}
