#include "GBA.hpp"

GBA::GBA(std::string rom) {
    romMemory = loadRom(rom);
    ARM7TDMI arm7 = {};
}

uint8_t* GBA::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    romMemory = new uint8_t[romSizeInBytes];
    fileStream.read(reinterpret_cast<char*>(romMemory),romSizeInBytes);
    return romMemory;
}