#pragma once
#include <stdint.h>
#include <SDL.h>
#include "GBAMemory.hpp"

struct LCD {
    static const uint8_t 
        SCALE = 6,
        WIDTH = 240 * SCALE,
        HEIGHT = 160 * SCALE,
        FPS = 60; // actually ~59.71 fps, but this is faster to calculate

    uint64_t secondsPassed;

    // 280896 cycles per frame

    GBAMemory* systemMemory;
    
    LCD(GBAMemory*);
};