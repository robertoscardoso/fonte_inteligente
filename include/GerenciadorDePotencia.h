#ifndef GERENCIADOR_DE_POTENCIA_H
#define GERENCIADOR_DE_POTENCIA_H

#include <Arduino.h>
#include "GerenciadorDeLogs.h" 
#include <NTPClient.h>       

// Define o tempo que o ESP32 ficará "dormindo" (em minutos).
#define TEMPO_SONO_MINUTOS 30 

class GerenciadorDePotencia {
public:
    GerenciadorDePotencia();
    void ativarModoEconomia();

    // --- NOSSO NOVO MÉTODO ---
    /**
     * @brief Verifica a tensão e, se crítico, salva o log e entra em modo de economia.
     * @param tensaoBateria Tensão atual da bateria.
     * @param tensaoDeCorte Tensão mínima para ativar o modo.
     * @param redeExternaAtiva Verdadeiro se a rede externa estiver ligada.
     * @param porcentagem Percentual atual da bateria (para o log).
     * @param logger Referência ao objeto GerenciadorDeLogs.
     * @param client Referência ao objeto NTPClient.
     */
    void verificarEAtivarEconomia(
        float tensaoBateria, 
        float tensaoDeCorte, 
        bool redeExternaAtiva, 
        float porcentagem, 
        GerenciadorDeLogs& logger, // Passa o objeto de log
        NTPClient& client        // Passa o objeto de tempo
    );
    

private:
    void prepararParaDormir();
};

#endif