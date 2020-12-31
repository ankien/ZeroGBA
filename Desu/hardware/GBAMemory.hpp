#pragma once
#include <cstdint>
#include <fstream>
#include <filesystem>
#include "MMIO.h" // for getting mmio fields

struct GBAMemory {

    uint8_t* bios;
    uint8_t* wramOnBoard;
    uint8_t* wramOnChip;
    uint8_t* IORegisters;
    uint8_t* pram;
    uint8_t* vram;
    uint8_t* oam;
    uint8_t* gamePak;
    uint8_t* gPakSram;
    uint8_t unusedMemoryAccess;

    // for getting and setting memory
    uint8_t& operator[](uint32_t);

    // todo: create a saveRom function for different storage types (None, EEPROM-512/8, SRAM-32, Flash-64/128)
    bool loadRom(std::string);
};