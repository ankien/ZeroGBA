#pragma once

#include <iostream>
#include <fstream>
#include <SDL.h>

struct GameBoy {
    // Flags(upper 4 bits of 'f' register): zero, add/sub, half-carry, carry
    static enum r8 {a, b, c, d, e, f, h, l};
    uint8_t eightBitReg[8];
    uint8_t memory[0xFFFF];

    uint16_t sp, pc;

    uint16_t get16Reg(uint8_t first, uint8_t second) {
        uint16_t reg = (static_cast<uint16_t>(eightBitReg[first]) << 8);
        return reg | eightBitReg[second];
    }

    void set16Reg(uint16_t input, uint8_t first, uint8_t second) {
        eightBitReg[first]  = ((input & 0xFF00) >> 8);
        eightBitReg[second] = input & 0xFF;
    }

    bool load(std::string);
    void interpretCycle();
};