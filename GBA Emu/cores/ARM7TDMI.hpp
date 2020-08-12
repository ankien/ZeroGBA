#pragma once

#include <stdint.h>

struct ARM7TDMI {
    static const uint32_t CLOCK_RATE = 16780000;
    static const uint32_t DS_CLOCK_RATE = 33000000;

    void interpretCycle();
};