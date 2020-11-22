#include "GBA.hpp"

GBA::GBA(std::string rom) {
    romMemory = loadRom(rom);
    arm7 = {};
}

void GBA::interpretARMCycle(uint8_t* romMemory) {
    uint32_t instruction = (romMemory[arm7.pc+3] << 24) | romMemory[arm7.pc];
    uint16_t armIndex = arm7.fetchARMIndex(instruction);
    (arm7.*(arm7.armTable[armIndex]))(instruction);
}

void GBA::interpretTHUMBCycle(uint8_t* romMemory) {
    uint16_t instruction = romMemory[arm7.pc];
    uint8_t thumbIndex = arm7.fetchTHUMBIndex(instruction);
    (arm7.*(arm7.thumbTable[thumbIndex]))(instruction);
}

uint8_t* GBA::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    romMemory = new uint8_t[romSizeInBytes];
    fileStream.read(reinterpret_cast<char*>(romMemory),romSizeInBytes);
    return romMemory;
}