#include "GBA.hpp"

GBA::GBA(std::string rom) {
    romMemory = loadRom(rom);
    arm7tdmi = {};
}

// each instruction has multiple cycles, there's a pipeline too?
void GBA::interpretARM(uint8_t* romMemory) {
    if() { // if cond field matches flags or somthin'
        uint32_t instruction = (romMemory[arm7tdmi.pc+3] << 24) |
                               (romMemory[arm7tdmi.pc+2] << 16) | 
                               (romMemory[arm7tdmi.pc+1] << 8)  |
                                romMemory[arm7tdmi.pc];
        uint16_t armIndex = arm7tdmi.fetchARMIndex(instruction);
        (arm7tdmi.*(arm7tdmi.armTable[armIndex]))(instruction);
    }
}

// do thumb instructions have cond? don't think so
void GBA::interpretTHUMB(uint8_t* romMemory) {
    uint16_t instruction = (romMemory[arm7tdmi.pc+1] << 8) |
                            romMemory[arm7tdmi.pc];
    uint8_t thumbIndex = arm7tdmi.fetchTHUMBIndex(instruction);
    (arm7tdmi.*(arm7tdmi.thumbTable[thumbIndex]))(instruction);
}

uint8_t* GBA::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    romMemory = new uint8_t[romSizeInBytes];
    fileStream.read(reinterpret_cast<char*>(romMemory),romSizeInBytes);
    return romMemory;
}