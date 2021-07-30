#pragma once
#include <cstdint>
#include <SDL.h>
#include <glew.h>
#include <string>
#include "memory/GBAMemory.hpp"

struct LCD {
    /// GBA variables ///
    static const uint16_t 
        SCALE = 1,
        WIDTH = 240,
        HEIGHT = 160;
    

    uint16_t fps = 0;
    // used to calculate fps
    uint32_t currMillseconds = 0;
    uint32_t millisecondsElapsedAtLastSecond = 0;

    GBAMemory* systemMemory;

    /// SDL + OpenGL variables ///
    SDL_Window* window;
    uint32_t program;
    // pixel format: xbbbbbgggggrrrrr, x unused
    uint16_t* pixelBuffer;

    /// For the software renderer, stores palette indexes for scanline composition ///
    uint8_t bgLayer[4][240];
    struct Sprite {
        uint8_t pindex = 0;
        uint8_t priority = 4; // not actual possible value
        bool alpha = 0,
             window = 0;

    } spriteLayer[240];

    // [Shape][Size][width or height], in pixels, not tiles!
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

    // for compositing a scanline
    enum bgTypes {BG0,BG1,BG2,BG3,OBJ,BD};
    
    LCD();

    // read tile data from VRAM (+ OAM for sprites)
    // and use the data as an index into PRAM
    uint16_t screenEntryIndex(uint16_t,uint16_t,uint16_t);
    void renderTextBG(uint8_t,uint8_t);
    void renderAffineBG(uint8_t);
    template<bool> void renderSprites(int16_t);

    // Take the BG and sprite layers and output them into pixelBuffer 
    // with priority and alpha blending
    template<bool> uint16_t brightnessFade(uint8_t,uint16_t);
    uint16_t blend(uint8_t,uint16_t,uint8_t,uint16_t);
    template<bool> void composeScanline(uint16_t*,uint8_t);

    void renderScanline();

    void draw();

    std::string loadShader(const char*);
    uint32_t createShader(std::string,uint32_t);
    void compileShaders();
};

inline uint16_t LCD::screenEntryIndex(uint16_t tx,uint16_t ty,uint16_t bgcntSize) {
    uint16_t n = tx + ty*32;
    if(tx >= 32)
        n += 0x03E0;
    if(ty >= 32 && bgcntSize == 0xC000)
        n += 0x0400;
    return n;
}