#include "GBA.hpp"

uint8_t& GBA::operator[](uint32_t i) {
    // unmapped memory accesses will crash! this isn't supposed to happen in the first place but the GBA handles it fine
    switch(i >> 24) {
        case 0x00:
            return bios[i];
        case 0x02:
            return wramOnBoard[i-0x2000000];
        case 0x03:
            return wramOnChip[i-0x3000000];
        case 0x04:
            return IORegisters[i-0x4000000];
        case 0x05:
            return pram[i-0x5000000];
        case 0x06:
            return vram[i-0x6000000];
        case 0x07:
            return oam[i-0x7000000];
        case 0x08:
            return gamePak[i-0x8000000];
        case 0x0E:
            return gPakSram[i-0xE000000];
        default: // unmapped memory access, will crash! todo: handle unpredictable behavior
            break;
    }
}

GBA::GBA() {
    bios = new uint8_t[0x4000];
    wramOnBoard = new uint8_t[0x40000];
    wramOnChip = new uint8_t[0x8000];
    IORegisters = new uint8_t[0x3FF];
    pram = new uint8_t[0x400];
    vram = new uint8_t[0x18000];
    oam = new uint8_t[0x400];
    gamePak = new uint8_t[0x3000000];
    gPakSram = new uint8_t[0x10000];
    arm7tdmi = new ARM7TDMI(this);
    lcd = new LCD(this);
}

// each instruction has multiple cycles, there's a pipeline, DMA channels, audio channels, PPU, and timers too? oh boy
// i think i can fake the pipeline and DMA
void GBA::interpretARM() {
    uint32_t instruction = ((*this)[arm7tdmi->pc+3] << 24) |
                           ((*this)[arm7tdmi->pc+2] << 16) | 
                           ((*this)[arm7tdmi->pc+1] << 8)  |
                            (*this)[arm7tdmi->pc];
    if(arm7tdmi->checkCond(instruction & 0xF0000000)) {
        uint16_t armIndex = arm7tdmi->fetchARMIndex(instruction);
        (arm7tdmi->*(arm7tdmi->armTable[armIndex]))(instruction);
    }
}

void GBA::interpretTHUMB() {
    uint16_t instruction = ((*this)[arm7tdmi->pc+1] << 8) |
                            (*this)[arm7tdmi->pc];
    uint8_t thumbIndex = arm7tdmi->fetchTHUMBIndex(instruction);
    (arm7tdmi->*(arm7tdmi->thumbTable[thumbIndex]))(instruction);
}

bool GBA::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    if(romSizeInBytes > 0x1FFFFFF)
        return 0;
    fileStream.read(reinterpret_cast<char*>((*this)[0x8000000]),romSizeInBytes);
    return 1;
}