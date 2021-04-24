#include "GBAMemory.hpp"

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