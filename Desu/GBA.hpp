#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include "hardware/GBA/LCD.hpp"
#include "cores/ARM7TDMI.hpp"

struct GBA { // data bus for our system
    ARM7TDMI* arm7tdmi;
    LCD* lcd;
    
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

    GBA();

    void interpretARM();
    void interpretTHUMB();
    bool loadRom(std::string);
};