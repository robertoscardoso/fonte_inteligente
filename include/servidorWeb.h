#ifndef SERVIDORWEB_H
#define SERVIDORWEB_H

#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "GerenciadorDeLogs.h"
#include "Bateria.h"
#include "RedeExterna.h"

class ServidorWeb
{
private:
    WebServer server;
    GerenciadorDeLogs *logs;
    Bateria *bateria;
    RedeExterna *rede;

    float tensaoBateria;
    float tensaoRede;
    float porcentagem;
    bool redeAtiva;
    bool redeAtivaAnterior;

    String idDispositivo;
    String apelido;

    // Handlers (callbacks)
    void handleRoot();
    void handleData();
    void handleHistorico();
    void handleHistoricoNew();
    void handleConnect();
    
    // Configurações
    void handleSalvarApelido();
    void handleResetConfig(); // <--- FALTAVA ISSO AQUI
    void salvarConfigNoArquivo();

public:
    ServidorWeb(
        int porta,
        GerenciadorDeLogs *logs,
        Bateria *bateria,
        RedeExterna *rede
    );

    void iniciar(const char *ssidAP, const char *senhaAP, const char *hostname);
    void atualizar(); 
    
    void verificarConfiguracao(String dataHoraAtual);
};

#endif