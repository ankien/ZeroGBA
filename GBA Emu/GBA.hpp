#pragma once
#include <iostream>
#include <fstream>
#include <SDL.h>
#include "cores/ARM7TDMI.hpp"

struct GBA {
    /// CPU(s) ///
    ARM7TDMI arm7;

    GBA();
    bool loadRom(std::string);
};