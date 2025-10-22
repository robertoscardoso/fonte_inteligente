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

    executarSQL(String(sql_criar_tabela));

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
    executarSQL(sql_insert);
}

// Método para ler e imprimir todos os logs
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

    const char *sql_select = "SELECT DATA_HORA, TIPO, BATERIA_PERCENTUAL FROM LOG_ENERGIA ORDER BY ID DESC;";
    executarSQL(String(sql_select));

    Serial.println("-----------------------------------------\n");
}

// Implementação da função de callback
int GerenciadorDeLogs::callback(void *data, int argc, char **argv, char **azColName)
{
    Serial.printf("| ");
    for (int i = 0; i < argc; i++)
    {
        Serial.printf("%s: %s | ", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    Serial.println();
    return 0;
}

// Implementação do executor de SQL
void GerenciadorDeLogs::executarSQL(const String &sql)
{
    char *zErrMsg = 0;
    const char *sql_c = sql.c_str();

    int rc = sqlite3_exec(_db, sql_c, callback, (void *)"Resultados", &zErrMsg);

    if (rc != SQLITE_OK)
    {
        Serial.printf("ERRO SQL (%d): %s\n", rc, zErrMsg);
        sqlite3_free(zErrMsg);
    }
}