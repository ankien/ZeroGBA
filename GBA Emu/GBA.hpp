#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
//#include <SDL.h>
#include "cores/ARM7TDMI.hpp"

struct GBA {
    ARM7TDMI arm7;
    std::vector<uint8_t> rom;

    GBA();
    bool loadRom(std::string);
};