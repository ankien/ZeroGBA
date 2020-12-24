#pragma once
#include <stdint.h>
#include <SDL.h>
#include <glew.h>
#include <string>
#include "GBAMemory.hpp"

struct LCD {
    /// GBA variables ///
    static const uint16_t 
        SCALE = 6,
        WIDTH = 240,
        HEIGHT = 160;

    uint8_t fps = 0;
    // seconds elapsed for last frame drawn
    uint8_t secondsElapsed = SDL_GetTicks() / 1000;
    GBAMemory* systemMemory;

    /// SDL + OpenGL variables ///
    SDL_Window* window;
    uint32_t program;
    uint32_t* frameBuffer; // 32-bit cuz glew is strict...
    uint32_t* vertexArrayObject;
    
    LCD(GBAMemory*);

    // not really a "fetch/getter", loads shit like BGs and OBJs
    // into a single line of the framebuffer
    void fetchScanline();

    void draw();

    std::string loadShader(const char*);
    uint32_t createShader(std::string,uint32_t);
    void compileShaders();


};