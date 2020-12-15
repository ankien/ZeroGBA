#pragma once
#include <stdint.h>
#include <fstream>
#include <filesystem>

struct Memory {
    uint8_t* bios;
    uint8_t* wramOnBoard;
    uint8_t* wramOnChip;
    uint8_t* IORegisters;
    uint8_t* pram;
    uint8_t* vram;
    uint8_t* oam;
    uint8_t* gamePak;
    uint8_t* gPakSram;

    uint8_t& operator[](uint32_t);

    Memory();

    bool loadRom(std::string);
};