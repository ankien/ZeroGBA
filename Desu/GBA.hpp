#pragma once
#include <SDL.h>
#include <glm.hpp>
#include <glew.h>
#include "hardware/GBA/LCD.hpp"
#include "cores/ARM7TDMI.hpp"
#include "hardware/GBA/Memory.hpp"

struct GBA {
    Memory* memory;
    ARM7TDMI* arm7tdmi;
    LCD* lcd;

    GBA();

    void interpretARM();
    void interpretTHUMB();
};