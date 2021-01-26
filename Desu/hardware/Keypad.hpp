#pragma once
#include <SDL.h>
#include <glew.h>
#include <cstdint>
#include "MMIO.h"
#include "GBAMemory.hpp"

// for keypad hardware AND emulator controls
struct Keypad {

    GBAMemory* systemMemory;

    // Event/control variables
    bool enterDown = false;
    bool altDown = false;
    bool running = true;
    bool notSkippingFrames = true;
    SDL_Event event;
    SDL_Window* window;
    SDL_DisplayMode displayMode;
    int width, height;
    
    void toggleFullscreen(SDL_Window*);
    void pollInputs();
};