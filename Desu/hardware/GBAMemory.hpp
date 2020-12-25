#pragma once
#include <cstdint>
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

    /// LCD ///
    uint16_t dispcnt;
    uint16_t greenSwap;
    uint16_t dispstat;
    uint16_t vcount;
    uint16_t bgcnt0;
    uint16_t bgcnt1;
    uint16_t bgcnt2;
    uint16_t bgcnt3;
    uint16_t bghofs0;
    uint16_t bghofs1;
    uint16_t bghofs2;
    uint16_t bghofs3;
    uint16_t bgvofs0;
    uint16_t bgvofs1;
    uint16_t bgvofs2;
    uint16_t bgvofs3;
    // todo: rotation and scaling regs
    uint16_t winh0;
    uint16_t winh1;
    uint16_t winv0;
    uint16_t winv1;
    uint16_t winin;
    uint16_t winout;
    uint16_t mosaic;
    uint16_t bldcnt;
    uint16_t bldalpha;
    uint16_t bldy;

    // MMIO field helpers
    uint32_t getBits(uint32_t,uint8_t,uint8_t);
    uint16_t getBits(uint16_t,uint8_t,uint8_t);

    // for getting memory (can be used for setting but don't pls)
    uint8_t& operator[](uint32_t);

    // for setting memory, only bytes can be set
    void setByte(uint32_t,uint8_t);

    void setMappedIO(uint16_t,uint8_t);

    // todo: create a saveRom function for different storage types (None, EEPROM-512/8, SRAM-32, Flash-64/128)
    bool loadRom(std::string);
};

inline uint32_t GBAMemory::getBits(uint32_t val, uint8_t startBit, uint8_t endBit) {
    uint8_t lshiftAmount = 31-endBit;
    return (val << lshiftAmount) >> (lshiftAmount+startBit);
}
inline uint16_t GBAMemory::getBits(uint16_t val, uint8_t startBit, uint8_t endBit) {
    uint8_t lshiftAmount = 16-endBit;
    return (val << lshiftAmount) >> (lshiftAmount+startBit);
}

inline void GBAMemory::setByte(uint32_t i, uint8_t num) {
    // every time you set a byte in memory, check to see
    // if it overlaps with a MMIO field, then set it if so
    if((i & 0xFFFF000) == 0x4000000)
        setMappedIO(i & 0xFFF,num);
    (*this)[i] = num;
}