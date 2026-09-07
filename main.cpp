#include <WiFi.h>
#include <WebSocketsClient.h>
#include <FastLED.h>
#include <WiFiManager.h>  
#include <Preferences.h>  
#include "game.h"
#include "framebuffer.h"
#include "renderer.h"

#define MATRIX_W 16
#define MATRIX_H 16
#define NUM_LEDS 256
#define LED_PIN 13
#define COLOR_ORDER GRB
#define LED_TYPE WS2812B
#define BRIGHTNESS 80

CRGB leds[NUM_LEDS];

char cloudflare_host[100] = "cole-seu-link-aqui.trycloudflare.com"; 
const int server_port = 443; 

WebSocketsClient webSocket;
Preferences preferences;

GameState game;
Framebuffer fb(MATRIX_W, MATRIX_H);
PongRenderer renderer(MATRIX_W, MATRIX_H);

unsigned long last_micros = 0;

const uint8_t *current_payload = nullptr;
size_t current_length = 0;

uint16_t getIndex(uint8_t x, uint8_t y) {
    if (x >= MATRIX_W || y >= MATRIX_H) return 0;
    if (y % 2 == 0) return y * MATRIX_W + x;
    return y * MATRIX_W + (MATRIX_W - 1 - x);
}

class FastLEDDriver {
public:
    void present(const Framebuffer &fb) {
        for (int y = 0; y < fb.height; y++) {
            for (int x = 0; x < fb.width; x++) {
                Color c = fb.get_pixel(x, y);
                leds[getIndex(x, y)] = CRGB(c.r, c.g, c.b);
            }
        }
        FastLED.show();
    }
};

FastLEDDriver display;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch(type) {
        case WStype_BIN:
            current_payload = payload;
            current_length = length;
            break;
        case WStype_DISCONNECTED:
            game.n_players = 0;
            break;
        default:
            break;
    }
}

void setup() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    FastLED.show();

    preferences.begin("pong-app", false);
    String savedHost = preferences.getString("cf_host", cloudflare_host);
    strncpy(cloudflare_host, savedHost.c_str(), sizeof(cloudflare_host));

    WiFiManager wm;
    WiFiManagerParameter custom_cf_host("server", "Link", cloudflare_host, 100);
    wm.addParameter(&custom_cf_host);

    if (!wm.autoConnect("ESP32-Pong-Setup")) {
        ESP.restart();
    }

    if (String(cloudflare_host) != String(custom_cf_host.getValue())) {
        strncpy(cloudflare_host, custom_cf_host.getValue(), sizeof(cloudflare_host));
        preferences.putString("cf_host", cloudflare_host);
    }

    webSocket.beginSSL(cloudflare_host, server_port, "/esp32");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);

    last_micros = micros();
}

void loop() {
    webSocket.loop();

    unsigned long now = micros();
    float dtime = (now - last_micros) / 1000000.0f;
    last_micros = now;

    game.update(dtime, current_payload, current_length);
    current_payload = nullptr;
    current_length = 0;

    renderer.render(game, fb);
    display.present(fb);
}