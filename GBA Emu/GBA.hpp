#pragma once

#include <iostream>
#include <fstream>
#include <SDL.h>
#include "cores/ARM7TDMI.hpp"

struct GBA {
    /// CPU(s) ///
    
    /// Memory ///
    uint8_t bios[0x3FFF];
    uint8_t workOnboard[0x3FFFF];
    uint8_t workOnchip[0x7FFF];
    uint8_t gpack[0x1FFFFFF];
    uint8_t gpack1[0x1FFFFFF];
    uint8_t gpack2[0x1FFFFFF];
    uint8_t gpackSram[0xFFFF];
    /// Video ///
    uint8_t screen[240*160];
    uint8_t palette[0x3FF];
    uint8_t vram[0x17FFF];
    uint8_t oam[0x3FF];
    /// Sound ///

    /// Controls ///
    uint8_t keys[10];
};