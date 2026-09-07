#pragma once
#include <stdint.h>

struct Color { 
    uint8_t r, g, b; 
};

class Framebuffer {
public:
    const uint16_t width, height;
    Color* pixels;

    Framebuffer(uint16_t w, uint16_t h);
    ~Framebuffer();

    void set_pixel(uint16_t x, uint16_t y, Color color);
    Color get_pixel(uint16_t x, uint16_t y) const;
    void clear();
};