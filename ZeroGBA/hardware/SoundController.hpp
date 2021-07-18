#pragma once
#include <SDL.h>
#include <cstdint>
#include <functional>
#include "../Scheduler.hpp"
#include "../common/bits.hpp"

struct GBAMemory;

struct SoundController {
    
    Scheduler* scheduler;
    GBAMemory* systemMemory;
    bool* noAudioSync;

    enum sampleData {
        Channels = 2,
        BufferSize = 8192,
        SampleRate = 48000,
        SamplePeriod = 16777216 / SampleRate
    };
    int16_t buffer[BufferSize]{}; // 32 digit buffer (one bank) * 16-bit format * 2 channels
    uint16_t bufferPos = 0;

    // 8-stage frame sequencer is used to tick other APU channel components (length, envelope, sweep)
    enum frameSequencerData {
        FrameSequencerRate = 512,
        FrameSequencerPeriod = 16777216 / FrameSequencerRate
    };
    uint8_t frameSequencerStage = 0;

    SoundController();

    std::function<uint32_t()> tickFrameSequencer;
    std::function<uint32_t()> getSample;

    // Channels 1-4
    template<uint8_t> void envelopeStep();
    template<uint8_t> void lengthStep();
    void sweepStep();

    // DMA channels
    uint8_t currFifoSize[2]{};
    uint8_t currFifoBytePos[2]{};
    uint8_t fifos[2][32]{};
    int16_t fifoLatch[2]{};
    void timerOverflow(uint8_t);
    void fillFifo(uint8_t,const uint32_t,uint8_t);
};