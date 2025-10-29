#ifndef REDE_EXTERNA_H
#define REDE_EXTERNA_H

#include <Arduino.h>
#include "Energia.h"

enum statusRedeExterna{
    ATIVADO,
    DESATIVADO
};

class RedeExterna: public Energia{
    public:
        RedeExterna(uint8_t canal);

        statusRedeExterna getStatus();

        void setStatus(statusRedeExterna status);

    private:
        statusRedeExterna statusAtual;
};
#endif