#pragma once
#include <SDL.h>
#include <glew.h>
#include "hardware/GBA/LCD.hpp"
#include "cores/ARM7TDMI.hpp"
#include "hardware/GBA/GBAMemory.hpp"

struct GBA {
    GBAMemory* memory;
    ARM7TDMI* arm7tdmi;
    LCD* lcd;

    GBA();

    void interpretARM();
    void interpretTHUMB();
};