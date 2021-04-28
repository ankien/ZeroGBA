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
#include "hardware/Keypad.hpp"
#include "Scheduler.hpp"

struct GBA {

    /// Hardware ///
    ARM7TDMI arm7tdmi{};
    Interrupts interrupts{};
    GBAMemory* systemMemory;
    LCD lcd{};
    Keypad keypad{};
    Scheduler scheduler{};

    GBA();

    // todo: implement a cached interpreter (fetch a batch of instructions before interpretation)
    void interpretARM();
    void interpretTHUMB();

    // Control helpers
    struct {
        uint8_t jit : 1; // not implemented lol
    } runtimeOptions{};
    bool parseArguments(uint64_t argc, char* argv[]);
    void run(char*);

    // Scheduler events
    std::function<uint32_t()> postFrame = [&]() {
        scheduler.cyclesPassedSinceLastFrame -= 280896;
        systemMemory->IORegisters[4] ^= 0x1; // turn off vblank
        scheduler.resetEventList();

        // todo: implement JIT polling and run ahead - https://byuu.net/input/latency/
        keypad.pollInputs();

        lcd.draw();

        // naive sync to video approach
        // roughly 1000ms / 60fps - delay since start of last frame draw
        // todo: add error accumulation (to account for 2 frames over 60),
        // and fix it so that frame times > 16 don't delay the next frame (frame time accumulation)
        if(keypad.notSkippingFrames)
            SDL_Delay(16 - ((lcd.currMillseconds - lcd.millisecondsElapsedAtLastFrame) % 16));

        lcd.millisecondsElapsedAtLastFrame = SDL_GetTicks();

        return 280896;
    };
    std::function<uint32_t()> startHBlank = [&]() {
        systemMemory->IORegisters[4] |= 0x2; // turn on hblank
        if(DISPSTAT_HBLANK_IRQ) {
            systemMemory->IORegisters[0x202] |= 0x2;// set hblank REG_IF
            interrupts.scheduleInterruptCheck();
        }
        lcd.renderScanline(); // draw visible line
        return 1232;
    };
    std::function<uint32_t()> endHBlank = [&]() {
        systemMemory->IORegisters[4] ^= 0x2; // turn off hblank
        systemMemory->IORegisters[6] = (VCOUNT+1) % 228; // increment vcount
        systemMemory->IORegisters[4] &= 0xFB;
        systemMemory->IORegisters[4] |= ((VCOUNT == DISPSTAT_VCOUNT_SETTING) << 2); // set v-counter flag
        if(DISPSTAT_VCOUNT_IRQ) {
            if(DISPSTAT_VCOUNTER_FLAG)
                systemMemory->IORegisters[0x202] |= 0x4;
            else
                return 1232;

            interrupts.scheduleInterruptCheck();
        }
        return 1232;
    };
    std::function<uint32_t()> startVBlank = [&]() {
        systemMemory->IORegisters[4] |= 0x1; // set vblank
        if(DISPSTAT_VBLANK_IRQ) {
            systemMemory->IORegisters[0x202] |= 0x1;// set vblank REG_IF
            interrupts.scheduleInterruptCheck();
        }
        return 197120;
    };
};