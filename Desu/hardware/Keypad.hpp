#pragma once
#include <SDL.h>
#include <glew.h>
#include <cstdint>
#include "MMIO.h"
#include "../hardware/GBAMemory.hpp"

// currently for keypad AND controls
struct Keypad {

    GBAMemory* systemMemory;

    // Event/control variables
    bool enterDown = false;
    bool altDown = false;
    bool running = true;
    SDL_Event event;
    SDL_Window* window; SDL_DisplayMode displayMode; int width, height;
    
    void toggleFullscreen(SDL_Window*);
    void pollInputs();
};