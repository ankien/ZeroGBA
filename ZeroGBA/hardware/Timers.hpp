#pragma once
#include <cstdint>
#include "../Scheduler.hpp"
#include "Interrupts.hpp"

struct SoundController;

struct Timers {

    Interrupts* interrupts;
    Scheduler* scheduler;
    SoundController* soundController;
    uint8_t* IORegisters;

    uint16_t internalTimer[4]{};
    
    void removeTimerStep(uint8_t);
    void scheduleTimerStep(uint8_t,uint64_t); // called when timers are enabled
};