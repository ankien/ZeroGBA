#pragma once
#include <stdint.h>
#include <SDL.h>
#include "../../GBA.hpp"

struct LCD {
    static const uint16_t 
        WIDTH = 240,
        HEIGHT = 160,
        FPS = 60; // actually ~59.71 fps, but this is faster to calculate

    GBA* systemMemory;
    
    LCD(GBA*);
};