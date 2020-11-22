#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
//#include <SDL.h>
#include "cores/ARM7TDMI.hpp"

struct GBA {
    ARM7TDMI arm7;
    uint8_t* romMemory;

    GBA(std::string);

    void interpretARMCycle(uint8_t*);
    void interpretTHUMBCycle(uint8_t*);
    uint8_t* loadRom(std::string);
};