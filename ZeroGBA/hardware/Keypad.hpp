#pragma once
#include <SDL.h>
#include <glew.h>
#include <cstdint>
#include "MMIO.h"
#include "memory/GBAMemory.hpp"

// for keypad hardware AND emulator controls
struct Keypad {

    GBAMemory* systemMemory;

    // Event/control variables
    bool enterDown = false;
    bool altDown = false;
    bool noAudioSync = false;
    SDL_Event event;
    SDL_Window* window;
    SDL_DisplayMode displayMode;
    uint32_t initialWidth, initialHeight;
    
    void toggleFullscreen(SDL_Window*);
    void pollInputs();
};