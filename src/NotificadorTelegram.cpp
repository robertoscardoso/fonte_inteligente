// NotificadorTelegram.cpp

#include <Arduino.h>
#include "NotificadorTelegram.h"

// Implementação do Construtor
NotificadorTelegram::NotificadorTelegram(const char* token, const char* chatId)
    : _botToken(token), _chatId(chatId), _inicioQuedaEnergia(0) { // <-- Inicializa a nova variável com 0
    // Cria a instância do bot dinamicamente
    _bot = new UniversalTelegramBot(_botToken, _client_telegram);
}

// Implementação do método de inicialização
void NotificadorTelegram::inicializar() {
    // Configura o certificado raiz para a conexão segura
    _client_telegram.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

// Envia a mensagem de "Sistema Online"
void NotificadorTelegram::enviarMensagemInicializacao() {
    String mensagem = "✅ Sistema de Monitoramento de Energia iniciado e online.";
    _bot->sendMessage(_chatId, mensagem, "");
}

// Formata e envia a mensagem de queda de energia
void NotificadorTelegram::enviarAlertaQuedaEnergia(String dataHora, float nivelBateria) {
    // Armazena o momento exato da queda de energia
    _inicioQuedaEnergia = millis();

    String mensagem = "⚠️ *ALERTA: Queda de Energia!*\n\n";
    mensagem += "Rede externa desativada. Acionando bateria de backup.\n\n";
    mensagem += "*Nível da Bateria:* " + String(nivelBateria, 1) + "%\n";
    mensagem += "*Data/Hora:* " + dataHora; // Corrigido de "NEWOData/Hora" para "Data/Hora"
    _bot->sendMessage(_chatId, mensagem, "Markdown");
}

// Formata e envia a mensagem de retorno da energia
void NotificadorTelegram::enviarAvisoRetornoEnergia(String dataHora) {
    String mensagem = "✅ *Energia da Rede Externa RESTABELECIDA!*\n\n";

    // Calcula e adiciona o tempo que ficou sem energia
    if (_inicioQuedaEnergia > 0) {
        unsigned long duracaoMillis = millis() - _inicioQuedaEnergia;
        unsigned long duracaoSegundos = duracaoMillis / 1000;
        
        int horas = duracaoSegundos / 3600;
        int minutos = (duracaoSegundos % 3600) / 60;
        int segundos = duracaoSegundos % 60;

        String tempoDecorrido = "Tempo sem energia: ";
        if (horas > 0) {
            tempoDecorrido += String(horas) + "h ";
        }
        if (minutos > 0 || horas > 0) { // Mostra minutos se houver horas também
            tempoDecorrido += String(minutos) + "m ";
        }
        tempoDecorrido += String(segundos) + "s";
        
        mensagem += tempoDecorrido + "\n\n";

        // Reseta o marcador para a próxima queda
        _inicioQuedaEnergia = 0;
    }

    mensagem += "Data/Hora: " + dataHora;
    _bot->sendMessage(_chatId, mensagem, "Markdown");
}