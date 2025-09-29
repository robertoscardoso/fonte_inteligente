#include "Arduino.h"
#include <Adafruit_ADS1X15.h>
#include "Energia.h"


Adafruit_ADS1115 Energia::adc;
bool Energia::inicializado = false;



Energia::Energia(uint8_t canal): canal(canal){
    if(!inicializado){
        if(!adc.begin()){
            Serial.println("Erro ao inicializar o ADC externo");
        }
        adc.setGain(GAIN_TWO);
        inicializado = true;
    }
}



int16_t Energia::getValorBruto(){

    return adc.readADC_SingleEnded(this->canal);
}



float Energia::getTensao() {
    int16_t valorBruto = getValorBruto();

    // Conversão depende do ganho configurado
    float multiplier = 0.0;

    switch (adc.getGain()) {
        case GAIN_TWOTHIRDS: multiplier = 0.1875F; break; // ±6.144V
        case GAIN_ONE:       multiplier = 0.125F;  break; // ±4.096V
        case GAIN_TWO:       multiplier = 0.0625F; break; // ±2.048V
        case GAIN_FOUR:      multiplier = 0.03125F;break; // ±1.024V
        case GAIN_EIGHT:     multiplier = 0.015625F;break;// ±0.512V
        case GAIN_SIXTEEN:   multiplier = 0.0078125F;break;// ±0.256V
    }

    return valorBruto * multiplier / 1000.0F; // converte mV -> V
}