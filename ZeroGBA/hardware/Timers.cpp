#include "Timers.hpp"
#include "SoundController.hpp"

void Timers::removeOverflow(uint8_t eventType) {
    scheduler->eventList.remove_if([=](const Scheduler::Event& event) { return event.eventType == eventType; });
}

void Timers::scheduleOverflow(uint8_t timerId, uint8_t prescaler) {
    enableTimestamp[timerId] = scheduler->cyclesPassed;
    const uint8_t initialTimerId = timerId;
    const uint16_t initialControlAddress = 0x102 + 4 * timerId;

    scheduler->scheduleEvent([=]() mutable {
        
        bool overflowing = true;
        
        while(overflowing) {
            uint16_t controlAddress = 0x102 + 4 * timerId;
            const uint8_t timerControlLo = IORegisters[controlAddress];

            if(timerId <= 1) // timers 0 and 1 are used to supply sample rate for DMA sound channel A and/or B
                soundController->timerOverflow(timerId); // DMA-Sound playback procedure

            if(timerControlLo & 0x40) { // timer IRQ
                IORegisters[0x202] |= 1 << 3 + timerId;
                interrupts->scheduleInterruptCheck();
            }

            if(timerId < 3) {
                if(IORegisters[0x102 + 4 * (timerId + 1)] & 0x84) { // if i+1 is cascading and enabled
                    timerId++;
                    internalTimer[timerId]++;
                    overflowing = internalTimer[timerId] == 0;
                } else
                    overflowing = false;
            } else
                overflowing = false;
        }

        timerId = initialTimerId;
        return (static_cast<uint16_t>(0 - *reinterpret_cast<uint16_t*>(&IORegisters[initialControlAddress - 2]))+1) * period[prescaler];
    },Scheduler::Timer0+timerId,scheduler->cyclesPassed + (static_cast<uint16_t>(0 - internalTimer[timerId])+1) * period[prescaler],true);
}

void Timers::updateTimer(uint8_t timerId, uint8_t tmcntHLo) {
    if(!static_cast<bool>(tmcntHLo & 0x4) && static_cast<bool>(tmcntHLo & 0x80))
        internalTimer[timerId] = (scheduler->cyclesPassed - enableTimestamp[timerId]) / period[tmcntHLo & 0x3];
}

/*
void Timers::scheduleTimerStep(uint8_t timerId,uint64_t cycleTimestamp) {
    // tick the timer, if it has overflowed, repeat with (i+1) if that is cascading and enabled
    // when a timer overflows, start at the current reload value
    // raise interrupt check on overflow if enabled
    scheduler->scheduleEvent([=]() mutable {
        uint8_t initialTimerId = timerId;
        bool tick = true;
        while(tick) {
            internalTimer[timerId]++;
            if(internalTimer[timerId] == 0) {
                tick = false;
                uint16_t controlAddress = 0x102 + 4 * timerId;
                const uint8_t timerControlLo = IORegisters[controlAddress];

                if(timerId <= 1) // timers 0 and 1 are used to supply sample rate for DMA sound channel A and/or B
                    soundController->timerOverflow(timerId); // DMA-Sound playback procedure

                if(timerControlLo & 0x40) { // timer IRQ
                    IORegisters[0x202] |= 1 << 3 + timerId;
                    interrupts->scheduleInterruptCheck();
                }

                internalTimer[timerId] = *reinterpret_cast<uint16_t*>(&IORegisters[controlAddress - 2]);

                if(timerId < 3)
                    if(IORegisters[0x102 + 4 * (timerId + 1)] & 0x84) { // if i+1 is cascading and enabled
                        timerId++;
                        tick = true;
                    }
            } else
                tick = false;
        }
        timerId = initialTimerId;
        return cycleTimestamp; // where cycleTimeStamp is the number of cycles it takes for this timer to tick
    },Scheduler::Timer0+timerId,scheduler->cyclesPassed+cycleTimestamp,true);
}
*/