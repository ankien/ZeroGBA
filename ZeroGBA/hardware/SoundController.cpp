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
        
        // Channels 1-4
        int16_t volumeScaledPsg[2];
        for(uint8_t psgSide : {0,1})
            volumeScaledPsg[psgSide] = (getAmplitude<1>() * static_cast<bool>(soundCntL & (1 << 12-4*psgSide))) +
                                       (getAmplitude<2>() * static_cast<bool>(soundCntL & (1 << 13-4*psgSide))) +
                                       (getAmplitude<3>() * static_cast<bool>(soundCntL & (1 << 14-4*psgSide))) +
                                       (getAmplitude<4>() * static_cast<bool>(soundCntL & (1 << 15-4*psgSide)));
        
        uint8_t psgChannelVol = soundCntH & 0x3;
        // -0x200 to 0x200, each channel amplitude is -0x80 to 0x80
        int16_t psgLeft = (volumeScaledPsg[0] * ((soundCntL & 0x70) >> 4)) >> (5-psgChannelVol);
        int16_t psgRight = (volumeScaledPsg[1] * (soundCntL & 0x7)) >> (5-psgChannelVol);

        // DMA channels
        int16_t volumeScaledDma[2];
        for(uint8_t fifoChannel : {0,1})
            volumeScaledDma[fifoChannel] = fifoLatch[fifoChannel] << static_cast<bool>(soundCntH & (1 << 2+fifoChannel));
        // -0x400 to 0x400
        int16_t dmaLeft = volumeScaledDma[0]*static_cast<bool>(soundCntH&0x200) + volumeScaledDma[1]*static_cast<bool>(soundCntH&0x2000);
        int16_t dmaRight = volumeScaledDma[0]*static_cast<bool>(soundCntH&0x100) + volumeScaledDma[1]*static_cast<bool>(soundCntH&0x1000);

        int16_t total[2];
        total[0] = psgLeft + dmaLeft;
        total[1] = psgRight + dmaRight;
        for(auto& num : total) {
            if(num > 0x1FF)
                num = 0x1FF;
            else if(num < -0x200)
                num = -0x200;
        }

        // bring 10-bit signed to 16-bit signed range @ 9-bit depth (divided by 2) (?)
        buffer[bufferPos] = total[0] << 5;
        buffer[bufferPos + 1] = total[1] << 5;
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

void SoundController::removeWaveGenStep(uint8_t eventType) {
    scheduler->eventList.remove_if([=](const Scheduler::Event& event) { return event.eventType == eventType; });
}

template<uint8_t soundChannelId>
uint32_t SoundController::calculateFrequencyTimer() {
    switch(soundChannelId) {
        case 1:
        case 2:
            return (2048 - (*reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x5C + 8*soundChannelId]) & 0x7FF)) * 16;
        case 3:
            return (2048 - (*reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x74]) & 0x7FF)) * 8;
        case 4:
        {
            uint8_t divisorCode = systemMemory->IORegisters[0x7C] & 0x7;
            uint8_t shiftFrequency = (systemMemory->IORegisters[0x7C] & 0xF0) >> 4;
            return ((divisorCode > 0 ? (divisorCode << 4) : 8) << shiftFrequency) * 4;
        }
    }
}

void SoundController::scheduleWaveGenStep(uint8_t soundChannelId) {
    switch(soundChannelId) {
        case 1:
            scheduler->scheduleEvent([&]() {
                waveDutyPosition[0] = (waveDutyPosition[0] + 1) % 8;
                return calculateFrequencyTimer<1>();
            },Scheduler::SoundChannel1,scheduler->cyclesPassedSinceLastFrame+calculateFrequencyTimer<1>(),true);
            return;
        case 2:
            scheduler->scheduleEvent([&]() {
                waveDutyPosition[1] = (waveDutyPosition[1] + 1) % 8;
                return calculateFrequencyTimer<2>();
            },Scheduler::SoundChannel2,scheduler->cyclesPassedSinceLastFrame+calculateFrequencyTimer<2>(),true);
            return;
        case 3:
            scheduler->scheduleEvent([&]() {
                waveRamPosition = (waveRamPosition + 1) % 32;
                if((waveRamPosition == 0) && (systemMemory->IORegisters[0x70] & 0x20))
                    systemMemory->IORegisters[0x70] ^= 0x40;
                uint8_t waveRamByte = waveRam[static_cast<bool>(systemMemory->IORegisters[0x70] & 0x40)][waveRamPosition / 2];
                waveRamSample = waveRamPosition & 1 ? waveRamByte & 0xF : waveRamByte >> 4;
                return calculateFrequencyTimer<3>();
            },Scheduler::SoundChannel3,scheduler->cyclesPassedSinceLastFrame+calculateFrequencyTimer<3>(),true);
            return;
        case 4:
            scheduler->scheduleEvent([&]() {
                uint16_t xorResult = (lfsr & 0b1) ^ ((lfsr & 0b10) >> 1);
                lfsr = (lfsr >> 1) | (xorResult << 14);
                
                if(systemMemory->IORegisters[0x7C] & 0x8) {
                    lfsr &= ~(1 << 6);
                    lfsr |= xorResult << 6;
                }
                return calculateFrequencyTimer<4>();
            },Scheduler::SoundChannel4,scheduler->cyclesPassedSinceLastFrame+calculateFrequencyTimer<4>(),true);
            return;
    }
}

template<uint8_t soundChannelId>
int16_t SoundController::getAmplitude() {
    constexpr uint8_t index = soundChannelId - 1;
    if(enabled[index] && dacEnabled[index]) {    
        constexpr uint8_t volIdx = soundChannelId == 4 ? 2 : soundChannelId - 1;
        switch(soundChannelId) {
            case 1:
            case 2:
                return static_cast<int16_t>(wavePatternDuty[(systemMemory->IORegisters[0x5C + soundChannelId*6] & 0xC0) >> 6][waveDutyPosition[index]]) * currentVolume[volIdx];
            case 3:
            {
                // 4-bit sample to 8-bit
                constexpr uint8_t volumeRatio[4] = {0,4,2,1};
                const uint8_t soundVolume = (systemMemory->IORegisters[0x73] & 0x60) >> 5;
                return (static_cast<int16_t>(waveRamSample) << 1) * (systemMemory->IORegisters[0x73] & 0x80 ? 3 : volumeRatio[soundVolume]);
            }
            case 4:
                // inverted 0th bit amplified
                return static_cast<int16_t>(((~lfsr) & 1) ? 8 : -5) * currentVolume[volIdx];
        }
    } else 
        return 0;
}

void SoundController::initEnvelope(uint8_t soundChannelId) {
    uint8_t idx = soundChannelId == 4 ? 2 : soundChannelId - 1;
    constexpr uint8_t envelopAddresses[4] = {0x63,0x69,NULL,0x79};
    uint8_t envelopeRegIdx = envelopAddresses[soundChannelId-1];
    uint8_t envelopeReg = systemMemory->IORegisters[envelopeRegIdx];
    periodTimer[idx] = envelopeReg & 0x7;
    currentVolume[idx] = (envelopeReg >> 4) & 0xF;
}

template<uint8_t soundChannelId>
void SoundController::envelopeStep() {
    constexpr uint8_t envelopAddresses[4] = {0x63,0x69,NULL,0x79};
    constexpr uint8_t envelopeRegIdx = envelopAddresses[soundChannelId-1];
    constexpr uint8_t periodIdx = soundChannelId == 4 ? 2 : soundChannelId - 1;
    uint8_t envelopeReg = systemMemory->IORegisters[envelopeRegIdx];
    uint8_t period = envelopeReg & 0x7;
    if(period) {
        if(periodTimer[periodIdx] > 0)
            periodTimer[periodIdx]--;

        if(periodTimer[periodIdx] == 0) {
            periodTimer[periodIdx] = period;

            bool isUpwards = envelopeReg & 0x8;
            if((currentVolume[periodIdx] < 0xF && isUpwards) || (currentVolume[periodIdx] > 0x0 && !isUpwards))
                currentVolume[periodIdx] += isUpwards ? 1 : -1 ;
        }
    }
}

template<uint8_t soundChannelId>
void SoundController::lengthStep() {
    constexpr uint8_t index = soundChannelId - 1;
    bool lengthFlag;
    switch(soundChannelId) {
        case 1:
            lengthFlag = systemMemory->IORegisters[0x65] & 0x40;
            break;
        case 2:
            lengthFlag = systemMemory->IORegisters[0x6D] & 0x40;
            break;
        case 3:
            lengthFlag = systemMemory->IORegisters[0x75] & 0x40;
            break;
        case 4:
            lengthFlag = systemMemory->IORegisters[0x7D] & 0x40;
            break;
    }
    if(lengthFlag && lengthCounter[index] > 0) {
        lengthCounter[index]--;
        enabled[index] = lengthCounter[index];
    }
}

uint16_t SoundController::calculateNewFrequency() {
    const uint8_t sweepReg = systemMemory->IORegisters[0x60];
    uint16_t newFrequency = shadowFrequency >> (sweepReg & 0x7);
    newFrequency = shadowFrequency + (sweepReg & 0x8 ? -1 : 1) * newFrequency;
    if(newFrequency > 0x7FF)
        enabled[0] = false;
    return newFrequency;
}

void SoundController::sweepStep() {
    if(sweepTimer > 0)
        sweepTimer--;

    if(sweepTimer == 0) {
        uint8_t sweepPeriod = (systemMemory->IORegisters[0x60] & 0x70) >> 4;
        sweepTimer = sweepPeriod > 0 ? sweepPeriod : 8;

        if(sweepEnabled && sweepPeriod > 0) {
            uint16_t newFrequency = calculateNewFrequency();
            
            if(newFrequency <= 0x7FF) {
                shadowFrequency = newFrequency;
                systemMemory->memoryArray<uint16_t>(0x4000064) &= 0xF800;
                systemMemory->memoryArray<uint16_t>(0x4000064) |= newFrequency;
                calculateNewFrequency();
            }
        }
    }
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

void SoundController::fillFifo(uint8_t ioAddress,uint32_t value,uint8_t sizeOfThisInBytes) {
    const bool fifoChannel = ioAddress > 0xA3;

    if(currFifoSize[fifoChannel] < 32) {
        fifos[fifoChannel][(currFifoBytePos[fifoChannel] + currFifoSize[fifoChannel]) % 32] = value;
        currFifoSize[fifoChannel]++;
    }

    if(ioAddress+1 < 0xA8 && sizeOfThisInBytes > 0)
        fillFifo(ioAddress+1,value >> 8,sizeOfThisInBytes-1);
}