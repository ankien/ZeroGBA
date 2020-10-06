#include "GBA.hpp"

GBA::GBA() {
    
}

bool GBA::loadRom(std::string rom) {
    std::fstream fileStream(rom, std::ios::binary | std::ios::in);

    if(!fileStream.is_open())
        return false;

    if(std::filesystem::file_size(rom) > 32000000)
        return false;

    rom.insert(rom.begin(),
               std::istream_iterator<uint8_t>(fileStream),
               std::istream_iterator<uint8_t>());
    
    fileStream.close();
    return true;
}