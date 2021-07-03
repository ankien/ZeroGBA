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

void* GBAMemory::createSaveMap(std::string& romName) {
    const char* handler[5];
    handler[0] = ("EEPROM_"); // 512 bytes or 8KB
    handler[1] = ("SRAM_"); // uses SRAM (32KB)
    handler[2] = ("FLASH_"); // 64KB
    handler[3] = ("FLASH512_"); // 64KB
    handler[4] = ("FLASH1M_"); // 128KB
    
    romSaveType = EEPROM_V;
    std::string gPakString((char*)gamePak,0x2000000);
    for(const char* handle : handler)
        if(gPakString.find(handle) != std::string::npos)
            break;
        else
            romSaveType++;

    saveFile = romName+".sav";
    bool fileDidNotExist = !std::filesystem::exists(saveFile);
    uint32_t fileSize;
    void* fileEntry = nullptr;
    switch(romSaveType) 	{
        case EEPROM_V:
            romSaveType = EEPROM_V;
            // leave EEPROM memory uninitialized for now and determine size on the first read
            break;
        default: // just default to SRAM if unknown/unspecified
        case SRAM_V:
            romSaveType = SRAM_V;
            fileSize = 0x8000;
            fileEntry = createFileMap(romName+".sav",fileSize);
            break;
        case FLASH_V:
        case FLASH512_V:
            id = 0x1B32;
            romSaveType = FLASH_V;
            fileSize = 0x10000;
            fileEntry = createFileMap(romName+".sav",fileSize);
            break;
        case FLASH1M_V:
            id = 0x1362;
            romSaveType = FLASH1M_V;
            fileSize = 0x20000;
            fileEntry = createFileMap(romName+".sav",fileSize);
            break;
    }

    if(fileDidNotExist && romSaveType != EEPROM_V)
        memset(fileEntry,0xFF,fileSize);

    return fileEntry;
}
bool GBAMemory::loadRom(std::string& rom) {
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
    largerThan16KB = romSizeInBytes > 0x1000000 ? true : false ;
    romStream.read(reinterpret_cast<char*>(gamePak),romSizeInBytes);

    // detect / create save
    rom.erase(rom.find_last_of("."),std::string::npos);
    gPakSaveMem = reinterpret_cast<uint8_t*>(createSaveMap(rom));
    if(gPakSaveMem == nullptr && romSaveType != EEPROM_V) {
        printf("Error in detecting/creating save.\n");
        return 0;
    }

    return 1;
}