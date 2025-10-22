// NotificadorTelegram.h

#ifndef NOTIFICADOR_TELEGRAM_H
#define NOTIFICADOR_TELEGRAM_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

class NotificadorTelegram {
public:
    // Construtor: recebe o token do bot e o chat ID quando a classe é criada
    NotificadorTelegram(const char* token, const char* chatId);

    // Método para inicializar o cliente seguro
    void inicializar();

    // Métodos específicos para enviar cada tipo de mensagem
    void enviarMensagemInicializacao();
    void enviarAlertaQuedaEnergia(String dataHora, float nivelBateria);
    void enviarAvisoRetornoEnergia(String dataHora);

private:
    // Membros privados para armazenar as credenciais e objetos
    const char* _botToken;
    const char* _chatId;

    WiFiClientSecure _client_telegram;
    UniversalTelegramBot* _bot;

    // NOVO: Variável para armazenar o tempo (em milissegundos) do início da queda
    unsigned long _inicioQuedaEnergia; 
};

#endif // NOTIFICADOR_TELEGRAM_H