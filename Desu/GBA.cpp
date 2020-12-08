#include "GBA.hpp"

GBA::GBA() {
    memoryMap = new uint8_t[0xFFFFFFFF];
    arm7tdmi = new ARM7TDMI(memoryMap);
}

// each instruction has multiple cycles, there's a pipeline, DMA channels, audio channels, PPU, and timers too? oh boy
// i think i can fake the pipeline and DMA
void GBA::interpretARM(uint8_t* memoryMap) {
    uint32_t instruction = (memoryMap[arm7tdmi->pc+3] << 24) |
                           (memoryMap[arm7tdmi->pc+2] << 16) | 
                           (memoryMap[arm7tdmi->pc+1] << 8)  |
                            memoryMap[arm7tdmi->pc];
    if(arm7tdmi->checkCond(instruction & 0xF0000000)) {
        uint16_t armIndex = arm7tdmi->fetchARMIndex(instruction);
        (arm7tdmi->*(arm7tdmi->armTable[armIndex]))(instruction);
    }
}

// do thumb instructions have cond? don't think so
void GBA::interpretTHUMB(uint8_t* memoryMap) {
    uint16_t instruction = (memoryMap[arm7tdmi->pc+1] << 8) |
                            memoryMap[arm7tdmi->pc];
    uint8_t thumbIndex = arm7tdmi->fetchTHUMBIndex(instruction);
    (arm7tdmi->*(arm7tdmi->thumbTable[thumbIndex]))(instruction);
}

bool GBA::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    if(romSizeInBytes > 0x1FFFFFF)
        return 0;
    fileStream.read(reinterpret_cast<char*>(memoryMap) + 0x8000000,romSizeInBytes);
    return 1;
}