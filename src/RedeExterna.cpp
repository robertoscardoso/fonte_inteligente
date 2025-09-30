#include "Arduino.h"
#include "RedeExterna.h"

RedeExterna::RedeExterna(uint8_t canal)
    :Energia(canal){
        this->statusAtual = DESATIVADA;
}


statusRedeExterna RedeExterna::getStatus(){
    return statusAtual;
}

void RedeExterna::setStatus(statusRedeExterna status){
    this->statusAtual = status;
}