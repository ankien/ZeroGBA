#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <cstdio>
#include <SDL.h>
#include "hardware/cpu/ARM7TDMI.hpp"
#include "hardware/Interrupts.hpp"
#include "hardware/memory/GBAMemory.hpp"
#include "hardware/LCD.hpp"
#include "hardware/SoundController.hpp"
#include "hardware/Keypad.hpp"
#include "Scheduler.hpp"

#define DEBUG_VARS

struct GBA {

    /// Hardware ///
    Scheduler scheduler{};
    ARM7TDMI arm7tdmi{};
    Interrupts interrupts{};
    GBAMemory* systemMemory;
    LCD lcd{};
    SoundController soundController{}; // initialized after SDL in LCD
    Keypad keypad{};
    
    #ifdef DEBUG_VARS
    uint64_t instrCount = 0;
    #endif


    GBA();

    // todo: implement a cached interpreter
    void interpretARM();
    void interpretTHUMB();

    // Control helpers
    struct {
        uint8_t jit : 1; // not implemented lol
    } runtimeOptions{};
    bool parseArguments(uint64_t argc, char* argv[]);
    void run(char*);

    // Scheduler events
    const std::function<uint32_t()> postFrame = [&]() {
        scheduler.cyclesPassedSinceLastFrame = 0;
        systemMemory->IORegisters[4] &= 0xFE; // turn off vblank
        scheduler.addEventToFront([&]() {scheduler.resetEventList(); return 0;},Scheduler::Interrupt,0,false);

        // todo: implement JIT polling and run ahead - https://byuu.net/input/latency/
        keypad.pollInputs();

        lcd.draw();

        lcd.millisecondsElapsedAtLastFrame = SDL_GetTicks();

        return 280896;
    };
    const std::function<uint32_t()> startHBlank = [&]() {
        systemMemory->IORegisters[4] |= 0x2; // turn on hblank
        if(DISPSTAT_HBLANK_IRQ) {
            systemMemory->IORegisters[0x202] |= 0x2;// set hblank REG_IF
            interrupts.scheduleInterruptCheck();
        }
        if(VCOUNT < 160) {
            lcd.renderScanline(); // draw visible line
            // Increment internal reference point registers
            for(int8_t i = 0; i < 2; i++) {
                systemMemory->internalRef[i].x += systemMemory->memoryArray<int16_t>(0x4000022 + i*0x10);
                systemMemory->internalRef[i].y += systemMemory->memoryArray<int16_t>(0x4000026 + i*0x10);
            }
            systemMemory->delayedDma<0x2000>();
        }
        return 1232;
    };
    const std::function<uint32_t()> endHBlank = [&]() {
        systemMemory->IORegisters[4] &= 0xFD; // turn off hblank
        systemMemory->IORegisters[6] = (VCOUNT+1) % 228; // increment vcount
        VCOUNT == DISPSTAT_VCOUNT_SETTING ? systemMemory->IORegisters[4] |= 0x4 : systemMemory->IORegisters[4] &= 0xFB ; // set v-counter flag
        if(DISPSTAT_VCOUNT_IRQ) {
            if(DISPSTAT_VCOUNTER_FLAG)
                systemMemory->IORegisters[0x202] |= 0x4;
            else
                return 1232;
            
            interrupts.scheduleInterruptCheck();
        }
        return 1232;
    };
    const std::function<uint32_t()> startVBlank = [&]() {
        systemMemory->IORegisters[4] |= 0x1; // set vblank
        // Copy reference point registers to internal ones
        for(int8_t i = 0; i < 2; i++) {
            systemMemory->internalRef[i].x = systemMemory->memoryArray<uint32_t>(0x4000028 + i*0x10) & 0xFFFFFFF;
            systemMemory->internalRef[i].y = systemMemory->memoryArray<uint32_t>(0x400002C + i*0x10) & 0xFFFFFFF;
        }
        if(DISPSTAT_VBLANK_IRQ) {
            systemMemory->IORegisters[0x202] |= 0x1;// set vblank REG_IF
            interrupts.scheduleInterruptCheck();
        }
        systemMemory->delayedDma<0x1000>();
        return 280896;
    };
};