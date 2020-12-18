#pragma once
#include <stdint.h>
#include <SDL.h>
#include <glew.h>
#include "GBAMemory.hpp"

struct LCD {
    static const uint8_t 
        SCALE = 6,
        WIDTH = 240 * SCALE,
        HEIGHT = 160 * SCALE,
        FPS = 60; // actually ~59.71 fps, but this is faster to calculate

    GBAMemory* systemMemory;
    SDL_Window* window;
    
    LCD(GBAMemory*,SDL_Window*);

    // todo: put lines in hblank and draw in vblank
    void draw();

    void loadShaders();
};