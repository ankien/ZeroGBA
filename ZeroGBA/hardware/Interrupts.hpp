#pragma once
#include <cstdint>
#include "cpu/CPUState.hpp"
#include "../Scheduler.hpp"

struct Interrupts {

    CPUState* cpuState;
    Scheduler* scheduler;
    uint8_t* IORegisters;
    bool fuck = false;

    /// Scheduler Stuff ///
    void scheduleInterruptCheck();
    bool irqsEnabled();
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