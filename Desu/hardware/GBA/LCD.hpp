#pragma once
#include <stdint.h>
#include <SDL.h>
#include <glew.h>
#include <string>
#include "GBAMemory.hpp"

struct LCD {
    static const uint16_t 
        SCALE = 6,
        WIDTH = 240,
        HEIGHT = 160;

    uint8_t fps;
    uint8_t secondsElapsed = SDL_GetTicks() / 1000;

    GBAMemory* systemMemory;
    SDL_Window* window;
    
    LCD(GBAMemory*);

    // todo: put lines in hblank and draw in vblank
    
    // draw a frame
    void draw(uint8_t*);

    void loadShaders();
};