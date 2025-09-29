#include "Bateria.h"
#include "Arduino.h"

Bateria::Bateria(uint8_t canal, float voltagem_maxima, float voltagem_minima)
    : Energia(canal), MAX_VOLTAGEM(voltagem_maxima), MIN_VOLTAGEM(voltagem_minima)
{
    this->statusBateria = DESATIVADA;
}


float Bateria::calcularPorcentagem(){
    float tensao_atual = getTensao();

    float faixa_tensao = MAX_VOLTAGEM - MIN_VOLTAGEM;

    if(tensao_atual <=MIN_VOLTAGEM){
        return 0.0;
    }

    if(tensao_atual>=MAX_VOLTAGEM){
        return 100.0;
    }

    float porcentagem = ((tensao_atual - MIN_VOLTAGEM) / faixa_tensao) * 100.0;

    return porcentagem;
}

BateriaStatus Bateria::getStatus(){
    return statusBateria;
}

void Bateria::setStatus(BateriaStatus novoStatus){
    this->statusBateria = novoStatus;
}

float Bateria::getPorcentagem(){
    return calcularPorcentagem();
}

