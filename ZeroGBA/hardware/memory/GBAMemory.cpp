#include "GBAMemory.hpp"

uint8_t GBAMemory::getUnusedMemType(uint32_t address) {
    
    if(address <= 0x3FFF) {
        if(cpuState->r[15] >> 24 == 0x00)
            return NotUnused;
        return Bios;
    }

    if(address > 0xFFFFFFF)
        return GenericUnused;
    
    switch(address >> 24) 	{
        case 0x04:
            if(address > 0x40003FF)
                return GenericUnused;
            break;
        case 0x00:
        case 0x01:
            return GenericUnused;
    }

    return NotUnused;
}
uint32_t GBAMemory::readUnusedMem(bool thumb,uint8_t memType) {
    
    if(memType == Bios)
        switch(stateRelativeToBios) {
            case AfterStartupOrReset:
                return *reinterpret_cast<uint32_t*>(&bios[0xDC + 8]);
                break;
            case DuringIRQ:
                return *reinterpret_cast<uint32_t*>(&bios[0x134 + 8]);
                break;
            case AfterIRQ:
                return *reinterpret_cast<uint32_t*>(&bios[0x13C + 8]);
                break;
            case AfterSWI:
                return *reinterpret_cast<uint32_t*>(&bios[0x188 + 8]);
        }
    else {
        if(!thumb)
            return memoryArray<uint32_t>(cpuState->r[15] + 8);
        else {
            uint32_t pcVal = cpuState->r[15];
            switch(pcVal >> 24) {
                case 0x00:
                case 0x07:
                    if(pcVal % 4)
                        return memoryArray<uint32_t>(pcVal+2);
                    else
                        return memoryArray<uint32_t>(pcVal+4);
                case 0x03:
                    if(pcVal % 4)
                        return memoryArray<uint16_t>(pcVal+4) << 16 | memoryArray<uint16_t>(pcVal+2);
                    else
                        return memoryArray<uint32_t>(pcVal+2);
                default:
                    return memoryArray<uint16_t>(pcVal+4) << 16 | memoryArray<uint16_t>(pcVal+4);
            }
        }
    }
}

bool GBAMemory::loadRom(std::string rom) {
    // load BIOS
    std::ifstream biosStream("gba_bios.bin", std::ifstream::in | std::ifstream::binary);
    biosStream.read(reinterpret_cast<char*>(bios),0x4000);

    // load ROM
    std::ifstream romStream(rom.c_str(), std::ifstream::in | std::ifstream::binary);
    uint32_t romSizeInBytes = std::filesystem::file_size(rom);
    if(romSizeInBytes > 0x02000000) {
        printf("ROM size too large!\n");
        return 0;
    }
    romStream.read(reinterpret_cast<char*>(gamePak),romSizeInBytes);
    return 1;
}