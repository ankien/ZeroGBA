#pragma once
#include <cstdint>
#include <SDL.h>
#include <glew.h>
#include "core/ARM7TDMI.hpp"
#include "hardware/GBAMemory.hpp"
#include "hardware/LCD.hpp"
#include "hardware/Keypad.hpp"

struct GBA {
    /// Cycle scheduler variables ///
    uint32_t cyclesPassed = 0;
    int16_t cyclesSinceHBlank = 0;

    /// Hardware ///
    ARM7TDMI arm7tdmi{};
    GBAMemory* memory;
    // todo: eventually make ppu rendering, keypad, and apu dedicated to their own threads somehow
    LCD lcd{};
    Keypad keypad{};

    GBA();

    void interpretARM();
    void interpretTHUMB();
};