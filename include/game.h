//pong
#pragma once
#include <stddef.h>
#include <stdint.h>
struct Paddle
{   
    float _size=0.3f;
    float _width = 0.02f;
    float _y_position=(1.0f - _size )/2.0f; //posicao relativa [0.0-1.0]
    float _x_position=0.0f;
    void update_position(uint8_t payload);
};
struct Ball
{
    float _size=0.02f;
    float _y_position=(1.0f - _size )/2.0f;
    float _x_position=(1.0f - _size )/2.0f;
    float _y_velocity=0.1f;
    float _x_velocity=0.2f;
    void velocity(float velocityx,float velocityy);
    void update_position(float dtime);
    void xcolision(Paddle &player);
    void ycolision();
    void reset();
};
struct GameState
{
    uint8_t n_players=0;
    Paddle p1;
    Paddle p2;
    Ball ball;
    GameState()
    {
        p2._x_position=(1.0f-p2._width);
        ball.reset();
    };
    void update(float dtime,const uint8_t *payload = nullptr, size_t length = 0);
};
