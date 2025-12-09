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
#include "ServidorWeb.h"

#define CAMINHO_DB "/littlefs/log_energia.db"

const char *hostname = "fonteinteligente";
const char *ssid = "FONTE-BACKUP";
const char *password = "";
const long updateInterval = 1000;
const int pinoBuckBooster = 0;
unsigned long lastUpdateTime = 0;
bool redeExternaEstavaAtiva = false;
bool verificado = false;

Bateria B_18650(1, 4.0, 3.1);
RedeExterna tomada(3);
GerenciadorDeLogs gerenciadorDeLogs(CAMINHO_DB);
ServidorWeb servidor(80, &gerenciadorDeLogs, &B_18650, &tomada);

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -10800;
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

void setup()
{
    Serial.begin(115200);
    Serial.println("\nSistema Iniciado");

    if (!LittleFS.begin())
    {
        Serial.println("Erro LittleFS");
        while (1)
            ;
    }

    servidor.iniciar(ssid, password, hostname);
    servidor.enviarConfiguracaoParaServidor();
    Energia::inicializar();
    pinMode(pinoBuckBooster, OUTPUT);
    digitalWrite(pinoBuckBooster, LOW);
    timeClient.begin();
    timeClient.update();

    if (!gerenciadorDeLogs.iniciar())
    {
        Serial.println("Erro Logs");
        while (1)
            ;
    }
}

void loop()
{
    servidor.atualizar();

    float porcentagem = B_18650.getPorcentagem();
    float tensaoTomada = tomada.getTensao();
    bool redeExternaAtiva = (tensaoTomada > 4.0);
    long epochTime = timeClient.getEpochTime();
    if (millis() - lastUpdateTime >= updateInterval)
    {
        timeClient.update();

        if (porcentagem > 0 || redeExternaAtiva)
        {
            digitalWrite(pinoBuckBooster, LOW);
        }
        else
        {
            digitalWrite(pinoBuckBooster, HIGH);
            delay(10000);
        }

        if ((epochTime > 1704067200) && !verificado)
        {
            servidor.verificarConfiguracao(obterDataHoraFormatada(timeClient), epochTime);
            verificado = !verificado;
        }

        if (redeExternaAtiva != redeExternaEstavaAtiva)
        {
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
            // servidor.enviarLogParaServidor(dataHoraAtual, tipo_evento, porcentagem);
            redeExternaEstavaAtiva = redeExternaAtiva;
        }

        lastUpdateTime = millis();
    }
}