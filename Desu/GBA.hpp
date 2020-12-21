#pragma once
#include <stdint.h>
#include <SDL.h>
#include <glew.h>
#include "hardware/GBA/LCD.hpp"
#include "cores/ARM7TDMI.hpp"
#include "hardware/GBA/GBAMemory.hpp"

struct GBA {
    uint32_t cyclesPassed = 0;
    GBAMemory* memory;
    ARM7TDMI* arm7tdmi;
    LCD* lcd;

    GBA();

    void interpretARM();
    void interpretTHUMB();
    // do cyclic tasks for both ARM and THUMB CPU ticks that aren't refreshing the display
    // basically every other task not performed every intruction cycle
    // todo: make an event scheduler?
    void doProcesses();
};

inline void GBA::doProcesses() {
    //if((cycles % something)== 0)
}