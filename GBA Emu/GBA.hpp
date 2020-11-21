#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
//#include <SDL.h>
#include "cores/ARM7TDMI.hpp"

struct GBA {
    static ARM7TDMI arm7;
    uint8_t* romMemory;

    GBA(std::string);
    uint8_t* loadRom(std::string);
};