#pragma once
#include <cstdint>
#include <SDL.h>
#include <glew.h>
#include "hardware/LCD.hpp"
#include "core/ARM7TDMI.hpp"
#include "hardware/GBAMemory.hpp"

struct GBA {
    /// Cycle scheduler variables ///
    uint32_t cyclesPassed = 0;
    int16_t cyclesSinceHBlank = 0;

    /// Hardware ///
    GBAMemory* memory;
    ARM7TDMI* arm7tdmi;
    // todo: eventually make ppu rendering, keypad, and apu dedicated to their own threads somehow
    LCD* lcd;

    GBA();

    void interpretARM();
    void interpretTHUMB();
};