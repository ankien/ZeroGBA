#include "GBAMemory.hpp"

uint8_t& GBAMemory::operator[](uint32_t i) {
    // unmapped memory accesses will crash! this isn't supposed to happen in the first place but the GBA handles it fine
    switch(i >> 24) {
        case 0x00:
            return bios[i];
        case 0x02:
            return wramOnBoard[i-0x2000000];
        case 0x03:
            return wramOnChip[i-0x3000000];
        case 0x04:
            return IORegisters[i-0x4000000];
        case 0x05:
            return pram[i-0x5000000];
        case 0x06:
            return vram[i-0x6000000];
        case 0x07:
            return oam[i-0x7000000];
        case 0x08:
            return gamePak[i-0x8000000];
        case 0x0E:
            return gPakSram[i-0xE000000];
        default: // unmapped memory access, will crash! todo: handle unpredictable behavior
            break;
    }
}

// todo: make this constructor accept args to
// turn it into a general memory class for more than one system
GBAMemory::GBAMemory() {
    bios = new uint8_t[0x4000];
    wramOnBoard = new uint8_t[0x40000];
    wramOnChip = new uint8_t[0x8000];
    IORegisters = new uint8_t[0x3FF];
    pram = new uint8_t[0x400];
    vram = new uint8_t[0x18000];
    oam = new uint8_t[0x400];
    gamePak = new uint8_t[0x3000000];
    gPakSram = new uint8_t[0x10000];


}

void GBAMemory::setMappedIO(uint16_t addr, uint8_t num) {
    switch(addr) {
        case 0x000: dispcnt &= 0xFF00 | num; break;
        case 0x001: dispcnt &= 0x00FF | (num << 8); break;
        case 0x002: greenSwap &= 0xFF00 | num; break;
        case 0x003: greenSwap &= 0x00FF | (num << 8); break;
        case 0x004: dispstat &= 0xFF00 | num; break;
        case 0x005: dispstat &= 0x00FF | (num << 8); break;
        case 0x006: vcount &= 0xFF00 | num; break;
        case 0x007: vcount &= 0x00FF | (num << 8); break;
        case 0x008: bgcnt0 &= 0xFF00 | num; break;
        case 0x009: bgcnt0 &= 0x00FF | (num << 8); break;
        case 0x00A: bgcnt1 &= 0xFF00 | num; break;
        case 0x00B: bgcnt1 &= 0x00FF | (num << 8); break;
        case 0x00C: bgcnt2 &= 0xFF00 | num; break;
        case 0x00D: bgcnt2 &= 0x00FF | (num << 8); break;
        case 0x00E: bgcnt3 &= 0xFF00 | num; break;
        case 0x00F: bgcnt3 &= 0x00FF | (num << 8); break;
        case 0x010: bghofs0 &= 0xFF00 | num; break;
        case 0x011: bghofs0 &= 0x00FF | (num << 8); break;
        case 0x012: bgvofs0 &= 0xFF00 | num; break;
        case 0x013: bgvofs0 &= 0x00FF | (num << 8); break;
        case 0x014: bghofs1 &= 0xFF00 | num; break;
        case 0x015: bghofs1 &= 0x00FF | (num << 8); break;
        case 0x016: bgvofs1 &= 0xFF00 | num; break;
        case 0x017: bgvofs1 &= 0x00FF | (num << 8); break;
        case 0x018: bghofs2 &= 0xFF00 | num; break;
        case 0x019: bghofs2 &= 0x00FF | (num << 8); break;
        case 0x01A: bgvofs2 &= 0xFF00 | num; break;
        case 0x01B: bgvofs2 &= 0x00FF | (num << 8); break;
        case 0x01C: bghofs3 &= 0xFF00 | num; break;
        case 0x01D: bghofs3 &= 0x00FF | (num << 8); break;
        case 0x01E: bgvofs3 &= 0xFF00 | num; break;
        case 0x01F: bgvofs3 &= 0x00FF | (num << 8); break;
        case 0x040: winh0 &= 0xFF00 | num; break;
        case 0x041: winh0 &= 0x00FF | (num << 8); break;
        case 0x042: winh1 &= 0xFF00 | num; break;
        case 0x043: winh1 &= 0x00FF | (num << 8); break;
        case 0x044: winv0 &= 0xFF00 | num; break;
        case 0x045: winv0 &= 0x00FF | (num << 8); break;
        case 0x046: winv1 &= 0xFF00 | num; break;
        case 0x047: winv1 &= 0x00FF | (num << 8); break;
        case 0x048: winin &= 0xFF00 | num; break;
        case 0x049: winin &= 0x00FF | (num << 8); break;
        case 0x04A: winout &= 0xFF00 | num; break;
        case 0x04B: winout &= 0x00FF | (num << 8); break;
        case 0x04C: mosaic &= 0xFF00 | num; break;
        case 0x04D: mosaic &= 0x00FF | (num << 8); break;
        case 0x050: bldcnt &= 0xFF00 | num; break;
        case 0x051: bldcnt &= 0x00FF | (num << 8); break;
        case 0x052: bldalpha &= 0xFF00 | num; break;
        case 0x053: bldalpha &= 0x00FF | (num << 8); break;
        case 0x054: bldy &= 0xFF00 | num; break;
        case 0x055: bldy &= 0x00FF | (num << 8); break;
    }
}

// todo: create a saveRom function for different storage types (None, EEPROM-512/8, SRAM-32, Flash-64/128)
bool GBAMemory::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    if(romSizeInBytes > 0x1FFFFFF)
        return 0;
    fileStream.read(reinterpret_cast<char*>(gamePak),romSizeInBytes);
    return 1;
}