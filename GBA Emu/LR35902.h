#pragma once

#include <cstdint>
#include <iostream>

struct LR35902 {
    uint8_t eightBitReg[8];
    static enum r8 {a, b, c, d, e, f, h, l};
    uint16_t sp, pc;

    uint16_t get16r(uint8_t first, uint8_t second) {
        uint16_t reg = (static_cast<uint16_t>(eightBitReg[first]) << 8);
        return reg | eightBitReg[second];
    }

    void set16r(uint16_t input, uint8_t first, uint8_t second) {
        eightBitReg[first] = ;
        eightBitReg
    }

    bool load();
    void cycle();
};