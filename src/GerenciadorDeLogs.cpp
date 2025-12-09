#include <Arduino.h>
#include "GerenciadorDeLogs.h"
#include "LittleFS.h"

// Define se o LittleFS deve ser formatado caso a montagem falhe
#define FORMATAR_LITTLEFS_SE_FALHAR true

// Construtor
GerenciadorDeLogs::GerenciadorDeLogs(const char *caminho_db) : _db(nullptr), _caminhoDb(caminho_db) {}

// Destrutor
GerenciadorDeLogs::~GerenciadorDeLogs()
{
    if (_db)
    {
        sqlite3_close(_db);
        Serial.println("Banco de Dados SQLite fechado.");
    }
}

// Método de inicialização
bool GerenciadorDeLogs::iniciar()
{
    // 1. Inicializa o LittleFS
    if (!LittleFS.begin(FORMATAR_LITTLEFS_SE_FALHAR))
    {
        Serial.println("ERRO: Falha na inicialização do LittleFS!");
        return false;
    }
    Serial.println("LittleFS montado com sucesso.");

    // 2. Abre ou cria o banco de dados
    int rc = sqlite3_open(_caminhoDb, &_db);
    if (rc)
    {
        Serial.printf("Não foi possível abrir/criar o DB: %s\n", sqlite3_errmsg(_db));
        return false;
    }
    Serial.println("Banco de Dados SQLite aberto com sucesso.");

    // 3. Cria a tabela de logs se ela não existir
    const char *sql_criar_tabela =
        "CREATE TABLE IF NOT EXISTS LOG_ENERGIA ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "DATA_HORA TEXT NOT NULL,"
        "TIPO TEXT NOT NULL,"
        "BATERIA_PERCENTUAL REAL NOT NULL);";

    executarSQL(String(sql_criar_tabela)); // Usa o executor padrão

    return true;
}

// Método para registrar um evento no banco de dados
void GerenciadorDeLogs::registrarEvento(const String &dataHora, const String &tipo, float percentualBateria)
{
    String sql_insert = "INSERT INTO LOG_ENERGIA (DATA_HORA, TIPO, BATERIA_PERCENTUAL) VALUES ('" +
                      dataHora + "', '" +
                      tipo + "', " +
                      String(percentualBateria, 1) + ");";

    Serial.printf("Salvando evento no DB: %s - %.1f%%\n", tipo.c_str(), percentualBateria);
    executarSQL(sql_insert); // Usa o executor padrão
}

// Método para ler e imprimir todos os logs (MODIFICADO)
void GerenciadorDeLogs::lerTodosOsLogs()
{
    if (!_db)
    {
        Serial.println("ERRO: O Banco de Dados não está aberto para leitura.");
        return;
    }

    Serial.println("\n-----------------------------------------");
    Serial.println(" HISTÓRICO DE EVENTOS SALVOS (SQLite) ");
    Serial.println("-----------------------------------------");

    // (MODIFICADO) A query agora seleciona o ID também
    const char *sql_select = "SELECT ID, DATA_HORA, TIPO, BATERIA_PERCENTUAL FROM LOG_ENERGIA ORDER BY ID DESC;";
    
    // (MODIFICADO) Chama sqlite3_exec diretamente para usar o callback de serial
    char *zErrMsg = 0;
    int rc = sqlite3_exec(_db, sql_select, callback, (void *)"Resultados", &zErrMsg);
    if (rc != SQLITE_OK)
    {
        Serial.printf("ERRO SQL (lerTodosOsLogs): %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    Serial.println("-----------------------------------------\n");
}

// Implementação da função de callback (para print no Serial) (MODIFICADO)
int GerenciadorDeLogs::callback(void *data, int argc, char **argv, char **azColName)
{
    // Agora o argv[0] é o ID
    Serial.printf("| ");
    for (int i = 0; i < argc; i++)
    {
        // Imprime "NOME_COLUNA: valor"
        Serial.printf("%s: %s | ", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    Serial.println();
    return 0;
}

// Implementação do executor de SQL (usado apenas para INSERT/CREATE)
void GerenciadorDeLogs::executarSQL(const String &sql)
{
    char *zErrMsg = 0;
    const char *sql_c = sql.c_str();

    // Executa sem callback, pois é para INSERT/CREATE
    int rc = sqlite3_exec(_db, sql_c, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK)
    {
        Serial.printf("ERRO SQL (%d): %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }
}

// ------------------------------------------------------------------
// MÉTODOS PARA O SERVIDOR WEB
// ------------------------------------------------------------------

// (MODIFICADO) CALLBACK ESTÁTICO PARA CONSTRUIR O JSON
// Este callback é chamado pelo sqlite3_exec para cada linha do resultado
static int jsonCallback(void *data, int argc, char **argv, char **azColName) {
    String *jsonString = static_cast<String*>(data); 
    
    if (jsonString->length() > 1) { // Adiciona vírgula se não for o primeiro item
        *jsonString += ",";
    }
    
    *jsonString += "{";
    *jsonString += "\"id\":" + String(argv[0] ? argv[0] : "0") + ","; 
    *jsonString += "\"data_hora\":\"" + String(argv[1] ? argv[1] : "") + "\",";
    *jsonString += "\"tipo\":\"" + String(argv[2] ? argv[2] : "") + "\",";
    *jsonString += "\"bateria_perc\":" + String(argv[3] ? argv[3] : "0.0");
    *jsonString += "}";
    
    return 0;
}

// (MODIFICADO) MÉTODO PÚBLICO (Para Carga Inicial)
String GerenciadorDeLogs::getLogsAsJson() {
    if (!_db) {
        Serial.println("ERRO: DB não está aberto para ler logs JSON (carga inicial).");
        return "[]"; // Retorna um array JSON vazio
    }

    String json = "["; // Inicia o array JSON
    char *zErrMsg = 0;
    
    // (MUDANÇA CRÍTICA) A query SQL agora seleciona o ID
    const char *sql_select = "SELECT ID, DATA_HORA, TIPO, BATERIA_PERCENTUAL FROM LOG_ENERGIA ORDER BY ID DESC LIMIT 50;"; 

    // Usa o jsonCallback (que agora espera o ID), passando o ponteiro da string 'json'
    int rc = sqlite3_exec(_db, sql_select, jsonCallback, &json, &zErrMsg);

    if (rc != SQLITE_OK) {
        Serial.printf("ERRO SQL (getLogsAsJson): %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    json += "]"; // Fecha o array JSON
    return json;
}


String GerenciadorDeLogs::getNewLogsAsJson(int ultimoID) {
    if (!_db) {
        Serial.println("ERRO: DB não está aberto para ler logs JSON (novos).");
        return "[]";
    }

    String json = "[";
    char *zErrMsg = 0;
    
    String sql_select_str = "SELECT ID, DATA_HORA, TIPO, BATERIA_PERCENTUAL FROM LOG_ENERGIA WHERE ID > " + 
                            String(ultimoID) + 
                            " ORDER BY ID ASC;"; 

    int rc = sqlite3_exec(_db, sql_select_str.c_str(), jsonCallback, &json, &zErrMsg);

    if (rc != SQLITE_OK) {
        Serial.printf("ERRO SQL (getNewLogsAsJson): %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    json += "]";
    return json;
}