#include "Arduino.h"
#include "Energia.h"
#include "RedeExterna.h"
#include "Bateria.h"

// -------------------------------------------------------------
// DECLARAÇÕES GLOBAIS
// -------------------------------------------------------------
Bateria B_18650(0, 3.2, 0.0);
RedeExterna tomada(3);

// -----------------------------------------------------------------------------
// 1. FUNÇÃO AUXILIAR PARA O ENUM DA TOMADA
// -----------------------------------------------------------------------------
String statusTomadaToString(statusRedeExterna status)
{
    switch (status)
    {
    case ATIVADO:
        return "ATIVADA";
    case DESATIVADO:
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
void setup()
{
    delay(3000);
    Serial.begin(115200);
    Serial.println("Sistema de Monitoramento de Energia Inicializado!");

    Energia::inicializar();
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

void loop()
{
    // --- OBTENÇÃO DOS DADOS DE AMBAS AS FONTES ---
    // Dados da Bateria
    float tensaoBateria = B_18650.getTensao();
    float porcentagem = B_18650.getPorcentagem(); // Isso também atualiza o status da bateria
    String statusBateria = statusBateriaToString(B_18650.getStatus());

    // Dados da Rede Externa (Tomada)
    float tensaoTomada = tomada.getTensao();
    String statusTomada = statusTomadaToString(tomada.getStatus());

    // --- IMPRESSÃO DOS DADOS ---
    Serial.println("\n=========================================");
    Serial.println("        MONITORAMENTO DE ENERGIA         ");
    Serial.println("=========================================");

    // --- INDICAÇÃO DA FONTE DE ENERGIA ATIVA ---
    // A lógica para decidir a fonte ativa permanece a mesma.
    if (tensaoTomada > 0.2)
    {
        Serial.println(" > FONTE ATIVA: REDE EXTERNA <");
        tomada.setStatus(ATIVADO);
        B_18650.setStatus(DESATIVADA);
    }
    else
    {
        Serial.println(" > FONTE ATIVA: BATERIA <");
        tomada.setStatus(DESATIVADO);
        B_18650.setStatus(ATIVADA);
    }

    Serial.println("-----------------------------------------");

    // --- DADOS DA REDE EXTERNA (SEMPRE EXIBIDOS) ---
    Serial.println("DADOS DA REDE EXTERNA:");
    Serial.println(" -> Status:       " + statusTomada);
    Serial.println(" -> Tensão Atual:  " + String(tensaoTomada, 1) + "V");

    Serial.println("-----------------------------------------");

    // --- DADOS DA BATERIA (SEMPRE EXIBIDOS) ---
    Serial.println("DADOS DA BATERIA (18650):");
    Serial.println(" -> Status:       " + statusBateria);
    Serial.println(" -> Tensão Atual:  " + String(tensaoBateria, 2) + "V");
    Serial.println(" -> Porcentagem:  " + String(porcentagem, 1) + "%");

    Serial.println("=========================================\n");

    delay(1000); // Espera 1 segundo antes da próxima atualização.
}