#ifndef ENERGIA_H
#define ENERGIA_H

#include <Arduino.h>
#include "stdint.h"
#include <Adafruit_ADS1X15.h>

class Energia
{
public:
    Energia(uint8_t canal);

    virtual int16_t getValorBruto();

    virtual float getTensao();
    static void inicializar();
    virtual ~Energia() {}

protected:
    uint8_t canal;
    static Adafruit_ADS1115 adc;
    static bool inicializado;
};

#endif