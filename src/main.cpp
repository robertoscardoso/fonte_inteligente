#include "Arduino.h"
#include "Energia.h"
#include "RedeExterna.h"
#include "Bateria.h"

// -------------------------------------------------------------
// DECLARAÇÕES GLOBAIS
// -------------------------------------------------------------
Bateria B_18650(0, 4.2, 2.7);
RedeExterna tomada(1);

// -----------------------------------------------------------------------------
// 1. FUNÇÃO AUXILIAR PARA O ENUM DA TOMADA
// -----------------------------------------------------------------------------
String statusTomadaToString(statusRedeExterna status)
{
    switch (status)
    {
    case ATIVADA:
        return "ATIVADA";
    case DESATIVADA:
        return "DESATIVADA";
    default:
        return "DESCONHECIDO";
    }
}

// -----------------------------------------------------------------------------
// 2. FUNÇÃO AUXILIAR PARA O ENUM DA BATERIA (CORREÇÃO DO SEU ERRO)
// -----------------------------------------------------------------------------
String statusBateriaToString(BateriaStatus status)
{
    switch (status)
    {
    case ATIVADA:
        return "ATIVADA";
    case DESATIVADA:
        return "DESATIVADA";
    default:
        return "DESCONHECIDO";
    }
}
// -----------------------------------------------------------------------------

void loop()
{
    // --- OBTENÇÃO DOS DADOS DA BATERIA ---
    float tensaoBateria = B_18650.getTensao(); // Presumindo que este método já existe
    float porcentagem = B_18650.getPorcentagem();
    String statusBateria = statusBateriaToString(B_18650.getStatus());

    // --- OBTENÇÃO DOS DADOS DA TOMADA (NOVO DADO!) ---
    String statusTomada = statusTomadaToString(tomada.getStatus());
    float tensaoTomada = tomada.getTensao(); // <-- CHAMADA DO NOVO MÉTODO

    // --- IMPRESSÃO DOS DADOS ---

    Serial.println("\n=========================================");
    Serial.println("            DADOS DO SISTEMA             ");
    Serial.println("=========================================");

    // Dados da Rede Externa (Tomada)
    Serial.println("STATUS DA REDE EXTERNA:");
    Serial.println(" -> Status:      " + statusTomada);
    // Exibe a tensão da tomada
    Serial.println(" -> Tensão Atual: " + String(tensaoTomada, 1) + "V");

    Serial.println("-----------------------------------------");

    // Dados da Bateria
    Serial.println("DADOS DA BATERIA (18650):");
    Serial.println(" -> Status:      " + statusBateria);
    Serial.println(" -> Tensão Atual: " + String(tensaoBateria, 2) + "V");
    Serial.println(" -> Porcentagem: " + String(porcentagem, 1) + "%");

    Serial.println("=========================================\n");

    delay(5000); // Espera 5 segundos
}