#include "SaudeBateria.h"
#include <Arduino.h>

SaudeBateria::SaudeBateria() {
    // Inicialização básica
    ciclos_cumulativos = 0.0;
    estadoAtual = CARREGANDO; // Assume que começa carregando
    percentual_pico = 100.0;
}

void SaudeBateria::iniciar() {
    preferences.begin("monitor_ciclos", false); 
    
    // Carrega os valores salvos
    this->ciclos_cumulativos = preferences.getFloat("ciclos_cumul", 0.0);
    this->estadoAtual = (BateriaCicloEstado)preferences.getInt("ciclo_estado", CARREGANDO);
    this->percentual_pico = preferences.getFloat("perc_pico", 100.0);

    Serial.printf("✅ Gerenciador de Ciclos iniciado. Ciclos: %.2f, Estado: %d\n", 
        this->ciclos_cumulativos, this->estadoAtual);
}

void SaudeBateria::atualizar(float percentualAtual, bool redeExternaAtiva) {
    BateriaCicloEstado novoEstado = redeExternaAtiva ? CARREGANDO : DESCARREGANDO;

    if (novoEstado == DESCARREGANDO && this->estadoAtual == CARREGANDO) {
        // --- TRANSIÇÃO: ACABOU DE CAIR A ENERGIA ---
        // A bateria estava carregando e agora vai descarregar
        
        this->estadoAtual = DESCARREGANDO;
        this->percentual_pico = percentualAtual; // Salva o % no momento da queda

        // Salva o estado e o pico na memória
        preferences.putInt("ciclo_estado", DESCARREGANDO);
        preferences.putFloat("perc_pico", this->percentual_pico);

        Serial.printf("### CICLO: Queda de energia. Iniciando descarga de %.1f%% ###\n", this->percentual_pico);
    }
    else if (novoEstado == CARREGANDO && this->estadoAtual == DESCARREGANDO) {
        // --- TRANSIÇÃO: ENERGIA ACABOU DE VOLTAR ---
        // A bateria estava descarregando e agora vai carregar
        
        this->estadoAtual = CARREGANDO;
        
        // CALCULA A FRAÇÃO DO CICLO
        float percentual_gasto = this->percentual_pico - percentualAtual;
        
        // Garante que não contamos "cargas" (ex: caiu em 80%, voltou em 82%)
        if (percentual_gasto < 0) {
            percentual_gasto = 0;
        }
        
        // Converte o percentual gasto (0-100) para uma fração (0.0 - 1.0)
        float ciclo_fracionario = percentual_gasto / 100.0;

        // Adiciona ao total e salva
        this->ciclos_cumulativos += ciclo_fracionario;
        preferences.putFloat("ciclos_cumul", this->ciclos_cumulativos);
        preferences.putInt("ciclo_estado", CARREGANDO);

        Serial.printf("### CICLO: Retorno da energia. Fração adicionada: %.2f (Total: %.2f) ###\n", 
            ciclo_fracionario, this->ciclos_cumulativos);
    }
    // Se o estado for o mesmo (ex: continua carregando ou continua descarregando), 
    // não fazemos nada, apenas esperamos a próxima transição.
}

float SaudeBateria::getCiclosCumulativos() {
    return this->ciclos_cumulativos;
}

void SaudeBateria::resetarCiclos() {
    this->ciclos_cumulativos = 0.0;
    this->estadoAtual = CARREGANDO; // Reseta para o estado "carregando"
    this->percentual_pico = 100.0;

    preferences.putFloat("ciclos_cumul", 0.0);
    preferences.putInt("ciclo_estado", CARREGANDO);
    preferences.putFloat("perc_pico", 100.0);
    
    Serial.println("Contagem de ciclos cumulativos ZERADA.");
}