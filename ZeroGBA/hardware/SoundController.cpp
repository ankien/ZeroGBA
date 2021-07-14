#include "memory/GBAMemory.hpp"
#include "SoundController.hpp"

SoundController::SoundController() {
    audioSpec = {
        .freq = 48000,
        .format = AUDIO_S16, // max 10-bit output range [GBAtek - GBA Sound Control Registers]
        .channels = 2,       // L/R channels
        .samples = 1024,     // 32 digit buffer (one bank) * 16-bit format * 2 channels
        .callback = nullptr, // ???, called whenever sample buffer is emptied
    };

    SDL_PauseAudioDevice(SDL_OpenAudioDevice(nullptr,0,&audioSpec,&audioSpec,0),0);
}

void SoundController::timerOverflow(uint8_t timerId) {
    for(uint8_t fifoChannel = 0; fifoChannel < 2; fifoChannel++) {
        const bool timerSelect = systemMemory->IORegisters[0x83] & (4 << (timerId*4));
        if(timerId == timerSelect) {
            if(currFifoSize[fifoChannel] > 0) {
                fifoLatch[fifoChannel] = signExtend<int16_t,8>(fifos[fifoChannel][currFifoBytePos[fifoChannel]]) * 2; //  50% volume setting, DMA channel ranges -0x100-0xFE,
                currFifoBytePos[fifoChannel] = (currFifoBytePos[fifoChannel] + 1) % 32;        // 100% = -0x200 - 0x1FC
                currFifoSize[fifoChannel]--;
            }
        }

        // if FIFO only contains <= 16 bytes, request more data via DMA
        if(currFifoSize[fifoChannel] <= 16)
            systemMemory->soundDma(fifoChannel);
    }
}