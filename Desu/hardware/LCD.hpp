#pragma once
#include <cstdint>
#include <SDL.h>
#include <glew.h>
#include <string>
#include "GBAMemory.hpp"

struct LCD {
    /// GBA variables ///
    static const uint16_t 
        SCALE = 1,
        WIDTH = 240,
        HEIGHT = 160;
    

    uint8_t fps = 0;
    // used to calculate fps and frame delta
    uint32_t currMillseconds = 0;
    uint32_t millisecondsElapsedAtLastSecond = 0;
    uint32_t millisecondsElapsedAtLastFrame = 0;

    GBAMemory* systemMemory;

    /// SDL + OpenGL variables ///
    SDL_Window* window;
    uint32_t program;
    // pixel format: xbbbbbgggggrrrrr, x unused
    uint16_t* pixelBuffer;

    /// For the software renderer ///
    uint16_t bgLayer[4][240];
    struct {
        uint16_t color;
        uint8_t priority;
        uint8_t alpha : 1;
    } spriteLayer[240];

    // [Shape][Size][width or height]
    const uint8_t spriteOBJSize[4][4][2] = {
        // Square
        {
            {8,8},
            {16,16},
            {32,32},
            {64,64}
        },
        // Horizontal
        {
            {16,8},
            {32,8},
            {32,16},
            {64,32}
        },
        // Vertical
        {
            {8,16},
            {8,32},
            {16,32},
            {32,64}
        },
        // Prohibited
        {
            {0,0},
            {0,0},
            {0,0},
            {0,0}
        }
    };
    
    LCD();

    void renderTextBG(uint8_t);
    void renderAffineBG(uint8_t);
    void renderSprites(uint32_t);
    void composeScanline(uint16_t*);
    void renderScanline();

    void draw();

    std::string loadShader(const char*);
    uint32_t createShader(std::string,uint32_t);
    void compileShaders();
};