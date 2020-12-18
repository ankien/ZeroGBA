#include "GBA.hpp"

GBA::GBA() {
    memory = new GBAMemory();
    arm7tdmi = new ARM7TDMI(memory);

    // SDL + OpenGL setup
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_GL_SetAttribute();

    SDL_Window* window = SDL_CreateWindow(
        "Placeholder Title", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        lcd->WIDTH, 
        lcd->HEIGHT, 
        SDL_WINDOW_OPENGL
    );

    SDL_GL_CreateContext(window);
    glewInit();

    lcd = new LCD(memory,window);
}

// each instruction has multiple cycles, there's a pipeline, DMA channels, audio channels, PPU, and timers too? oh boy
// i think i can fake the pipeline and DMA
void GBA::interpretARM() {
    uint32_t instruction = ((*memory)[arm7tdmi->pc+3] << 24) |
                           ((*memory)[arm7tdmi->pc+2] << 16) | 
                           ((*memory)[arm7tdmi->pc+1] << 8)  |
                            (*memory)[arm7tdmi->pc];
    if(arm7tdmi->checkCond(instruction & 0xF0000000)) {
        uint16_t armIndex = arm7tdmi->fetchARMIndex(instruction);
        (arm7tdmi->*(arm7tdmi->armTable[armIndex]))(instruction);
    }
}

void GBA::interpretTHUMB() {
    uint16_t instruction = ((*memory)[arm7tdmi->pc+1] << 8) |
                            (*memory)[arm7tdmi->pc];
    uint8_t thumbIndex = arm7tdmi->fetchTHUMBIndex(instruction);
    (arm7tdmi->*(arm7tdmi->thumbTable[thumbIndex]))(instruction);
}