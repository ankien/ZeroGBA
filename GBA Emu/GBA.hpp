#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
//#include <SDL.h>
#include "cores/ARM7TDMI.hpp"

struct GBA {
    ARM7TDMI arm7;
    char* romMemory;

    GBA(std::string);
    char* loadRom(std::string);
};