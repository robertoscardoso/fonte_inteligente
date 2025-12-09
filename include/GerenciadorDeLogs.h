#ifndef GERENCIADOR_DE_LOGS_H
#define GERENCIADOR_DE_LOGS_H

#include <Arduino.h>
#include <sqlite3.h>

class GerenciadorDeLogs
{
public:
    // Construtor: recebe o caminho do arquivo do banco de dados
    GerenciadorDeLogs(const char *caminho_db);

    // Destrutor: fecha a conexão com o banco de dados
    ~GerenciadorDeLogs();

    // Método para inicializar o sistema de arquivos e o banco de dados
    bool iniciar();

    // Método para salvar um novo evento de energia
    void registrarEvento(const String &dataHora, const String &tipo, float percentualBateria);

    // Método para ler e imprimir todos os logs salvos no Serial Monitor
    void lerTodosOsLogs();
    
    String getLogsAsJson();
    String getNewLogsAsJson(int ultimoID);

private:
    sqlite3 *_db; // Ponteiro para o objeto do banco de dados
    const char *_caminhoDb; // Armazena o caminho para o arquivo do DB

    // Função estática de callback para processar os resultados de consultas (SELECT)
    static int callback(void *data, int argc, char **argv, char **azColName);

    // Método privado para executar comandos SQL genéricos
    void executarSQL(const String &sql);
};

#endif // GERENCIADOR_DE_LOGS_H