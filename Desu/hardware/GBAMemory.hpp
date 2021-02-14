#pragma once
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include "MMIO.h" // for getting mmio fields

struct GBAMemory {

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

    // for getting and setting memory
    uint8_t& operator[](uint32_t);

    // todo: create a saveRom function for different storage types (None, EEPROM-512/8, SRAM-32, Flash-64/128)
    bool loadRom(std::string);
};