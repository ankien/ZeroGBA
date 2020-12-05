#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
//#include <SDL.h>
#include "cores/ARM7TDMI.hpp"

struct GBA { // the "data bus" for our GBA
    ARM7TDMI arm7tdmi;
    uint8_t* romMemory;

    GBA(std::string);

    void interpretARM(uint8_t*);
    void interpretTHUMB(uint8_t*);
    uint8_t* loadRom(std::string);
};