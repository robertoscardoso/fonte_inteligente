#include "GerenciadorDePotencia.h"
#include <WiFi.h>
#include <time.h> 

// --- CONSTRUTOR E MÉTODOS EXISTENTES ---
GerenciadorDePotencia::GerenciadorDePotencia() {
    // Construtor
}

void GerenciadorDePotencia::prepararParaDormir() {
    Serial.println("Desligando periféricos para entrar em modo de economia...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi Desligado.");
    Serial.end();
}

void GerenciadorDePotencia::ativarModoEconomia() {
    prepararParaDormir();
    uint64_t tempo_sono_us = (uint64_t)TEMPO_SONO_MINUTOS * 60 * 1000000ULL;
    esp_sleep_enable_timer_wakeup(tempo_sono_us);
    Serial.println("Entrando em sono profundo por " + String(TEMPO_SONO_MINUTOS) + " minutos.");
    esp_deep_sleep_start();
}


// (Necessária para este arquivo compilar sem conflito)
static String obterDataHoraFormatada(NTPClient &client)
{
    time_t epochTime = client.getEpochTime();
    struct tm *timeInfo = localtime(&epochTime);
    char buffer[30];
    strftime(buffer, 30, "%d/%m/%Y %H:%M:%S", timeInfo);
    return String(buffer);
}



void GerenciadorDePotencia::verificarEAtivarEconomia(
    float tensaoBateria, 
    float tensaoDeCorte, 
    bool redeExternaAtiva, 
    float porcentagem, 
    GerenciadorDeLogs& logger,
    NTPClient& client
) {
    
    if (tensaoBateria <= tensaoDeCorte && !redeExternaAtiva) 
    {
        Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Serial.println("!!! NÍVEL CRÍTICO DE BATERIA ATINGIDO !!!");
        Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        
        client.update(); // Atualiza a hora
        String dataHoraAtual = obterDataHoraFormatada(client); // Pega a hora formatada

        // 1. Registrar último log
        logger.registrarEvento(dataHoraAtual, "BATERIA_CRITICA", porcentagem);
        
        Serial.println("Aguardando 2s para salvar log...");
        delay(2000); 

        // 2. Iniciar modo de economia 
        this->ativarModoEconomia(); 
    }
}