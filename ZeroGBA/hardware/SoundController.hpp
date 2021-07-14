#pragma once
#include <SDL.h>
#include <cstdint>
#include "../Scheduler.hpp"
#include "../common/bits.hpp"

struct GBAMemory;

struct SoundController {
    
    Scheduler* scheduler;
    SDL_AudioSpec audioSpec{};
    GBAMemory* systemMemory;

    SoundController();

    // Channel 1 - Tone & Sweep


    // Channel 2 - Tone


    // Channel 3 - Wave Output


    // Channel 4 - Noise


    // DMA channels
    uint8_t currFifoSize[2]{};
    uint8_t currFifoBytePos[2]{};
    uint8_t fifos[2][32]{};
    int16_t fifoLatch[2]{};
    void timerOverflow(uint8_t);
};