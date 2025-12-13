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

// --- CONFIGURAÇÕES DEEP SLEEP ---
#define U_S_TO_S_FACTOR 1000000ULL  // Fator para converter segundos em microsegundos
#define TIME_TO_SLEEP  30          // Tempo dormindo em segundos (5 minutos)
#define LIMITE_BATERIA_CRITICA 2.9  // Tensão mínima da bateria para dormir
#define LIMITE_REDE_OFF 4.0         // Tensão abaixo disso considera rede desligada

#define CAMINHO_DB "/littlefs/log_energia.db"

const char *hostname = "fonteinteligente";
const char *ssid = "FONTE-A";
const char *password = "";
const long updateInterval = 1000;
const int pinoBuckBooster = 0;
unsigned long lastUpdateTime = 0;
bool redeExternaEstavaAtiva = false;
bool verificado = false;

//Trava para impedir oscilação em bateria baixa
bool bloqueioBateriaCritica = false;

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

void verificarDeepSleep() {
    float tensaoBat = B_18650.getTensao();
    float tensaoRede = tomada.getTensao();

    // Se Bateria <= 2.9V E Rede estiver desligada (< 4.0V)
    if (tensaoBat <= LIMITE_BATERIA_CRITICA && tensaoRede < LIMITE_REDE_OFF) {
        Serial.println("!!! BATERIA CRITICA E SEM REDE !!!");
        Serial.printf("Bat: %.2fV | Rede: %.2fV. Dormindo por 5min...\n", tensaoBat, tensaoRede);
        // garante que o BuckBooster não seja energizado 
        pinMode(pinoBuckBooster, OUTPUT);
        digitalWrite(pinoBuckBooster, HIGH); 

        // Configura o timer e dorme
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * U_S_TO_S_FACTOR);
        esp_deep_sleep_start();
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\nSistema Iniciado");
    Energia::inicializar();
    verificarDeepSleep();//verifica se deve dormir 

    if (!LittleFS.begin())
    {
        Serial.println("Erro LittleFS");
        while (1)
            ;
    }

    servidor.iniciar(ssid, password, hostname);
    servidor.enviarConfiguracaoParaServidor();
    
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
    verificarDeepSleep();

    servidor.atualizar();

    float porcentagem = B_18650.getPorcentagem();
    float tensaoTomada = tomada.getTensao();
    bool redeExternaAtiva = (tensaoTomada > 4.0);
    long epochTime = timeClient.getEpochTime();
    if (millis() - lastUpdateTime >= updateInterval)
    {
        timeClient.update();
        // --- NOVA LÓGICA COM DETECÇÃO DE TROCA DE BATERIA ---
        
        // Lê a tensão atual da bateria (necessário para identificar a troca)
        float tensaoAtualBateria = B_18650.getTensao();

        // 1. DESBLOQUEIO:
        // Destrava se a rede externa voltou OU se a tensão da bateria subiu subitamente (> 4.0V)
        // Isso indica que uma bateria carregada foi inserida.
        if (redeExternaAtiva || tensaoAtualBateria > 4.0) {
            bloqueioBateriaCritica = false;
        }

        // 2. BLOQUEIO (TRAVA):
        // Se a bateria zerou e não tem rede, ativa a trava.
        if (porcentagem == 0 && !redeExternaAtiva) {
            bloqueioBateriaCritica = true;
        }

        // 3. ATUAÇÃO NO PINO:
        if (bloqueioBateriaCritica || (porcentagem == 0 && !redeExternaAtiva)) 
        {
             digitalWrite(pinoBuckBooster, HIGH); // Mantém Desligado
        }
        else 
        {
             digitalWrite(pinoBuckBooster, LOW);  // Liga (Sistema Normal)
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
        // --- NOVO CÓDIGO PARA IMPRIMIR O ESTADO DO PINO ---
        int estadoPinoBuck = digitalRead(pinoBuckBooster);
        Serial.printf("Pino Buck Booster (GPIO %d) Estado: %s\n", 
                       pinoBuckBooster, 
                       (estadoPinoBuck == HIGH ? "HIGH (OFF)" : "LOW (ON)"));
        // ---------------------------------------------------
        lastUpdateTime = millis();
    }
}