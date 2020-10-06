#include "GBA.hpp"

GBA::GBA(std::string rom) {
    romMemory = loadRom(rom);
    
}

char* GBA::loadRom(std::string rom) {
    std::ifstream fileStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    char* romMemory = new char[romSizeInBytes];
    fileStream.read(romMemory,romSizeInBytes);
    return romMemory;
}