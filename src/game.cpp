#include "game.h"
#include <stdint.h>
void Paddle::update_position(uint8_t payload)
{
    _y_position=(payload/255.0f)*(1.0f-_size);
}
void Ball::velocity(float velocityx,float velocityy)
{
    _x_velocity=velocityx;
    _y_velocity=velocityy;
}
void Ball::update_position(float dtime)
{
    _x_position+=_x_velocity*dtime;
    _y_position+=_y_velocity*dtime;
}
void Ball::xcolision(Paddle &player)
{   
    bool hit_y = (_y_position + _size >= player._y_position) && (_y_position <= player._y_position + player._size);
    bool hit_x = (_x_position + _size >= player._x_position) && (_x_position <= player._x_position + player._width);
    if (hit_x && hit_y)
        _x_velocity *= -1.0f;
}
void Ball::ycolision()
{
    if (_y_position <= 0.0f) {
        _y_position = 0.0f;
        _y_velocity *= -1.0f;
    } 
    else if (_y_position + _size >= 1.0f) {
        _y_position = 1.0f - _size;
        _y_velocity *= -1.0f;
    }
}
void Ball::reset()
{
    _x_position=(1.0f - _size) / 2.0f;
    _y_position=(1.0f - _size) / 2.0f;
}
void GameState::update(float dtime,const uint8_t *payload, size_t length)
{
    if (payload != nullptr && length >= 7) {
        n_players = payload[0];
        if (n_players >= 1) p1.update_position(payload[2]);
        if (n_players >= 2) p2.update_position(payload[5]);
    }
    ball.update_position(dtime);
    ball.xcolision(p1);
    ball.xcolision(p2);
    ball.ycolision();
    if (ball._x_position < 0.0f || ball._x_position > 1.0f) {
        ball.reset();
    }
}