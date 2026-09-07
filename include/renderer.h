#pragma once
#include "game.h"
#include "framebuffer.h"

struct PongRenderer
{
    int _width; 
    int _height;
    PongRenderer(int width, int height) : _width(width), _height(height) {}
    void render(const GameState &state, Framebuffer &fb);
};