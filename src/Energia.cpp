#include <Arduino.h>
#include "Energia.h"
#include <Wire.h>

Adafruit_ADS1115 Energia::adc;
bool Energia::inicializado = false;

Energia::Energia(uint8_t canal) : canal(canal)
{
}

// ---------------------------------------------------------
// Inicializa o ADC externo apenas uma vez, após o Wire estar ativo
// ---------------------------------------------------------
void Energia::inicializar()
{
    if (!inicializado)
    {

        if (!adc.begin(0X48))
        {
            Serial.println("Erro ao inicializar o ADC externo (ADS1115)");
            return;
        }
        inicializado = true;
        adc.setGain(GAIN_TWOTHIRDS);
        Serial.println("ADC externo (ADS1115) inicializado com sucesso!");
        delay(2000);
    }
}

// ---------------------------------------------------------
int16_t Energia::getValorBruto()
{
    if (!inicializado)
    {
        Serial.println("ADC não inicializado!");
        return 0;
    }
    return adc.readADC_SingleEnded(this->canal);
}

// ---------------------------------------------------------
float Energia::getTensao()
{
    if (!inicializado)
    {
        Serial.println("ADC não inicializado!");
        return 0.0;
    }

    int16_t valorBruto = getValorBruto();

    return adc.computeVolts(valorBruto);;
}
