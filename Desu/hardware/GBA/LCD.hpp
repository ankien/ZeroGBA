#pragma once
#include <stdint.h>
#include <SDL.h>
#include "Memory.hpp"

struct LCD {
    static const uint8_t 
        WIDTH = 240,
        HEIGHT = 160,
        FPS = 60; // actually ~59.71 fps, but this is faster to calculate

    uint64_t secondsPassed;

    // 280896 cycles per frame

    Memory* systemMemory;
    
    LCD(Memory*);
};