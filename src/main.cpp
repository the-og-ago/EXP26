#include <WiFi.h>
#include <WebSocketsClient.h>
#include <FastLED.h>
#include <WiFiManager.h>  
#include <Preferences.h>  

/* ===================== CONFIGURAÇÃO DOS LEDS ===================== */
#define MATRIX_W 16
#define MATRIX_H 16
#define NUM_LEDS 256
#define LED_PIN 13
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B
#define BRIGHTNESS 80

CRGB leds[NUM_LEDS];

/* ===================== CONFIGURAÇÃO DE REDE DINÂMICA ===================== */
char cloudflare_host[100] = "cole-seu-link-aqui.trycloudflare.com"; 
const int server_port = 443; 

WebSocketsClient webSocket;
Preferences preferences;

/* ===================== VARIÁVEIS DO PONG E LERP ===================== */
#define PADDLE_HEIGHT 3

float target_p1_y = 6;
float target_p2_y = 6;

float p1_y = 6;
float p2_y = 6;

float ball_x = 8;
float ball_y = 8;
float ball_dx = 0.5;
float ball_dy = 0.3;

unsigned long lastGameUpdate = 0;
#define GAME_SPEED 50 

/* ===================== MAPEAMENTO SERPENTINO ===================== */
uint16_t getIndex(uint8_t x, uint8_t y) {
  if (x >= MATRIX_W || y >= MATRIX_H) return 0;
  if (y % 2 == 0) return y * MATRIX_W + x;
  return y * MATRIX_W + (MATRIX_W - 1 - x);
}

/* ===================== EVENTOS DO WEBSOCKET (BINÁRIO) ===================== */
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED:
      Serial.println("[ESP32] Conectado ao Túnel Cloudflare!");
      break;

    case WStype_BIN:
      // Pacote enviado pelo server.js: [Active_Players, P1_X, P1_Y, P1_Btn, P2_X, P2_Y, P2_Btn, ...]
      if (length >= 7) { 
        // Byte 2 = Padrão de leitura da posição Y do Player 1 (0-255 -> 13-0)
        target_p1_y = map(payload[2], 0, 255, 13, 0);
        
        // Byte 5 = Padrão de leitura da posição Y do Player 2 (0-255 -> 13-0)
        target_p2_y = map(payload[5], 0, 255, 13, 0);
      }
      break;
      
    case WStype_DISCONNECTED:
      Serial.println("[ESP32] Desconectado do Servidor!");
      break;
  }
}

/* ===================== FÍSICA DO JOGO ===================== */
void resetBall() {
  ball_x = 8;
  ball_y = 8;
  ball_dx = (random(0, 2) == 0 ? 0.5 : -0.5);
  ball_dy = (random(-3, 4) / 10.0);
}

void updatePong() {
  // LERP: Suavização de Movimento na renderização
  p1_y += (target_p1_y - p1_y) * 0.3;
  p2_y += (target_p2_y - p2_y) * 0.3;

  ball_x += ball_dx;
  ball_y += ball_dy;

  if (ball_y <= 0 || ball_y >= MATRIX_H - 1) {
    ball_dy *= -1;
    ball_y = constrain(ball_y, 0, MATRIX_H - 1);
  }

  if (ball_x <= 1) {
    if (ball_y >= p1_y && ball_y <= p1_y + PADDLE_HEIGHT) {
      ball_dx *= -1;
      ball_x = 1;
    } else if (ball_x <= 0) {
      resetBall();
    }
  }

  if (ball_x >= MATRIX_W - 2) {
    if (ball_y >= p2_y && ball_y <= p2_y + PADDLE_HEIGHT) {
      ball_dx *= -1;
      ball_x = MATRIX_W - 2;
    } else if (ball_x >= MATRIX_W - 1) {
      resetBall();
    }
  }
}

/* ===================== RENDERIZAÇÃO ===================== */
void drawPong() {
  FastLED.clear();

  // Raquete Player 1 (Azul)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    int y = (int)p1_y + i;
    if (y >= 0 && y < MATRIX_H) leds[getIndex(0, y)] = CRGB::Blue;
  }

  // Raquete Player 2 (Vermelha)
  for (int i = 0; i < PADDLE_HEIGHT; i++) {
    int y = (int)p2_y + i;
    if (y >= 0 && y < MATRIX_H) leds[getIndex(MATRIX_W - 1, y)] = CRGB::Red;
  }

  // Bola (Branca)
  leds[getIndex((uint8_t)ball_x, (uint8_t)ball_y)] = CRGB::White;
  FastLED.show();
}

/* ===================== SETUP E LOOP ===================== */
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // 1. Carrega a URL da Cloudflare gravada na memória Flash
  preferences.begin("pong-app", false);
  String savedHost = preferences.getString("cf_host", cloudflare_host);
  strncpy(cloudflare_host, savedHost.c_str(), sizeof(cloudflare_host));

  // 2. Configura o Captive Portal (WiFiManager)
  WiFiManager wm;
  WiFiManagerParameter custom_cf_host("server", "Link Cloudflare (sem https://)", cloudflare_host, 100);
  wm.addParameter(&custom_cf_host);

  Serial.println("Tentando conectar ao WiFi...");
  
  // Se não encontrar nenhuma rede conhecida, levanta o Access Point "ESP32-Pong-Setup"
  if (!wm.autoConnect("ESP32-Pong-Setup")) {
    Serial.println("Falha na conexao. Reiniciando ESP32...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("\nWi-Fi Conectado!");

  // 3. Atualiza e salva o link da Cloudflare se tiver sido alterado pelo portal web
  if (String(cloudflare_host) != String(custom_cf_host.getValue())) {
    strncpy(cloudflare_host, custom_cf_host.getValue(), sizeof(cloudflare_host));
    preferences.putString("cf_host", cloudflare_host);
    Serial.println("Novo Host salvo permanentemente na memória Flash!");
  }

  // 4. Inicia WebSocket nativo
  Serial.print("Conectando ao WS: ");
  Serial.println(cloudflare_host);
  webSocket.beginSSL(cloudflare_host, server_port, "/esp32");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); 

  randomSeed(analogRead(0));
  resetBall();
}

void loop() {
  webSocket.loop();

  if (millis() - lastGameUpdate > GAME_SPEED) {
    lastGameUpdate = millis();
    updatePong();
    drawPong();
  }
}