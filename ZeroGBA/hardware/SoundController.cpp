#include "memory/GBAMemory.hpp"
#include "SoundController.hpp"

SoundController::SoundController() {
    SDL_AudioSpec audioSpec = {
        .freq = SampleRate,
        .format = AUDIO_S16, // max 10-bit output range [GBAtek - GBA Sound Control Registers]
        .channels = Channels,
        .samples = BufferSize/Channels,
        .callback = nullptr, // ???, called whenever sample buffer is emptied
    };

    tickFrameSequencer = [&]() {
        switch(frameSequencerStage) {
            case 0:
                lengthStep<1>();
                lengthStep<2>();
                lengthStep<3>();
                lengthStep<4>();
                break;
            case 1:
                break;
            case 2:
                lengthStep<1>();
                lengthStep<2>();
                lengthStep<3>();
                lengthStep<4>();
                sweepStep();
                break;
            case 3:
                break;
            case 4:
                lengthStep<1>();
                lengthStep<2>();
                lengthStep<3>();
                lengthStep<4>();
                break;
            case 5:
                break;
            case 6:
                lengthStep<1>();
                lengthStep<2>();
                lengthStep<3>();
                lengthStep<4>();
                sweepStep();
                break;
            case 7:
                envelopeStep<1>();
                envelopeStep<2>();
                envelopeStep<4>();
                break;
        }

        frameSequencerStage++;
        frameSequencerStage %= 8;

        return FrameSequencerPeriod;
    };

    getSample = [&]() {
        const uint16_t soundCntL = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x80]);
        const uint16_t soundCntH = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x82]);
        const uint16_t biasLevel = (*reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x88]) & 0x3FE) >> 1;
        // Channels 1-4


        // DMA channels
        int16_t volumeScaledLatch[2];
        for(uint8_t fifoChannel = 0; fifoChannel < 2; fifoChannel++)
            volumeScaledLatch[fifoChannel] = fifoLatch[fifoChannel] << static_cast<bool>(soundCntH & (1 << 2+fifoChannel));
        int16_t dmaLeft = volumeScaledLatch[0]*static_cast<bool>(soundCntH&0x200) + volumeScaledLatch[1]*static_cast<bool>(soundCntH&0x2000);
        int16_t dmaRight = volumeScaledLatch[0]*static_cast<bool>(soundCntH&0x100) + volumeScaledLatch[1]*static_cast<bool>(soundCntH&0x1000);

        int16_t total[2];
        // to unsigned
        total[0] = dmaLeft + biasLevel;
        total[1] = dmaRight + biasLevel;
        for(auto& num : total) {
            if(num > 0x3FF)
                num = 0x3FF;
            else if(num < 0)
                num = 0;
        }
        // back to signed
        total[0] -= biasLevel;
        total[1] -= biasLevel;

        // bring 10-bit signed to 16-bit signed range
        buffer[bufferPos] = total[0] << 6;
        buffer[bufferPos + 1] = total[1] << 6;
        bufferPos += 2;

        if(bufferPos >= BufferSize) {
            if(*noAudioSync)
                SDL_ClearQueuedAudio(1);
            while(SDL_GetQueuedAudioSize(1) > BufferSize*sizeof(int16_t))
                SDL_Delay(1);
            SDL_QueueAudio(1,buffer,BufferSize*sizeof(int16_t));
            bufferPos = 0;
        }

        return SamplePeriod;
    };

    SDL_OpenAudio(&audioSpec, NULL);
    SDL_PauseAudio(0);
}

template<uint8_t soundChannelId>
void SoundController::envelopeStep() {

}

template<uint8_t soundChannelId>
void SoundController::lengthStep() {

}

void SoundController::sweepStep() {

}

void SoundController::timerOverflow(uint8_t timerId) {
    for(uint8_t fifoChannel = 0; fifoChannel < 2; fifoChannel++) {
        const bool timerSelect = systemMemory->IORegisters[0x83] & (4 << (timerId*4));
        if(timerId == timerSelect) {
            if(currFifoSize[fifoChannel] > 0) {
                fifoLatch[fifoChannel] = signExtend<int16_t,8>(fifos[fifoChannel][currFifoBytePos[fifoChannel]]) * 2; // 50% volume setting, DMA channel ranges -0x100-0xFE,
                currFifoBytePos[fifoChannel] = (currFifoBytePos[fifoChannel] + 1) % 32; // 100% = -0x200 - 0x1FC
                currFifoSize[fifoChannel]--;
            }
        }

        // if FIFO only contains <= 16 bytes, request more data via DMA
        if(currFifoSize[fifoChannel] <= 16)
            systemMemory->soundDma(fifoChannel);
    }
}

void SoundController::fillFifo(uint8_t ioAddress,const uint32_t value,uint8_t sizeOfThisInBytes) {
    const bool fifoChannel = ioAddress > 0xA3;

    if(currFifoSize[fifoChannel] < 32) {
        fifos[fifoChannel][(currFifoBytePos[fifoChannel] + currFifoSize[fifoChannel]) % 32] = value;
        currFifoSize[fifoChannel]++;
    }

    if(ioAddress+1 < 0xA8 && sizeOfThisInBytes > 0)
        fillFifo(ioAddress+1,value >> 8,sizeOfThisInBytes-1);
}