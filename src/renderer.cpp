#include "renderer.h"
#include <algorithm>
void PongRenderer::render(const GameState &state, Framebuffer &fb)
{
    fb.clear();

    int p1_y = (int)(state.p1._y_position * _height);
    int p1_h = (int)(state.p1._size * _height);
    for (int y = p1_y; y < p1_y + p1_h; ++y) {
        if (y >= 0 && y < _height) {
            fb.set_pixel(0, y, Color{0, 0, 255});
        }
    }

    int p2_y = (int)(state.p2._y_position * _height);
    int p2_h = (int)(state.p2._size * _height);
    for (int y = p2_y; y < p2_y + p2_h; ++y) {
        if (y >= 0 && y < _height) {
            fb.set_pixel(_width - 1, y, Color{255, 0, 0});
        }
    }

    int bx = (int)(state.ball._x_position * _width);
    int by = (int)(state.ball._y_position * _height);

    int ball_w = std::max(1, (int)(state.ball._size * _width));
    int ball_h = std::max(1, (int)(state.ball._size * _height));

    for (int x = 0; x < ball_w; ++x) {
        for (int y = 0; y < ball_h; ++y) {
            
            int px = bx + x;
            int py = by + y;

            if (px >= 0 && px < _width && py >= 0 && py < _height) {
                fb.set_pixel(px, py, Color{255, 255, 255});
            }
        }
    }
}