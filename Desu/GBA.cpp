#include "GBA.hpp"

GBA::GBA() {
    memory = new GBAMemory();
    memory->bios = (uint8_t*)calloc(0x4000,sizeof(uint8_t));
    memory->wramOnBoard = (uint8_t*)calloc(0x40000,sizeof(uint8_t));
    memory->wramOnChip = (uint8_t*)calloc(0x8000,sizeof(uint8_t));
    memory->IORegisters = (uint8_t*)calloc(0x3FF,sizeof(uint8_t));
    memory->pram = (uint8_t*)calloc(0x400,sizeof(uint8_t));
    memory->vram = (uint8_t*)calloc(0x18000,sizeof(uint8_t));
    memory->oam = (uint8_t*)calloc(0x400,sizeof(uint8_t));
    memory->gamePak = (uint8_t*)calloc(0x3000000,sizeof(uint8_t));
    memory->gPakSram = (uint8_t*)calloc(0x10000,sizeof(uint8_t));

    arm7tdmi = new ARM7TDMI();
    arm7tdmi->systemMemory = memory;
    // skip the bios, todo: implement everything in order to load it correctly
    arm7tdmi->pc = 0x8000000;
    arm7tdmi->setArrayIndex(0,0xCA5);
    arm7tdmi->setCPSR(0x1F);
    arm7tdmi->setModeArrayIndex(arm7tdmi->System,13,0x3007F00);
    arm7tdmi->setModeArrayIndex(arm7tdmi->IRQ,13,0x3007FA0);
    arm7tdmi->setModeArrayIndex(arm7tdmi->Supervisor,13,0x3007FE0);
    arm7tdmi->setModeArrayIndex(arm7tdmi->System,14,0x8000000);

    lcd = new LCD(memory);
}

// there's a pipeline, DMA channels, audio channels, PPU, and timers too?
// i think i can fake the pipeline
void GBA::interpretARM() {
    uint32_t instruction = ((*memory)[arm7tdmi->pc + 3] << 24) |
        ((*memory)[arm7tdmi->pc + 2] << 16) |
        ((*memory)[arm7tdmi->pc + 1] << 8) |
        (*memory)[arm7tdmi->pc];

    if(arm7tdmi->checkCond(instruction & 0xF0000000)) {
        uint16_t armIndex = arm7tdmi->fetchARMIndex(instruction);
        (arm7tdmi->*(arm7tdmi->armTable[armIndex]))(instruction);
        #if defined(PRINT_INSTR)
            printf(" %X\n",instruction); // debug
        #endif
        cyclesPassed += arm7tdmi->cycleTicks;
        cyclesSinceHBlank += arm7tdmi->cycleTicks;
    } else
        arm7tdmi->pc+=4;
}

void GBA::interpretTHUMB() {
    uint16_t instruction = ((*memory)[arm7tdmi->pc+1] << 8) |
                            (*memory)[arm7tdmi->pc];

    uint8_t thumbIndex = arm7tdmi->fetchTHUMBIndex(instruction);
    (arm7tdmi->*(arm7tdmi->thumbTable[thumbIndex]))(instruction);
    #if defined(PRINT_INSTR)
        printf(" %X\n",instruction); // debug
    #endif
    cyclesPassed += arm7tdmi->cycleTicks;
    cyclesSinceHBlank += arm7tdmi->cycleTicks;
}