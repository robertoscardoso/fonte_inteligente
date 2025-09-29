#ifndef BATERIA_H
#define BATERIA_H

#include "Arduino.h"
#include "Energia.h"

enum BateriaStatus
{
    ATIVADA,
    DESATIVADA
};

class Bateria : public Energia
{

    public:
        Bateria(uint8_t canal, float voltagem_maxima, float voltagem_minima);

        void setStatus(BateriaStatus novoStatus);
        BateriaStatus getStatus();

        float getPorcentagem();

    private:
        float calcularPorcentagem();
        float MAX_VOLTAGEM;
        float MIN_VOLTAGEM;
        BateriaStatus statusBateria;
};

#endif