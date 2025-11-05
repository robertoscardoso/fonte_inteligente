let gauge; // Variável global para o objeto do medidor

// --- 1. FUNÇÕES DO MEDIDOR (GAUGE) ---
function inicializarMedidor() {
    const opts = {
        angle: 0.1, lineWidth: 0.3, radiusScale: 1,
        pointer: { length: 0.5, strokeWidth: 0.035, color: '#333' },
        limitMax: 100, limitMin: 0,
        colorStart: '#d9534f', colorStop: '#5cb85c',
        strokeColor: '#E0E0E0', generateGradient: true, highDpiSupport: true,
        staticLabels: { font: "10px sans-serif", labels: [0, 25, 50, 75, 100], color: "#555" },
        staticZones: [
           {strokeStyle: "#d9534f", min: 0, max: 20},  // Vermelho
           {strokeStyle: "#f0ad4e", min: 20, max: 50}, // Laranja
           {strokeStyle: "#5cb85c", min: 50, max: 100} // Verde
        ],
    };
    const target = document.getElementById('gauge-bateria');
    gauge = new Gauge(target).setOptions(opts);
    gauge.maxValue = 100;
    gauge.setMinValue(0);
    gauge.animationSpeed = 40;
    gauge.set(0); // Valor inicial
}

// --- 2. BUSCA DE DADOS EM TEMPO REAL ---
function buscarDados() {
    fetch('/dados') // Busca dados da rota /dados (JSON em tempo real)
        .then(response => response.json())
        .then(data => {
            // Atualiza o Status da Rede (texto e cor)
            const statusRedeEl = document.getElementById('status-rede');
            statusRedeEl.textContent = data.statusRede;
            statusRedeEl.className = "value " + data.statusRede; 

            // Atualiza a Bateria (texto e medidor)
            document.getElementById('perc-bateria').textContent = data.percBateria.toFixed(1);
            if (gauge) {
                gauge.set(data.percBateria); 
            }

            // --- ADIÇÃO: ATUALIZA O CARD DE CICLOS ---
            document.getElementById('ciclos-bateria').textContent = data.ciclos.toFixed(2); // Mostra com 2 decimais

        })
        .catch(error => {
            console.error('Erro ao buscar dados em tempo real:', error);
            document.getElementById('status-rede').textContent = 'Erro';
            document.getElementById('perc-bateria').textContent = '--';
            document.getElementById('ciclos-bateria').textContent = '--'; // ADIÇÃO
            if (gauge) gauge.set(0);
        });
}

// --- 3. BUSCA DO HISTÓRICO DO BANCO DE DADOS ---
function buscarHistorico() {
    fetch('/historico') // Busca dados da nova rota /historico (JSON do DB)
        .then(response => response.json())
        .then(data => {
            const corpoTabela = document.getElementById('corpo-tabela-historico');
            corpoTabela.innerHTML = ""; 

            data.forEach(log => {
                let linha = document.createElement('tr');
                linha.innerHTML = `
                    <td>${log.data_hora}</td>
                    <td>${log.tipo === 'QUEDA' ? ' Energia Caiu' : ' Energia Retornou'}</td>
                    <td>${log.bateria_perc.toFixed(1)} %</td>
                `;
                corpoTabela.appendChild(linha);
            });
        })
        .catch(error => {
            console.error('Erro ao buscar histórico:', error);
            const corpoTabela = document.getElementById('corpo-tabela-historico');
            corpoTabela.innerHTML = `<tr><td colspan="3">Erro ao carregar histórico.</td></tr>`;
        });
}

// --- 4. INICIALIZAÇÃO ---
document.addEventListener('DOMContentLoaded', () => {
    inicializarMedidor();  // Prepara o medidor
    buscarDados();         // Busca os dados em tempo real pela 1ª vez
    buscarHistorico();     // Busca o histórico do DB pela 1ª vez

    // Define o intervalo para atualizar os dados em tempo real
    setInterval(buscarDados, 5000); // Atualiza a cada 5 segundos

    // --- LÓGICA DO BOTÃO DE RESET ---
    document.getElementById('btn-reset-ciclos').addEventListener('click', function() {
        if (confirm("Você tem certeza que quer zerar a contagem de ciclos?\nIsso deve ser feito apenas ao trocar a bateria.")) {
            
            fetch('/resetar-ciclos') // Chama a nova rota no ESP32
                .then(response => response.text())
                .then(message => {
                    alert(message); // Mostra a mensagem de sucesso (ex: "Contagem zerada...")
                    buscarDados(); // Atualiza os dados imediatamente para mostrar "0.00"
                })
                .catch(error => {
                    console.error('Erro ao zerar ciclos:', error);
                    alert('Erro ao tentar zerar os ciclos.');
                });
        }
    });
});