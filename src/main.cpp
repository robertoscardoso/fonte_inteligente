#include "Arduino.h"
#include "Energia.h"
#include "RedeExterna.h"
#include "Bateria.h"
#include "WiFiManager.h"
#include <NTPClient.h>
#include <time.h>
#include "LittleFS.h"
#include <config_ext.h>
#include <sqlite3.h> // O principal header
// -------------------------------------------------------------
// DECLARAÇÕES GLOBAIS E CONFIGURAÇÕES DO SQLITE
// -------------------------------------------------------------
Bateria B_18650(3, 3.2, 0.0);
RedeExterna tomada(1);
WiFiManager rede_wifi;
bool redeExternaEstavaAtiva = false;
int transicao_delay = 3000;

// Configurações do SQLite
#define DB_PATH "/littlefs/log_energia.db" // Caminho do banco de dados na LittleFS
sqlite3 *db;                               // Objeto de banco de dados SQLite

// Flag para inicialização do LittleFS
#define FORMAT_LITTLEFS_IF_FAILED true

// -----------------------------------------------------------------------------
// 1. FUNÇÕES AUXILIARES PARA ENUMS
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

String statusBateriaToString(BateriaStatus status)
{
    switch (status)
    {
    case ATIVADA:
        return "ATIVADA";
    case DESATIVADO:
        return "DESATIVADA";
    default:
        return "DESCONHECIDO";
    }
}

// -----------------------------------------------------------------------------
// 2. CONFIGURAÇÕES E FUNÇÕES NTP
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
    timeInfo = localtime(&epochTime);
    char buffer[30];
    strftime(buffer, 30, "%d/%m/%Y %H:%M:%S", timeInfo);
    return String(buffer);
}

// -----------------------------------------------------------------------------
// 3. FUNÇÕES SQLITE
// -----------------------------------------------------------------------------

// Função de Callback para processar os resultados do SELECT
static int callback(void *data, int argc, char **argv, char **azColName)
{
    Serial.printf("| ");
    for (int i = 0; i < argc; i++)
    {
        // Imprime o nome da coluna e o valor (Ex: DATA_HORA: 20/10/2025 10:30:00)
        Serial.printf("%s: %s | ", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    Serial.println();
    return 0;
}

// Função para executar comandos SQL (INSERT, CREATE, SELECT, etc.)
void execSQL(const String &sql)
{
    char *zErrMsg = 0;
    const char *sql_c = sql.c_str();

    // Executa o comando
    int rc = sqlite3_exec(db, sql_c, callback, (void *)"Resultados", &zErrMsg);

    if (rc != SQLITE_OK)
    {
        Serial.printf("ERRO SQL (%d): %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

// Função de leitura/consulta de logs no SQLite
void readLogSQL()
{
    // Verifica se o DB está aberto
    if (!db)
    {
        Serial.println("ERRO: O Banco de Dados não está aberto.");
        return;
    }

    Serial.println("\n-----------------------------------------");
    Serial.println(" HISTÓRICO DE EVENTOS SALVOS (SQLite) ");
    Serial.println("-----------------------------------------");

    // Seleciona as colunas relevantes, ordenando do mais recente para o mais antigo
    const char *sql_select = "SELECT DATA_HORA, TIPO, BATERIA_PERCENTUAL FROM LOG_ENERGIA ORDER BY ID DESC;";

    // Converte para String para usar a função execSQL
    execSQL(String(sql_select));

    Serial.println("-----------------------------------------\n");
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

    // 1. Inicializar LittleFS (necessário para o SQLite salvar o arquivo)
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
    {
        Serial.println("ERRO: Falha na inicialização do LittleFS!");
        return;
    }
    Serial.println("LittleFS montado com sucesso.");

    // 2. Abrir/Criar o Banco de Dados
    int rc = sqlite3_open(DB_PATH, &db);
    if (rc)
    {
        Serial.printf("Não foi possível abrir/criar o DB: %s\n", sqlite3_errmsg(db));
        return;
    }
    Serial.println("Banco de Dados SQLite aberto com sucesso.");

    // 3. Criar Tabela de Log (se não existir)
    const char *sql_create =
        "CREATE TABLE IF NOT EXISTS LOG_ENERGIA ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "DATA_HORA TEXT NOT NULL,"
        "TIPO TEXT NOT NULL,"                 // Armazena 'QUEDA' ou 'RETORNO'
        "BATERIA_PERCENTUAL REAL NOT NULL);"; // Armazena o percentual da bateria

    execSQL(String(sql_create));

    // NOVIDADE: Exibir logs salvos usando SQLite
    Serial.println("\n--- Iniciando Leitura do Histórico de Eventos ---");
    readLogSQL(); // Chama a nova função de leitura via SQL
    Serial.println("--- Leitura do Histórico Concluída ---");

    // Espera 5 segundos (5000 ms) antes de iniciar o loop principal
    Serial.println("Aguardando 5 segundos para iniciar o monitoramento...");
    delay(5000);
    Serial.println("Iniciando loop principal...");
}
// -----------------------------------------------------------------------------

void loop()
{
    // --------------------------------------------------------------------------
    // 1. OBTENÇÃO DOS DADOS
    // --------------------------------------------------------------------------

    // Dados da Bateria
    float tensaoBateria = B_18650.getTensao();
    float porcentagem = B_18650.getPorcentagem(); // Percentual é necessário para o log
    String statusBateria = statusBateriaToString(B_18650.getStatus());

    // Dados da Rede Externa (Tomada)
    float tensaoTomada = tomada.getTensao();
    String statusTomada = statusTomadaToString(tomada.getStatus());

    // Define o ESTADO ATUAL (variável local)
    bool redeExternaAtiva = (tensaoTomada > 0.2);

    // --------------------------------------------------------------------------
    // 2. DETECÇÃO DE BORDA E COMUTAÇÃO (A Lógica de Ação Única)
    // --------------------------------------------------------------------------

    if (redeExternaAtiva != redeExternaEstavaAtiva)
    {
        // Garante a hora exata do evento de transição
        timeClient.update();
        String dataHoraAtual = obterDataHoraFormatada(timeClient);

        Serial.println("\n*");
        Serial.println("*** TRANSIÇÃO DE ENERGIA DETECTADA! ***");

        String tipo_evento; // Variável para armazenar o TIPO (QUEDA/RETORNO)

        if (redeExternaAtiva)
        {
            // A rede VOLTOU: Comuta para REDE EXTERNA
            Serial.println("A rede externa VOLTOU as: " + dataHoraAtual);
            tipo_evento = "RETORNO";

            delay(transicao_delay);

            tomada.setStatus(ATIVADO);
            B_18650.setStatus(DESATIVADA);
        }
        else
        {
            // A rede CAIU: Comuta para BATERIA
            Serial.println("A rede externa CAIU as: " + dataHoraAtual);
            tipo_evento = "QUEDA";

            delay(transicao_delay);

            tomada.setStatus(DESATIVADO);
            B_18650.setStatus(ATIVADA);
        }

        // --- SALVANDO O LOG NO SQLITE AQUI ---
        // Monta o comando INSERT. Usamos String(porcentagem, 1) para formatar o float com 1 decimal.
        String sql_insert = "INSERT INTO LOG_ENERGIA (DATA_HORA, TIPO, BATERIA_PERCENTUAL) VALUES ('" +
                            dataHoraAtual + "', '" +
                            tipo_evento + "', " +
                            String(porcentagem, 1) + ");";

        Serial.printf("Salvando evento no DB: %s - %.1f%%\n", tipo_evento.c_str(), porcentagem);
        execSQL(sql_insert);
        // -------------------------------------------------------

        // ATUALIZAÇÃO DO ESPELHO (ESSENCIAL)
        redeExternaEstavaAtiva = redeExternaAtiva;
        Serial.println("*\n");
    }

    // Opcional: Manter o relógio interno atualizado a cada 60s (conforme config NTP)
    timeClient.update();

    // --------------------------------------------------------------------------
    // 3. IMPRESSÃO DOS DADOS (O Bloco de Apresentação)
    // --------------------------------------------------------------------------

    // Pega a hora atual para o log de monitoramento
    String dataHoraLog = obterDataHoraFormatada(timeClient);

    Serial.println("\n=========================================");
    Serial.println("          MONITORAMENTO DE ENERGIA       ");
    Serial.println("DATA/HORA: " + dataHoraLog);
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