#include "GBA.hpp"

GBA::GBA() {
    memory = new GBAMemory();
    arm7tdmi = new ARM7TDMI(memory);
    lcd = new LCD(memory);
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