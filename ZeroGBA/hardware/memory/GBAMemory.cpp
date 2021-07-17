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
                    if(pcVal & 3)
                        return memoryArray<uint32_t>(pcVal+2);
                    else
                        return memoryArray<uint16_t>(pcVal+4) * 0x00010001;
                case 0x03:
                    if(pcVal & 3)
                        return memoryArray<uint16_t>(pcVal+4) << 16 | memoryArray<uint16_t>(pcVal+2);
                    else
                        return memoryArray<uint16_t>(pcVal+2) << 16 | memoryArray<uint16_t>(pcVal+4);
                default:
                    return memoryArray<uint16_t>(pcVal+4) * 0x00010001;
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

void* GBAMemory::createFileMap(std::string saveFile, uint32_t sizeInBytes) {
    HANDLE file = CreateFileA(saveFile.c_str(),GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    return MapViewOfFile(
        CreateFileMappingA(file,NULL,PAGE_READWRITE,0,sizeInBytes,NULL),
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0
    );
}

// return bitmask to write with, also handles special write behavior for certain regions (like IF regs)
template<typename T>
uint32_t GBAMemory::writeable(uint32_t address, uint32_t unalignedAddress, T value) {
    
    uint8_t addressSection = address >> 24;
    

    switch(addressSection) {
        case 0x00:
            return 0x0;
    
        // Some IO regs have special behavior, this is how I handle them
        // Gotta keep in mind that this approach might not work if the MMIO fields overlap
        // Also, word stores/reads are never unaligned
        case 0x04:
        {
            constexpr uint8_t offset = sizeof(T) - 1;
            uint16_t ioAddress = address & 0xFFF;

            // Affine BG reference point regs
            if(ioAddress < 0x40 && ioAddress > 0x27 - offset) {
                memoryArray<T>(address) = value;
                if(ioAddress < 0x2C && ioAddress > 0x27 - offset)
                    internalRef[0].x = memoryArray<uint32_t>(0x4000028) & 0xFFFFFFF;

                if(ioAddress < 0x30 && ioAddress > 0x2B - offset)
                    internalRef[0].y = memoryArray<uint32_t>(0x400002C) & 0xFFFFFFF;

                if(ioAddress < 0x3C && ioAddress > 0x37 - offset)
                    internalRef[1].x = memoryArray<uint32_t>(0x4000038) & 0xFFFFFFF;

                if(ioAddress < 0x40 && ioAddress > 0x3B - offset)
                    internalRef[1].y = memoryArray<uint32_t>(0x400003C) & 0xFFFFFFF;

                return 0x0;
            }

            // DMA sound
            else if(ioAddress < 0xA7 && ioAddress > 0x9F - offset) {
                const bool fifoChannel = ioAddress > 0xA3;
                if(soundController->currFifoSize[fifoChannel] < 32) {
                    soundController->fifos[fifoChannel][(soundController->currFifoBytePos[fifoChannel] + soundController->currFifoSize[fifoChannel]) % 32] = value;
                    soundController->currFifoSize[fifoChannel]++;
                }
            }

            // DMA regs
            else if(ioAddress < 0xE0 && ioAddress > 0xA9 - offset) {
                uint8_t channel;

                if(address < 0x40000BC)
                    channel = 0;
                else if(address < 0x40000C8)
                    channel = 1;
                else if(address < 0x40000D4)
                    channel = 2;
                else
                    channel = 3;

                // if CNT_H will be modified
                uint32_t dmaCntHAddr = 0x40000BA + 12 * channel;
                uint16_t oldCntH = memoryArray<uint16_t>(dmaCntHAddr);
                const bool oldEnable = oldCntH & 0x8000;

                memoryArray<T>(address) = value; // write the new value

                uint16_t newCntH = memoryArray<uint16_t>(dmaCntHAddr);

                if(newCntH & 0x8000 && !oldEnable) {
                    // reload SAD, DAD, and CNT_L
                    internalSrc[channel] = memoryArray<uint32_t>(0x40000B0 + 12 * channel) & sourceAddressMasks[channel];
                    internalDst[channel] = memoryArray<uint32_t>(0x40000B4 + 12 * channel) & destAddressMasks[channel];

                    // only trigger dmas if if the enable bit is 1 and old enable bit was 0
                    if((newCntH & 0x3000) == 0x0000)
                        dmaTransfer(channel, newCntH);
                }

                return 0x0;
            }

            // Timer control regs
            else if(ioAddress < 0x10F && ioAddress > 0x101 - offset) {
                // a rescheduable timer event (ticking) is added to the scheduler when a timer is enabled (when disabled), and removed when disabled or cascaded
                // if the timer cascading bit is modified while a timer is enabled, timer stops (or starts) immediately
                uint8_t timerId;
                if(ioAddress < 0x104)
                    timerId = 0;
                else if(ioAddress < 0x108)
                    timerId = 1;
                else if(ioAddress < 0x10C)
                    timerId = 2;
                else
                    timerId = 3;

                const uint32_t controlAddress = 0x4000102 + 4 * timerId;

                bool oldTimerEnable = memoryArray<uint8_t>(controlAddress) & 0x80;
                bool oldTimerCascade = memoryArray<uint8_t>(controlAddress) & 0x4;

                memoryArray<T>(address) = value;

                bool newTimerEnable = memoryArray<uint8_t>(controlAddress) & 0x80;
                bool newTimerCascade = memoryArray<uint8_t>(controlAddress) & 0x4;
                const uint8_t prescalerSelection = memoryArray<uint8_t>(controlAddress) & 0x3;
                if(newTimerEnable) {
                    if(newTimerCascade)
                        interrupts->removeTimerSteps(timerId);
                    else if(!oldTimerEnable || oldTimerCascade) { // if enabled or no longer cascade
                        if(!oldTimerEnable)
                            internalTimer[timerId] = memoryArray<uint16_t>(controlAddress - 2);
                        uint16_t shiftAmount = prescalerSelection > 0 ? 1 << prescalerSelection * 2 + 4 : 1;
                        interrupts->scheduleTimerStep(timerId, shiftAmount);
                    }
                } else if(oldTimerEnable) { // if disabled
                    //IORegisters[0x202] |= 1<<3+timerId;   // this is a hack!
                    //interrupts->scheduleInterruptCheck(); // todo: figure out why mgba-suite's timer tests are broken without these lines
                    internalTimer[timerId] = 0;
                    interrupts->removeTimerSteps(timerId);
                }

                return 0x0;
            }

            // Interrupt regs
            else if(ioAddress < 0x20A && ioAddress > 0x1FF - offset) {
                // if writing to IE or IME
                if(address >= 0x4000200 - offset && address < 0x4000202 || address >= 0x4000208 - offset && address <= 0x4000208) { // respective bit range for IO regs
                    interrupts->scheduleInterruptCheck();
                }

                // if value overlaps with IF
                if(address >= 0x4000202 - offset && address <= 0x4000203) {
                    uint16_t oldIF = memoryArray<uint16_t>(0x4000202);
                    memoryArray<T>(address) = value;
                    memoryArray<uint16_t>(0x4000202) = oldIF & ~memoryArray<uint16_t>(0x4000202);
                    return 0x0;
                }
            }

            // HALTCNT reg
            else if(ioAddress < 0x302 && ioAddress > 0x300 - offset) {
                // writing 0 here switches the GBA into power saving mode until the next interrupt (no instructions executed)
                // schedule an event that skips to the next event and reschedules itself after the current front until (IE & IF) != 0
                memoryArray<T>(address) = value;
                if((memoryArray<uint8_t>(0x4000301) & 0x80) == 0)
                    interrupts->scheduleHaltCheck();
                return 0x0;
            }

            return *reinterpret_cast<const uint32_t*>(&writeMask[ioAddress]);
        }
        break;

        case 0x08:
            return 0x0;
        
        case 0x09:
            if(address >= 0x9FFFF00 && romSaveType == EEPROM_V)
                eepromWrite(value);
            return 0x0;

        case 0x0A:
        case 0x0B:
        case 0x0C:
            return 0x0;

        case 0x0D:
            if(romSaveType == EEPROM_V) {
                if(largerThan16KB && address >= 0xDFFFF00)
                    eepromWrite(value);
                else
                    eepromWrite(value);
            }
            return 0x0;

        case 0x0E:

            if(sizeof(T) > 1) {
                value = ror(value, unalignedAddress * 8);
                address = unalignedAddress;
            }

            if(romSaveType == FLASH_V || romSaveType == FLASH1M_V) {

                // Prepare write and set bank can be considered extra "states" after PREPARE_WRITE and SET_MEM_BANK commands
                if(precedingFlashCommand == PREPARE_WRITE) {
                    precedingFlashCommand = NONE;
                    memoryArray<uint8_t>(address) = value;
                }

                else if(address == 0xE000000 && precedingFlashCommand == SET_MEM_BANK && romSaveType == FLASH1M_V) {
                    precedingFlashCommand = NONE;
                    secondFlashBank = value & 1;
                }

                else if(address == 0xE002AAA) {
                    if(flashState == CMD_1) {
                        if(value == 0x55)
                            flashState = CMD_2;
                    }
                }

                else if(address == 0xE005555) {
                    switch(flashState) {
                        case READY:
                        {
                            if(value == 0xAA)
                                flashState = CMD_1;
                            break;
                        }
                        case CMD_2:
                            switch(value) {
                                case ENTER_CHIP_ID_MODE:
                                    idMode = true;
                                    precedingFlashCommand = ENTER_CHIP_ID_MODE;
                                    flashState = READY;
                                    break;
                                case EXIT_CHIP_ID_MODE:
                                    idMode = false;
                                    precedingFlashCommand = EXIT_CHIP_ID_MODE;
                                    flashState = READY;
                                    break;
                                case PREPARE_ERASE:
                                    precedingFlashCommand = PREPARE_ERASE;
                                    flashState = READY;
                                    break;
                                case ERASE_CHIP:
                                    if(precedingFlashCommand == PREPARE_ERASE)
                                        memset(gPakSaveMem, 0xFF, 0x10000);
                                    precedingFlashCommand = ERASE_CHIP;
                                    flashState = READY;
                                    break;
                                case PREPARE_WRITE:
                                    precedingFlashCommand = PREPARE_WRITE;
                                    flashState = READY;
                                    break;
                                case SET_MEM_BANK:
                                    precedingFlashCommand = SET_MEM_BANK;
                                    flashState = READY;
                                    break;
                                default:
                                    flashState = READY;
                                    break;
                            }
                    }
                }

                else if((address & 0xFFF) == 0 && precedingFlashCommand == PREPARE_ERASE && value == ERASE_4KB_SECTOR) {
                    uint16_t addressLo = address & 0xFFFF;
                    memset(gPakSaveMem + 0x10000 * secondFlashBank + addressLo, 0xFF, 0x1000);
                    precedingFlashCommand = ERASE_4KB_SECTOR;
                    flashState = READY;
                }

                return 0x0;

            } else {
                memoryArray<uint8_t>(address) = value;
                return 0x0;
            }

            break;
        
    }

    return 0xFFFFFFFF;
}
template<typename T>
uint32_t GBAMemory::readValue(uint32_t address) {
    uint32_t value = memoryArray<T>(address);

    uint8_t addressSection = address >> 24;
    constexpr uint8_t offset = sizeof(T) - 1;

    // handle special MMIO reads
    switch(addressSection) {
        case 0x04:
        {
            uint16_t ioAddress = address & 0xFFF;

            // Timer Reload regs
            if(ioAddress < 0x10E && ioAddress >= 0x100 - offset) {
                uint8_t timerId;
                if(ioAddress < 0x102)
                    timerId = 0;
                else if(ioAddress < 0x106)
                    timerId = 1;
                else if(ioAddress < 0x10A)
                    timerId = 2;
                else
                    timerId = 3;

                uint16_t tempTimer;
                const uint32_t reloadAddress = 0x4000100 + 4 * timerId;
                tempTimer = memoryArray<uint16_t>(reloadAddress);
                memoryArray<uint16_t>(reloadAddress) = internalTimer[timerId];

                value = memoryArray<T>(address);

                memoryArray<uint16_t>(reloadAddress) = tempTimer;
            }
        }
        break;

        case 0x09:
            if(address >= 0x9FFFF00 && romSaveType == EEPROM_V)
                value = eepromRead();
            break;

        case 0x0D:
            if(romSaveType == EEPROM_V) {
                if(largerThan16KB && address >= 0xDFFFF00)
                    value = eepromRead();
                else
                    value = eepromRead();
            }
            break;

        case 0x0E:

            if(address < 0x0E000002 && address >= 0x0E000000 - offset) {
                if(idMode) {
                    uint16_t oldHalfword = *reinterpret_cast<uint16_t*>(&gPakSaveMem[0x0]);
                    *reinterpret_cast<uint16_t*>(&gPakSaveMem[0x0]) = id;
                    value = memoryArray<T>(address);
                    *reinterpret_cast<uint16_t*>(&gPakSaveMem[0x0]) = oldHalfword;
                }
            }

            if(sizeof(T) == 4)
                value = (value & 0xFF) * 0x01010101;
            else if(sizeof(T) == 2)
                value = (value & 0xFF) * 0x0101;
            break;
    }

    return value;
}
void GBAMemory::storeValue(uint8_t value, uint32_t address) {
    switch(address >> 24) {
        case 0x05:
            storeValue(static_cast<uint16_t>(static_cast<uint16_t>(value) * 0x101), address);
            break;
        case 0x06:
            if(IORegisters[0] < 3) { // bitmap mode writes
                if(address < 0x6014000)
                    storeValue(static_cast<uint16_t>(static_cast<uint16_t>(value) * 0x101), address);
            } else {
                if(address < 0x6010000)
                    storeValue(static_cast<uint16_t>(static_cast<uint16_t>(value) * 0x101), address);
            }
            return;
        case 0x07:
            return;
        default:
        {
            uint8_t mask = writeable<uint8_t>(address,address,value);
            uint8_t* mem = &memoryArray<uint8_t>(address);
            mem[0] = (value & mask) | (mem[0] & ~mask);
        }
    }
}
void GBAMemory::storeValue(uint16_t value, uint32_t address) {
    uint16_t mask = writeable<uint16_t>(address & ~1,address,value);
    uint16_t* mem = &memoryArray<uint16_t>(address & ~1);
    mem[0] = (value & mask) | (mem[0] & ~mask);
}
void GBAMemory::storeValue(uint32_t value, uint32_t address) {
    uint32_t mask = writeable<uint32_t>(address & ~3,address,value);
    uint32_t* mem = &memoryArray<uint32_t>(address & ~3);
    mem[0] = (value & mask) | (mem[0] & ~mask);
}
uint8_t GBAMemory::readByte(uint32_t address) {
    uint8_t memType = getUnusedMemType(address);
    if(memType)
        return readUnusedMem(cpuState->state,memType);
    return readValue<uint8_t>(address);
}
uint16_t GBAMemory::readHalfWord(uint32_t address) {
    uint8_t memType = getUnusedMemType(address);
    if(memType)
        return readUnusedMem(cpuState->state,memType);
    address = address & ~1;
    return readValue<uint16_t>(address);
}
uint32_t GBAMemory::readHalfWordRotate(uint32_t address) {
    uint8_t rorAmount = (address & 1) << 3;
    return ror(readHalfWord(address),rorAmount);
}
uint32_t GBAMemory::readWord(uint32_t address) {
    uint8_t memType = getUnusedMemType(address);
    if(memType)
        return readUnusedMem(cpuState->state,memType);
    address = address & ~3;
    return readValue<uint32_t>(address);
}
uint32_t GBAMemory::readWordRotate(uint32_t address) {
    uint8_t rorAmount = (address & 3) << 3;
    return ror(readWord(address),rorAmount);
}

uint32_t GBAMemory::ror(uint32_t value, uint8_t shiftAmount) {
    shiftAmount &= 0x1F;
    return (value >> shiftAmount) | (value << (32 - shiftAmount));
}

void GBAMemory::dmaTransfer(uint8_t channel, uint16_t dmaCntH) {
    const uint8_t destAddressControl = (dmaCntH & 0x60) >> 5;
    int8_t destCtrlFactor = destAddressControl;
    int8_t srcCtrlFactor = (dmaCntH & 0x180) >> 7;
    int16_t startTiming = dmaCntH & 0x3000;
    const bool wordTransfer = dmaCntH & 0x400;
    uint8_t transferSize = wordTransfer ? 4 : 2;
    uint32_t length = memoryArray<uint16_t>(0x40000B8 + 12 * channel) & lengthMasks[channel];
    if(length == 0)
        channel == 3 ? length = 0x10000 : length = 0x4000;

    destCtrlFactor = getIncrementFactor(destCtrlFactor);
    srcCtrlFactor = getIncrementFactor(srcCtrlFactor);

    if(startTiming == 0x3000) {
        switch(channel) 	{
            case 1:
            case 2:
                length = 4;
                transferSize = 4;
                destCtrlFactor = 0;
                break;
            case 3:
                break;
                // todo: video capture
        }
    }

    int8_t destIncrement = transferSize * destCtrlFactor;
    int8_t srcIncrement = transferSize * srcCtrlFactor;

    // todo: implement DMA latch
    #define DMA_LOOP(dataType) \
    if(channel == 3 && gPakSaveMem == nullptr && (addressesEepromChip(internalSrc[channel]) || addressesEepromChip(internalDst[channel]))) { \
        eepromAddressBits = length - 3; \
        uint16_t eepromSizeInBytes = length <= 9 ? 512 : 0x2000; \
        bool eepromSaveExists = std::filesystem::exists(saveFile); \
        gPakSaveMem = reinterpret_cast<uint8_t*>(createFileMap(saveFile,eepromSizeInBytes)); \
        if(!eepromSaveExists) \
            memset(gPakSaveMem,0xFF,eepromSizeInBytes); \
    } \
    for(uint32_t i = 0; i < length; i++) { \
        dataType value = readValue<dataType>(internalSrc[channel]); \
        dataType mask = writeable<dataType>(internalDst[channel], internalDst[channel], value); \
        dataType* mem = &memoryArray<dataType>(internalDst[channel]); \
        mem[0] = (value & mask) | (mem[0] & ~mask); \
        internalSrc[channel] += srcIncrement; \
        internalDst[channel] += destIncrement; \
    }
    if(wordTransfer) {
        internalSrc[channel] &= ~3;
        internalDst[channel] &= ~3;
        DMA_LOOP(uint32_t)
    } else {
        internalSrc[channel] &= ~1;
        internalDst[channel] &= ~1;
        DMA_LOOP(uint16_t)
    }

    // reset destination address
    if(destAddressControl == 3)
        internalDst[channel] = memoryArray<uint32_t>(0x40000B4 + 12*channel) & destAddressMasks[channel];

    // If the repeat bit is set at the end of a transfer; enable bit is set, else clear enable bit
    if(dmaCntH & 0x200 && startTiming != 0x0000) {
        memoryArray<uint8_t>(0x40000BB + 12*channel) |= 0x80;
    } else {
        memoryArray<uint8_t>(0x40000BB + 12*channel) &= 0x7F;
    }

    // check for DMA interrupts
    if(dmaCntH & 0x4000) {
        memoryArray<uint8_t>(0x4000202) |= 1 << channel;
        interrupts->scheduleInterruptCheck();
    }
}
int8_t GBAMemory::getIncrementFactor(uint8_t addressControl) {
    switch(addressControl) {
        case 0:
        case 3:
            return 1;
        case 1:
            return -1;
        case 2:
            return 0;
    }
}

void GBAMemory::eepromWrite(uint8_t value) {
    if(eepromState == READ_DATA) return;

    value &= 1;

    serialBuffer = (serialBuffer << 1) | value;
    transmittedBits++;

    if(eepromState == BEFORE_REQUEST && transmittedBits == 2) {
        switch(serialBuffer) {
            case 0b10:
                currentEepromRequest = EEPROM_WRITE;
                break;
            case 0b11:
                currentEepromRequest = EEPROM_READ;
                break;
        }
        eepromState = AFTER_REQUEST;
        resetSerialBuffer();
    } else if(eepromState == AFTER_REQUEST) {
        if(transmittedBits == eepromAddressBits) {
            serialBuffer &= 0x3FF; // only use 10 bit address at most
            eepromAddress = serialBuffer * 8;

            if(currentEepromRequest == EEPROM_WRITE)
                memset(gPakSaveMem+eepromAddress,0,8); // writing automatically erases the old 64 bits of data

            eepromState = currentEepromRequest == EEPROM_READ ? AFTER_ADDRESS : WRITE_DATA;
            resetSerialBuffer();
        }
    } else if(eepromState == AFTER_ADDRESS) { // handle dummy bit for read, handle dummy bit for write after data
        if(currentEepromRequest == EEPROM_READ)
            eepromState = READ_DATA;
        else if(currentEepromRequest == EEPROM_WRITE)
            eepromState = BEFORE_REQUEST;
        resetSerialBuffer();
    } else if(eepromState == WRITE_DATA) {
        uint8_t bit = (transmittedBits - 1) % 8;
        uint8_t eepromByteIndexOffset = (transmittedBits - 1) / 8;

        gPakSaveMem[eepromAddress + eepromByteIndexOffset] |= value << (7-bit);

        if(transmittedBits == 64) {
            eepromState = AFTER_ADDRESS;
            resetSerialBuffer();
        }
    }
}
bool GBAMemory::eepromRead() {
    if(eepromState == READ_DATA) {

        uint8_t bit = transmittedBits % 8;
        uint8_t eepromByteIndexOffset = transmittedBits / 8;
        transmittedBits++;
        if(transmittedBits <= 4 && currentEepromRequest == EEPROM_READ) {
            if(transmittedBits == 4) {
                resetSerialBuffer();
                currentEepromRequest = EEPROM_NONE;
            }
            return 0;
        }

        if(transmittedBits == 64) {
            eepromState = BEFORE_REQUEST;
            resetSerialBuffer();
        }

        return (gPakSaveMem[eepromAddress + eepromByteIndexOffset] >> (7-bit)) & 1;
    }

    return 1; // finished reading, return 1 or games will hang
}
bool GBAMemory::addressesEepromChip(uint32_t address) {
    uint8_t addressSection = address >> 24;
    switch(addressSection) {
        case 0x09:
            return address >= 0x9FFFF00;
        case 0x0D:
            if(largerThan16KB)
                return address >= 0xDFFFF00;
            else
                return address >= 0xD000000;
        default:
            return false;
    }
}
void GBAMemory::resetSerialBuffer() {
    serialBuffer = 0;
    transmittedBits = 0;
}