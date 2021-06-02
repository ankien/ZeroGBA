#pragma once
#include <functional>
#include <cstdint>
#include "cpu/CPUState.hpp"
#include "../Scheduler.hpp"

struct Interrupts {

    CPUState* cpuState;
    Scheduler* scheduler;
    uint8_t* IORegisters;

    /// Scheduler Stuff ///
    void scheduleInterruptCheck();
    bool irqsEnabled();
    void scheduleHaltCheck();
    void haltCheck();
};


inline void Interrupts::scheduleInterruptCheck() {
    scheduler->scheduleInterruptCheck([&]() {
        if((*reinterpret_cast<uint16_t*>(&IORegisters[0x200]) & 0x3FFF) &
           (*reinterpret_cast<uint16_t*>(&IORegisters[0x202]) & 0x3FFF))
            if(irqsEnabled())
                cpuState->handleException(NormalInterrupt,4,IRQ);
        return 0;
    });
}
inline bool Interrupts::irqsEnabled() {
    if(IORegisters[0x208] & 0x1)
        return !cpuState->irqDisable;
    return false;
}

inline void Interrupts::scheduleHaltCheck() {
    scheduler->addEventToFront([&]() {
        std::function<void()> check = std::bind(&Interrupts::haltCheck,this);
        scheduler->eventList.erase(scheduler->eventList.begin());
        check();
        return Scheduler::HaltCheck; // like interrupts, halt-checking should be performed immediately (cycle timestamp of 0)
    },0,false);
}

inline void Interrupts::haltCheck() {
    // while (IE & IF) == 0, step through the other events 
    while(((*reinterpret_cast<uint16_t*>(&IORegisters[0x200]) & 0x3FFF) &
           (*reinterpret_cast<uint16_t*>(&IORegisters[0x202]) & 0x3FFF)) == 0) {

        const uint32_t frontTimestamp = scheduler->eventList.front().timestamp;
        // update cycles passed
        if(frontTimestamp != 0)
            scheduler->cyclesPassedSinceLastFrame = frontTimestamp;

        scheduler->step();
    }
}