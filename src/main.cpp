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
        while (1);
    }

    servidor.iniciar(ssid, password, hostname);

    Energia::inicializar();
    pinMode(pinoBuckBooster, OUTPUT);
    digitalWrite(pinoBuckBooster, LOW);

    timeClient.begin();
    timeClient.update();

    if (!gerenciadorDeLogs.iniciar())
    {
        Serial.println("Erro Logs");
        while (1);
    }
}

void loop()
{
    servidor.atualizar();

    float porcentagem = B_18650.getPorcentagem();
    float tensaoTomada = tomada.getTensao();
    bool redeExternaAtiva = (tensaoTomada > 0.5);

    if (millis() - lastUpdateTime >= updateInterval)
    {
        timeClient.update();

        // 1704067200 = 01/01/2024. Garante que so cria o ID se a data for valida e atual
        if (timeClient.getEpochTime() > 1704067200) {
            servidor.verificarConfiguracao(obterDataHoraFormatada(timeClient));
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

            redeExternaEstavaAtiva = redeExternaAtiva;
        }
        
        lastUpdateTime = millis();
    }
}