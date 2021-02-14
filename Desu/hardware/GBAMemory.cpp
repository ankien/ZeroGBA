#include "GBAMemory.hpp"

uint8_t& GBAMemory::operator[](uint32_t i) {
    switch(i >> 24) {
        case 0x00:
            return bios[i];
        case 0x02:
            return wramOnBoard[(i-0x2000000) % 0x40000];
        case 0x03:
            return wramOnChip[(i-0x3000000) % 0x8000];
        case 0x04:
            return IORegisters[i-0x4000000];
        case 0x05:
            return pram[(i-0x5000000) % 0x400];
        case 0x06:
            i = (i-0x6000000) % 0x20000;
            if(i > 0x17FFF)
                i -= 0x8000;
            return vram[i];
        case 0x07:
            return oam[(i-0x7000000) % 0x400];
        case 0x08:
        case 0x09:
            return gamePak[i-0x8000000];
        case 0x0A:
        case 0x0B:
            return gamePak[i-0xA000000];
        case 0x0C:
        case 0x0D:
            return gamePak[i-0xC000000];
        case 0x0E:
            return gPakSram[i-0xE000000];
        default:
            return unusedMemoryAccess;
            break;
    }
}

bool GBAMemory::loadRom(std::string rom) {
    // load BIOS
    std::ifstream biosStream("gba_bios.bin", std::ifstream::in | std::ifstream::binary);
    biosStream.read(reinterpret_cast<char*>(bios),0x4000);

    // load ROM
    std::ifstream romStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    if(romSizeInBytes > 0x1FFFFFF) {
        printf("ROM size too large!\n");
        return 0;
    }
    romStream.read(reinterpret_cast<char*>(gamePak),romSizeInBytes);
    return 1;
}