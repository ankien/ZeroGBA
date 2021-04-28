#pragma once
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include "../MMIO.h" // for getting mmio fields
#include "../Interrupts.hpp"

// debug console print, reeeally slow, like 1 fps slow
//#define PRINT_INSTR
// file-based trace, select # of instructions you want to trace from boot, prints to log.txt
//#define TRACE 500000

struct GBAMemory {

    Interrupts* interrupts;

    /*
struct DMA {
    
    GBAMemory* systemMemory;
    Interrupts* interrupts;

    const uint32_t sourceAddressMasks[4] = {0x07FFFFFF,0x0FFFFFFF,0x0FFFFFFF,0x0FFFFFFF}; 
    const uint32_t destAddressMasks[4] = {0x07FFFFFF,0x07FFFFFF,0x07FFFFFF,0x0FFFFFFF};
    const uint16_t lengthMasks[4] = {0x3FFF,0x3FFF,0x3FFF,0xFFFF};

    void triggerDma();
};
*/

    uint8_t bios[0x4000];
    uint8_t wramOnBoard[0x40000]; // AKA EWRAM
    uint8_t wramOnChip[0x8000]; // AKA IWRAM
    uint8_t IORegisters[0x3FF];
    uint8_t pram[0x400];
    uint8_t vram[0x18000];
    uint8_t oam[0x400];
    uint8_t gamePak[0x1000000];
    uint8_t gPakSram[0x10000];
    // This is the "address that's returned when there's an unmapped read
    // todo: handle unused memory reads correctly (writes are handled)
    uint8_t unusedMemoryAccess;

    // todo: create a saveRom function for different storage types (None, EEPROM-512/8, SRAM-32, Flash-64/128)
    bool loadRom(std::string);

    // memory helper functions
    template<typename T> T& memoryArray(uint32_t); // address is aligned by bytes for all types
    template<typename T> uint32_t writeable(uint32_t, T);
    void storeValue(uint8_t, uint32_t);
    void storeValue(uint16_t, uint32_t);
    void storeValue(uint32_t, uint32_t);
    // memory getters, rotates are for misaligned ldr+swp
    uint8_t readByte(uint32_t);
    uint16_t readHalfWord(uint32_t);
    uint32_t readHalfWordRotate(uint32_t);
    uint32_t readWord(uint32_t);
    uint32_t readWordRotate(uint32_t);

    uint32_t ror(uint32_t, uint8_t);
};

#include "memoryHelpers.inl"