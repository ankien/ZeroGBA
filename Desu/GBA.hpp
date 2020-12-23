#pragma once
#include <stdint.h>
#include <SDL.h>
#include <glew.h>
#include "hardware/GBA/LCD.hpp"
#include "cores/ARM7TDMI.hpp"
#include "hardware/GBA/GBAMemory.hpp"

struct GBA {
    /// Cycle scheduler variables ///
    uint32_t cyclesPassed = 0;
    int16_t cyclesSinceHBlank = 0;

    /// Hardware ///
    GBAMemory* memory;
    ARM7TDMI* arm7tdmi;
    LCD* lcd;

    GBA();

    void interpretARM();
    void interpretTHUMB();
};