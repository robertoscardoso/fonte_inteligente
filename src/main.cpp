#include "Arduino.h"
#include "Energia.h"
#include "RedeExterna.h"
#include "Bateria.h"
#include "WiFiManager.h"
#include <NTPClient.h>
#include <time.h>

// -------------------------------------------------------------
// DECLARAÇÕES GLOBAIS
// -------------------------------------------------------------
Bateria B_18650(3, 3.2, 0.0);
RedeExterna tomada(1);
WiFiManager rede_wifi;
bool redeExternaEstavaAtiva = false;
int transicao_delay = 3000;
// -----------------------------------------------------------------------------
// 1. FUNÇÃO AUXILIAR PARA O ENUM DA TOMADA
// -----------------------------------------------------------------------------
String statusTomadaToString(statusRedeExterna status)
{
    switch (status)
    {
    case ATIVADO:
        return "ATIVADA";
    case DESATIVADO:
        return "DESATIVADA";
    default:
        return "DESCONHECIDO";
    }
}

// -----------------------------------------------------------------------------
// 2. FUNÇÃO AUXILIAR PARA O ENUM DA BATERIA (CORREÇÃO DO SEU ERRO)
// -----------------------------------------------------------------------------
String statusBateriaToString(BateriaStatus status)
{
    switch (status)
    {
    case ATIVADA:
        return "ATIVADA";
    case DESATIVADA:
        return "DESATIVADA";
    default:
        return "DESCONHECIDO";
    }
}

// -----------------------------------------------------------------------------
// 3. CONFIGURAÇÕES DO SERVIDOR NTP
// -----------------------------------------------------------------------------

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -10800;
const char *ntpServer = "a.st1.ntp.br";
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds, 60000);

// Função para obter Data e Hora completas
String obterDataHoraFormatada(NTPClient &client)
{
    time_t epochTime = client.getEpochTime();
    struct tm *timeInfo;

    // Converte o tempo Epoch (segundos) para a estrutura tm (data/hora)
    timeInfo = localtime(&epochTime);

    // Aloca buffer para a string: DD/MM/YYYY HH:MM:SS (Exemplo: 20/10/2025 10:30:00)
    char buffer[30];

    // Usa strftime para formatar a data e hora
    strftime(buffer, 30, "%d/%m/%Y %H:%M:%S", timeInfo);

    return String(buffer);
}

// -----------------------------------------------------------------------------
void setup()
{
    delay(3000);
    Serial.begin(115200);
    Serial.println("Sistema de Monitoramento de Energia Inicializado!");
    rede_wifi.conectar("BORGES", "gomugomu");
    Energia::inicializar();

    timeClient.begin();
    timeClient.update();
}
// -----------------------------------------------------------------------------

void loop()
{
    // --------------------------------------------------------------------------
    // 1. OBTENÇÃO DOS DADOS
    // --------------------------------------------------------------------------

    // Dados da Bateria
    float tensaoBateria = B_18650.getTensao();
    float porcentagem = B_18650.getPorcentagem();
    String statusBateria = statusBateriaToString(B_18650.getStatus());

    // Dados da Rede Externa (Tomada)
    float tensaoTomada = tomada.getTensao();
    String statusTomada = statusTomadaToString(tomada.getStatus());

    // Define o ESTADO ATUAL (variável local)
    bool redeExternaAtiva = (tensaoTomada > 0.2);

    // --------------------------------------------------------------------------
    // 2. DETECÇÃO DE BORDA E COMUTAÇÃO (A Lógica de Ação Única)
    // --------------------------------------------------------------------------

    // O bloco só executa se o estado MUDOU (Detecção de Borda)
    if (redeExternaAtiva != redeExternaEstavaAtiva)
    {
        // Garante a hora exata do evento de transição
        timeClient.update();
        String dataHoraAtual = obterDataHoraFormatada(timeClient);

        Serial.println("\n***********************************");
        Serial.println("*** TRANSIÇÃO DE ENERGIA DETECTADA! ***");

        if (redeExternaAtiva)
        {
            // A rede VOLTOU: Comuta para REDE EXTERNA
            Serial.println("A rede externa VOLTOU as: " + dataHoraAtual);
            delay(transicao_delay);

            tomada.setStatus(ATIVADO);
            B_18650.setStatus(DESATIVADA);
        }
        else
        {
            // A rede CAIU: Comuta para BATERIA
            Serial.println("A rede externa CAIU as: " + dataHoraAtual);
            delay(transicao_delay);

            tomada.setStatus(DESATIVADO);
            B_18650.setStatus(ATIVADA);
        }

        // -------------------------------------------------------
        // ATUALIZAÇÃO DO ESPELHO (ESSENCIAL)
        // -------------------------------------------------------
        redeExternaEstavaAtiva = redeExternaAtiva;
        Serial.println("***********************************\n");
    }

    // Opcional: Manter o relógio interno atualizado a cada 60s (conforme config NTP)
    timeClient.update();

    // --------------------------------------------------------------------------
    // 3. IMPRESSÃO DOS DADOS (O Bloco de Apresentação)
    // --------------------------------------------------------------------------

    // Pega a hora atual para o log de monitoramento
    String dataHoraLog = obterDataHoraFormatada(timeClient);

    Serial.println("\n=========================================");
    Serial.println("         MONITORAMENTO DE ENERGIA         ");
    Serial.println("=========================================");

    // Imprime a fonte ativa no momento
    if (redeExternaEstavaAtiva)
    {
        Serial.println(" > FONTE ATIVA: REDE EXTERNA <");
    }
    else
    {
        Serial.println(" > FONTE ATIVA: BATERIA <");
    }

    Serial.println("-----------------------------------------");

    // DADOS DA REDE EXTERNA
    Serial.println("DADOS DA REDE EXTERNA:");
    Serial.println(" -> Status (Comutador): " + statusTomada);
    Serial.println(" -> Tensão Atual:      " + String(tensaoTomada, 1) + "V");

    Serial.println("-----------------------------------------");

    // DADOS DA BATERIA
    Serial.println("DADOS DA BATERIA (18650):");
    Serial.println(" -> Status (Comutador): " + statusBateria);
    Serial.println(" -> Tensão Atual:      " + String(tensaoBateria, 2) + "V");
    Serial.println(" -> Porcentagem:       " + String(porcentagem, 1) + "%");

    Serial.println("=========================================\n");

    delay(1000); // Espera 1 segundo antes da próxima atualização.
}