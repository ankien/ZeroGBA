#pragma once
#include <cstdint>
#include "Interrupts.hpp"
#include "SoundController.hpp"
#include "../Scheduler.hpp"

struct Timers {

    Interrupts* interrupts;
    SoundController* soundController;
    Scheduler* scheduler;
    uint8_t* IORegisters;

    static constexpr uint64_t period[4] = {1,64,256,1024};
    uint16_t internalTimer[4]{}; // current 16-bit counter values, regular timers are lazily-checked, while cascade are updated
    uint64_t enableTimestamp[4]{};
    
    void removeOverflow(uint8_t);
    void scheduleOverflow(uint8_t,uint8_t); // handles events based on overflow of timers, does not tick regular timers internally! (only cascading)
    void updateTimer(uint8_t,uint8_t); // update the current (regular) internal timer on MMIO read if enabled; does nothing for cascade
};