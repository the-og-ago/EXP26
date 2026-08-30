const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { WebSocketServer } = require('ws');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: "*", methods: ["GET", "POST"] } });

app.use(express.static(path.join(__dirname, 'public')));

// -------------------------------------------------------------
// 1. CONFIGURAÇÃO GLOBAL E MEMÓRIA DE ESTADO
// -------------------------------------------------------------
const MAX_PLAYERS = 3; // O servidor ajusta as vagas e o tamanho do pacote dinamicamente[cite: 3]

// Controla as vagas (assentos)[cite: 3]
const seats = new Array(MAX_PLAYERS).fill(null);

// Memória Central do Servidor (Fonte Única de Verdade)
// Inicia todos os jogadores na posição neutra: Centro (127) e Botão solto (0)
const playerInputs = new Array(MAX_PLAYERS).fill(null).map(() => ({ x: 127, y: 127, btn: 0 }));

// -------------------------------------------------------------
// 2. RECEPÇÃO DE CONTROLES (SOCKET.IO)
// -------------------------------------------------------------
io.on('connection', (socket) => {
    socket.playerIndex = -1; // -1 indica que é o Simulador ou Espectador[cite: 3]

    // Sistema dinâmico de alocação de vagas[cite: 3]
    socket.on('join', (kind) => {
        if (kind !== 'controller') return;

        const freeIndex = seats.findIndex(seat => seat === null);

        if (freeIndex !== -1) {
            seats[freeIndex] = socket.id;
            socket.playerIndex = freeIndex; 
            socket.role = `Player ${freeIndex + 1}`;
        } else {
            socket.role = 'Espectador';
        }

        socket.emit('role', { role: socket.role, index: socket.playerIndex });
    });

    // O input do celular (independente de ser acelerômetro, botão ou analógico) 
    socket.on('move', (data) => {
        if (socket.playerIndex === -1) return; 
        
        // Recebe os eixos neutros (-1.0 a 1.0) e botões
        const axes = data.axes || [0, 0];
        const buttons = data.buttons || [0];

        // Converte de escala float (-1.0 a +1.0) para 1 Byte (0 a 255)
        const byteX = Math.max(0, Math.min(255, Math.floor(((axes[0] + 1) / 2) * 255)));
        const byteY = Math.max(0, Math.min(255, Math.floor(((axes[1] + 1) / 2) * 255)));
        const byteBtn = buttons[0] ? 1 : 0;

        // Apenas ATUALIZA A MEMÓRIA, não envia para a rede ainda!
        playerInputs[socket.playerIndex] = { x: byteX, y: byteY, btn: byteBtn };
    });

    socket.on('disconnect', () => {
        if (socket.playerIndex !== -1 && seats[socket.playerIndex] === socket.id) {
            seats[socket.playerIndex] = null;
            // Retorna o input deste jogador para o centro ao desconectar
            playerInputs[socket.playerIndex] = { x: 127, y: 127, btn: 0 };
        }
    });
});

// -------------------------------------------------------------
// 3. CONEXÃO COM O ESP32 (WEBSOCKET NATIVO)
// -------------------------------------------------------------
const wss = new WebSocketServer({ server, path: '/esp32' }); // Mantido o caminho nativo[cite: 3]
const espClients = new Set();
const PORT = 3001; // Mantida a porta original[cite: 3]

wss.on('connection', (ws) => {
    console.log('ESP32 conectado');
    espClients.add(ws);
    ws.on('close', () => espClients.delete(ws));
});

function broadcastToESP(buffer) {
    for (const ws of espClients) {
        if (ws.readyState === ws.OPEN) {
            ws.send(buffer, { binary: true }); // Transmite em formato binário[cite: 3]
        }
    }
}

// -------------------------------------------------------------
// 4. MOTOR DE REDE (TICK RATE UNIFICADO A 30 FPS)
// -------------------------------------------------------------
const TICK_RATE = 30; 
const TICK_INTERVAL = 1000 / TICK_RATE; 

setInterval(() => {
    // 1. Atualiza o Simulador
    // O simulador recebe o array limpo, eliminando o lag visual no navegador
    io.emit('update_simulator', playerInputs);

    // 2. Atualiza o ESP32 (Se estiver conectado)
    if (espClients.size > 0) {
        const bytes = [];
        
        // Conta quantos jogadores estão ativamente conectados
        const activePlayers = seats.filter(seat => seat !== null).length;
        
        // BYTE 0: Informa a quantidade de players ativos
        bytes.push(activePlayers & 0xFF); 
        
        // BYTES SEGUINTES: 3 Bytes (X, Y e Botão) para todos os slots (MAX_PLAYERS)
        // Isso mantém o tamanho do pacote sempre fixo = 1 + (MAX_PLAYERS * 3) bytes
        for (let i = 0; i < MAX_PLAYERS; i++) {
            bytes.push(playerInputs[i].x & 0xFF);
            bytes.push(playerInputs[i].y & 0xFF);
            bytes.push(playerInputs[i].btn & 0xFF);
        }

        broadcastToESP(Buffer.from(bytes));
    }

}, TICK_INTERVAL);

server.listen(PORT, () => console.log(`Servidor de Inputs rodando na porta ${PORT} a ${TICK_RATE} FPS`));