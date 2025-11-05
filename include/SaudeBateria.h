#ifndef SAUDEBATERIA_H
#define SAUDEBATERIA_H

#include <Preferences.h>

enum BateriaCicloEstado {
    CARREGANDO,     // Rede externa ligada
    DESCARREGANDO   // Rede externa desligada
};

class SaudeBateria {
public:
    SaudeBateria(); 

    void iniciar();

    // O 'atualizar' recebe o % atual E se a rede está ativa
    void atualizar(float percentualAtual, bool redeExternaAtiva); 

    // ciclos parciais da bateria 
    float getCiclosCumulativos(); 

    void resetarCiclos();

private:
    Preferences preferences;
    
    BateriaCicloEstado estadoAtual; // Estado salvo (CARREGANDO ou DESCARREGANDO)
    float ciclos_cumulativos;     // contador fracionário (ex: 2.75)
    float percentual_pico;        // O % que a bateria tinha quando a energia caiu
};

#endif