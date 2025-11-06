// INCLUDES DO PROJETO
#include <Arduino.h>
#include "Energia.h"
#include "RedeExterna.h"
#include "Bateria.h"
#include "WiFiManager.h"
#include "GerenciadorDeLogs.h"
#include <NTPClient.h>
#include <time.h>
#include "WebServerManager.h"
#include "GerenciadorDePotencia.h"

// INCLUI A NOSSA NOVA CLASSE
#include "NotificadorTelegram.h"

// --- CONTROLE DE FUNCIONALIDADES ---
// Comente a linha abaixo para DESATIVAR o modo de economia de energia
#define HABILITAR_MODO_ECONOMIA
// Defina a tensão de corte (em Volts) para ativar a economia
const float TENSAO_DE_CORTE = 3.4; 
// ----------------------------------------

// -------------------------------------------------------------
// DECLARAÇÕES GLOBAIS
// -------------------------------------------------------------

Bateria B_18650(3, 4.2, 3.1); 

RedeExterna tomada(2);
WiFiManager rede_wifi;

// Cria a instância do gerenciador do servidor web
WebServerManager webServer(tomada, B_18650);

// Define o caminho do banco de dados
#define CAMINHO_DB "/littlefs/log_energia.db"
// Cria uma instância do nosso gerenciador de logs
GerenciadorDeLogs gerenciadorDeLogs(CAMINHO_DB);

bool redeExternaEstavaAtiva = false;
int transicao_delay = 3000;

// --- CONFIGURAÇÕES DO TELEGRAM ---
#define BOT_TOKEN "8033617155:AAGaXf8IOH_CRTGgP2V5nQIQ35GscalG0cY"
#define CHAT_ID "1561936580"
// Cria uma instância global da nossa nova classe
NotificadorTelegram notificador(BOT_TOKEN, CHAT_ID);

GerenciadorDePotencia gerenciadorPotencia;
// ---------------------------------

// -----------------------------------------------------------------------------
// 1. FUNÇÕES AUXILIARES PARA ENUMS (mantidas como estão)
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
// 2. CONFIGURAÇÕES E FUNÇÕES NTP (COM 'static')
// -----------------------------------------------------------------------------
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = -10800;
const char *ntpServer = "a.st1.ntp.br";
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds, 60000);

// --- ADICIONADO 'static' para evitar erro de "multiple definition" ---
static String obterDataHoraFormatada(NTPClient &client)
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

    // Tenta conectar e VERIFICA o resultado
    if (rede_wifi.conectar("Rede", "Senha")) { 
        // Se conectou com sucesso, inicia o servidor web
        webServer.iniciar(); 
    } else {
        // Se falhou, avisa e NÃO inicia o servidor
        Serial.println("Falha ao conectar ao WiFi. Servidor Web não iniciado.");
    }
    
    // --- USA A NOSSA CLASSE PARA INICIALIZAR O TELEGRAM ---
    notificador.inicializar();
    // ----------------------------------------------------

    Energia::inicializar();

    timeClient.begin();
    timeClient.update();

    if (!gerenciadorDeLogs.iniciar())
    {
        Serial.println("Falha crítica ao inicializar o Gerenciador de Logs. Sistema parado.");
        while (1);
    }

    Serial.println("\n--- Iniciando Leitura do Histórico de Eventos ---");
    gerenciadorDeLogs.lerTodosOsLogs();
    Serial.println("--- Leitura do Histórico Concluída ---");

    // --- USA A NOSSA CLASSE PARA ENVIAR A MENSAGEM INICIAL ---
    notificador.enviarMensagemInicializacao();
    // ---------------------------------------------------------

    Serial.println("Aguardando 5 segundos para iniciar o monitoramento...");
    delay(5000);
    Serial.println("Iniciando loop principal...");
}

// -----------------------------------------------------------------------------
// LOOP PRINCIPAL
// -----------------------------------------------------------------------------
void loop()
{
    float tensaoBateria = B_18650.getTensao();
    float porcentagem = B_18650.getPorcentagem();
    float tensaoTomada = tomada.getTensao();
    bool redeExternaAtiva = (tensaoTomada > 1.0);

    //VERIFICAÇÃO DO MODO DE ECONOMIA DE ENERGIA ---
    #ifdef HABILITAR_MODO_ECONOMIA

    gerenciadorPotencia.verificarEAtivarEconomia(
        tensaoBateria,
        TENSAO_DE_CORTE,
        redeExternaAtiva,
        porcentagem,
        gerenciadorDeLogs, // Passa o objeto de logs
        timeClient         // Passa o objeto de tempo
    );
    
    #endif
    // --- FIM DA VERIFICAÇÃO ---
    


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
            
            // --- USA A NOSSA CLASSE PARA ENVIAR O AVISO DE RETORNO ---
            notificador.enviarAvisoRetornoEnergia(dataHoraAtual);
            // ----------------------------------------------------------

            delay(transicao_delay);
            tomada.setStatus(ATIVADO);
            B_18650.setStatus(DESATIVADA);
        }
        else
        {
            Serial.println("A rede externa CAIU as: " + dataHoraAtual);
            tipo_evento = "QUEDA";

            // --- USA A NOSSA CLASSE PARA ENVIAR O ALERTA DE QUEDA ---
            notificador.enviarAlertaQuedaEnergia(dataHoraAtual, porcentagem);
            // ---------------------------------------------------------
            
            delay(transicao_delay);
            tomada.setStatus(DESATIVADO);
            B_18650.setStatus(ATIVADA);
        }

        gerenciadorDeLogs.registrarEvento(dataHoraAtual, tipo_evento, porcentagem);
        redeExternaEstavaAtiva = redeExternaAtiva;
        Serial.println("*\n");
    }

    timeClient.update();

    // O restante do loop de monitoramento permanece igual...
    String dataHoraLog = obterDataHoraFormatada(timeClient);
    Serial.println("\n=========================================");
    Serial.println("        MONITORAMENTO DE ENERGIA         ");
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