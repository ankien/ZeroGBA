#pragma once
#include <stdint.h>
#include <fstream>
#include <filesystem>

struct GBAMemory {
    uint8_t* bios;
    uint8_t* wramOnBoard;
    uint8_t* wramOnChip;
    uint8_t* IORegisters;
    uint8_t* pram;
    uint8_t* vram;
    uint8_t* oam;
    uint8_t* gamePak;
    uint8_t* gPakSram;

    /// MMIO ///
    struct {
        uint8_t bgMode;
        bool cgbMode, displayFrameSelect, hBlankIntervalFree, objCharacterVramMapping, forcedBlank,
        screenDisplayBG0, screenDisplayBG1, screenDisplayBG2, screenDisplayBG3, screenDisplayOBJ,
        window0DisplayFlag, window1DisplayFlag, objWindowDisplayFlag;
    } dispcnt;

    // for getting memory (can be used for setting but don't pls)
    uint8_t& operator[](uint32_t);

    // for setting memory, only bytes can be set
    void setByte(uint32_t,uint8_t);

    GBAMemory();

    void setMappedIO(uint16_t,uint8_t);

    bool loadRom(std::string);
};

inline void GBAMemory::setByte(uint32_t i, uint8_t num) {
    // every time you set a byte in memory, check to see
    // if it overlaps with a MMIO field, then set it if so
    if((i & 0xFFFF000) == 0x4000000)
        setMappedIO(i & 0xFFF,num);
    (*this)[i] = num;
}