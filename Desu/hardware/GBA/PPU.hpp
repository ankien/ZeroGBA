#pragma once
#include <stdint.h>

struct PPU {
    // dunno if i'm gonna change this
    uint8_t screen[240*160];
    uint8_t palette[0x3FF];
    uint8_t vram[0x17FFF];
    uint8_t oam[0x3FF];
};