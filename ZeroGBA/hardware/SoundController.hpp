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
        FrameSequencerPeriod = 16777216 / 512
    };
    uint8_t frameSequencerStage = 0;

    SoundController();

    std::function<uint32_t()> tickFrameSequencer;
    std::function<uint32_t()> getSample;

    // Channels 1-4

    uint8_t waveDutyPosition[2]{};
    static constexpr int8_t wavePatternDuty[4][8] = {
        {-8,-8,-8,-8,-8,-8,-8,8},
        {8,-8,-8,-8,-8,-8,-8,8},
        {8,-8,-8,-8,-8,8,8,8},
        {-8,8,8,8,8,8,8,-8},
    };
    bool sweepEnabled{};
    uint16_t shadowFrequency{}; // NR13, 11-bit value
    uint8_t sweepTimer{}; // 0-7
    uint8_t periodTimer[3]{}; // 0-7
    uint8_t currentVolume[3]{}; // 0 - 15
    bool dacEnabled[4]{};
    bool enabled[4]{}; // SOUNDCNT_X
    uint16_t lengthCounter[4]{};
    uint8_t waveRamPosition{};
    uint8_t waveRamSample{};
    uint8_t waveRam[2][32]{};
    uint16_t lfsr{};
    void removeWaveGenStep(uint8_t);
    template<uint8_t> uint32_t calculateFrequencyTimer();
    void scheduleWaveGenStep(uint8_t);
    template<uint8_t> int16_t getAmplitude();
    void initEnvelope(uint8_t);
    template<uint8_t> void envelopeStep();
    template<uint8_t> void lengthStep();
    uint16_t calculateNewFrequency();
    void sweepStep();

    // DMA channels
    uint8_t currFifoSize[2]{};
    uint8_t currFifoBytePos[2]{};
    uint8_t fifos[2][32]{};
    int16_t fifoLatch[2]{};
    void timerOverflow(uint8_t);
    void fillFifo(uint8_t,uint32_t,uint8_t);
};