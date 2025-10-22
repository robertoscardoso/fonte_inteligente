#include "Arduino.h"
#include "Energia.h"
#include "RedeExterna.h"
#include "Bateria.h"
#include "WiFiManager.h"
#include "GerenciadorDeLogs.h" // <-- INCLUI A NOVA CLASSE
#include <NTPClient.h>
#include <time.h>
#include <config_ext.h>

// -------------------------------------------------------------
// DECLARAÇÕES GLOBAIS
// -------------------------------------------------------------
Bateria B_18650(3, 3.2, 0.0);
RedeExterna tomada(1);
WiFiManager rede_wifi;

// Define o caminho do banco de dados
#define CAMINHO_DB "/littlefs/log_energia.db"
// Cria uma instância do nosso gerenciador de logs
GerenciadorDeLogs gerenciadorDeLogs(CAMINHO_DB);

bool redeExternaEstavaAtiva = false;
int transicao_delay = 3000;

// -----------------------------------------------------------------------------
// 1. FUNÇÕES AUXILIARES PARA ENUMS
// -----------------------------------------------------------------------------
String statusTomadaToString(statusRedeExterna status)
{
    switch (status)
    {
    case ATIVADO: return "ATIVADA";
    case DESATIVADO: return "DESATIVADA";
    default: return "DESCONHECIDO";
    }
}

String statusBateriaToString(BateriaStatus status)
{
    switch (status)
    {
    case ATIVADA: return "ATIVADA";
    case DESATIVADO: return "DESATIVADA";
    default: return "DESCONHECIDO";
    }
}

// -----------------------------------------------------------------------------
// 2. CONFIGURAÇÕES E FUNÇÕES NTP
// -----------------------------------------------------------------------------
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -10800;
const char *ntpServer = "a.st1.ntp.br";
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
// SETUP: Inicialização
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

    // Inicializa o gerenciador de logs. Se falhar, interrompe a execução.
    if (!gerenciadorDeLogs.iniciar())
    {
        Serial.println("Falha crítica ao inicializar o Gerenciador de Logs. Sistema parado.");
        while (1); // Loop infinito para travar o microcontrolador
    }

    // Exibe os logs que já estavam salvos
    Serial.println("\n--- Iniciando Leitura do Histórico de Eventos ---");
    gerenciadorDeLogs.lerTodosOsLogs();
    Serial.println("--- Leitura do Histórico Concluída ---");

    Serial.println("Aguardando 5 segundos para iniciar o monitoramento...");
    delay(5000);
    Serial.println("Iniciando loop principal...");
}

// -----------------------------------------------------------------------------
// LOOP PRINCIPAL
// -----------------------------------------------------------------------------
void loop()
{
    // 1. OBTENÇÃO DOS DADOS
    float porcentagem = B_18650.getPorcentagem();
    float tensaoTomada = tomada.getTensao();
    bool redeExternaAtiva = (tensaoTomada > 0.2);

    // 2. DETECÇÃO DE TRANSIÇÃO E AÇÃO
    if (redeExternaAtiva != redeExternaEstavaAtiva)
    {
        timeClient.update();
        String dataHoraAtual = obterDataHoraFormatada(timeClient);
        String tipo_evento;

        Serial.println("\n*");
        Serial.println("*** TRANSIÇÃO DE ENERGIA DETECTADA! ***");

        if (redeExternaAtiva)
        {
            Serial.println("A rede externa VOLTOU as: " + dataHoraAtual);
            tipo_evento = "RETORNO";
            delay(transicao_delay);
            tomada.setStatus(ATIVADO);
            B_18650.setStatus(DESATIVADA);
        }
        else
        {
            Serial.println("A rede externa CAIU as: " + dataHoraAtual);
            tipo_evento = "QUEDA";
            delay(transicao_delay);
            tomada.setStatus(DESATIVADO);
            B_18650.setStatus(ATIVADA);
        }

        // --- SALVANDO O LOG USANDO A NOVA CLASSE ---
        gerenciadorDeLogs.registrarEvento(dataHoraAtual, tipo_evento, porcentagem);
        // -------------------------------------------

        redeExternaEstavaAtiva = redeExternaAtiva; // Atualiza o estado para a próxima verificação
        Serial.println("*\n");
    }

    timeClient.update();

    // 3. IMPRESSÃO DOS DADOS DE MONITORAMENTO
    String dataHoraLog = obterDataHoraFormatada(timeClient);
    Serial.println("\n=========================================");
    Serial.println("         MONITORAMENTO DE ENERGIA          ");
    Serial.println("DATA/HORA: " + dataHoraLog);
    Serial.println("=========================================");
    Serial.println(redeExternaEstavaAtiva ? " > FONTE ATIVA: REDE EXTERNA <" : " > FONTE ATIVA: BATERIA <");
    Serial.println("-----------------------------------------");
    Serial.println("DADOS DA REDE EXTERNA:");
    Serial.println(" -> Status (Comutador): " + statusTomadaToString(tomada.getStatus()));
    Serial.println(" -> Tensão Atual:       " + String(tomada.getTensao(), 1) + "V");
    Serial.println("-----------------------------------------");
    Serial.println("DADOS DA BATERIA (18650):");
    Serial.println(" -> Status (Comutador): " + statusBateriaToString(B_18650.getStatus()));
    Serial.println(" -> Tensão Atual:       " + String(B_18650.getTensao(), 2) + "V");
    Serial.println(" -> Porcentagem:        " + String(B_18650.getPorcentagem(), 1) + "%");
    Serial.println("=========================================\n");

    delay(1000);
}