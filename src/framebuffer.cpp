#include "framebuffer.h"

Framebuffer::Framebuffer(uint16_t w, uint16_t h) : width(w), height(h) {
    pixels = new Color[width * height];
    clear();
}

Framebuffer::~Framebuffer() {
    delete[] pixels;
}

void Framebuffer::set_pixel(uint16_t x, uint16_t y, Color color) {
    if (x < width && y < height) {
        pixels[y * width + x] = color;
    }
}

Color Framebuffer::get_pixel(uint16_t x, uint16_t y) const {
    if (x < width && y < height) {
        return pixels[y * width + x];
    }
    return Color{0, 0, 0};
}

void Framebuffer::clear() {
    for (uint16_t i = 0; i < width * height; i++) {
        pixels[i] = Color{0, 0, 0};
    }
}