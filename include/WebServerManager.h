#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <ESPAsyncWebServer.h> // Inclui a biblioteca do servidor
#include "Energia.h"          // Para acessar os dados da rede
#include "Bateria.h"          // Para acessar os dados da bateria

class WebServerManager {
public:
    // Construtor: Recebe referências aos objetos de energia
    WebServerManager(Energia &rede, Bateria &bat); 

    // Método para iniciar o servidor web
    void iniciar();

    // (Opcional) Método para ser chamado no loop principal, se necessário
    // void loop(); 

private:
    AsyncWebServer server; // A instância do servidor web assíncrono (porta 80 por padrão)
    Energia& _redeExterna; // Referência ao objeto da rede externa
    Bateria& _bateria;     // Referência ao objeto da bateria

    // Métodos privados para lidar com as requisições
    void handleRoot(AsyncWebServerRequest *request);     // Responde à raiz "/"
    void handleDados(AsyncWebServerRequest *request);    // Responde a "/dados" (vamos usar depois)
    void handleHistorico(AsyncWebServerRequest *request); // Histórico 
    void handleNotFound(AsyncWebServerRequest *request); // Responde se a página não for encontrada
};

#endif // WEBSERVERMANAGER_H