#include "GBAMemory.hpp"

uint8_t& GBAMemory::operator[](uint32_t i) {
    switch(i >> 24) {
        case 0x00:
            if(i > 0x00003FFF)
                return unusedMemoryAccess;
            return bios[i];
        case 0x02:
            if(i > 0x0203FFFF)
                return unusedMemoryAccess;
            return wramOnBoard[i-0x2000000];
        case 0x03:
            if(i > 0x03007FFF)
                return unusedMemoryAccess;
            return wramOnChip[i-0x3000000];
        case 0x04:
            if(i > 0x040003FE)
                return unusedMemoryAccess;
            return IORegisters[i-0x4000000];
        case 0x05:
            if(i > 0x050003FF)
                return unusedMemoryAccess;
            return pram[i-0x5000000];
        case 0x06:
            if(i > 0x06017FFF)
                return unusedMemoryAccess;
            return vram[i-0x6000000];
        case 0x07:
            if(i > 0x070003FF)
                return unusedMemoryAccess;
            return oam[i-0x7000000];
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            return gamePak[i-0x8000000];
        case 0x0E:
            if(i > 0x0E00FFFF)
                return unusedMemoryAccess;
            return gPakSram[i-0xE000000];
        default:
            return unusedMemoryAccess;
            break;
    }
}

bool GBAMemory::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    if(romSizeInBytes > 0x1FFFFFF)
        return 0;
    fileStream.read(reinterpret_cast<char*>(gamePak),romSizeInBytes);
    return 1;
}