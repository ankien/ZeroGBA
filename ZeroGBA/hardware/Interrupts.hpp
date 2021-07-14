#pragma once
#include <functional>
#include <cstdint>
#include "cpu/CPUState.hpp"
#include "../Scheduler.hpp"

struct SoundController;

struct Interrupts {

    CPUState* cpuState;
    Scheduler* scheduler;
    uint8_t* IORegisters;
    uint16_t* internalTimer;
    SoundController* soundController;

    /// Scheduler Stuff ///
    void scheduleInterruptCheck();
    bool irqsEnabled();

    void scheduleHaltCheck();
    void haltCheck();

    void scheduleTimerStep(uint8_t,uint32_t); // called when timers are enabled
    void removeTimerSteps(uint8_t);
};


inline void Interrupts::scheduleInterruptCheck() {
    scheduler->addEventToFront([&]() {
        if((*reinterpret_cast<uint16_t*>(&IORegisters[0x200]) & 0x3FFF) &
           (*reinterpret_cast<uint16_t*>(&IORegisters[0x202]) & 0x3FFF))
            if(irqsEnabled())
                cpuState->handleException(NormalInterrupt,4,IRQ);
        return 0;
    },Scheduler::Interrupt,0,false);
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
        return 0; // like interrupts, halt-checking should be performed immediately (cycle timestamp of 0)
    },Scheduler::HaltCheck,0,false);
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
    scheduler->step();
}

inline void Interrupts::removeTimerSteps(uint8_t timerId) {
    scheduler->eventList.remove_if([=](const Scheduler::Event& event) { return event.eventType == timerId; });
}