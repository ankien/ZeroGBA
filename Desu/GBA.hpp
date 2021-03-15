#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <cstdio>
#include <SDL.h>
#include "core/ARM7TDMI.hpp"
#include "hardware/InterruptControl.hpp"
#include "hardware/GBAMemory.hpp"
#include "hardware/LCD.hpp"
#include "hardware/Keypad.hpp"
#include "Scheduler.hpp"

struct GBA {

    /// Hardware ///
    ARM7TDMI arm7tdmi{};
    InterruptControl interrupts{};
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
        scheduler.resetEventList();

        // todo: implement JIT polling and run ahead - https://byuu.net/input/latency/
        keypad.pollInputs();

        lcd.draw();

        // naive sync to video approach
        // roughly 1000ms / 60fps - delay since start of last frame draw
        // window's low resolution clock conveniently makes us run at 60 fps instead of 62 without error accumulation lmao
        if(keypad.notSkippingFrames)
            SDL_Delay(16 - ((lcd.currMillseconds - lcd.millisecondsElapsedAtLastFrame) % 16));

        lcd.millisecondsElapsedAtLastFrame = SDL_GetTicks();
        systemMemory->IORegisters[4] ^= 0x1;; // disable vblank

        return 280896;
    };
    std::function<uint32_t()> startHBlank = [&]() {
        lcd.fetchScanline(); // draw visible line
        systemMemory->IORegisters[4] |= 0x2; // turn on hblank
        return 1232;
    };
    std::function<uint32_t()> endHBlank = [&]() {
        systemMemory->IORegisters[4] ^= 0x2; // turn off hblank
        return 1232;
    };
    std::function<uint32_t()> startVBlank = [&]() {
        systemMemory->IORegisters[4] |= 0x1; // set vblank
        return 197120;
    };
};