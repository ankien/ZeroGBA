#pragma once

template<typename T>
inline T& GBAMemory::memoryArray(uint32_t i) {
    switch(i >> 24) {
        case 0x00:
            if(i > 0x3FFF)
                return *reinterpret_cast<T*>(&ignore);
            return *reinterpret_cast<T*>(&bios[i]);
        case 0x02:
            return *reinterpret_cast<T*>(&wramOnBoard[(i-0x2000000) % 0x40000]);
        case 0x03:
            return *reinterpret_cast<T*>(&wramOnChip[(i-0x3000000) % 0x8000]);
        case 0x04:
            return *reinterpret_cast<T*>(&IORegisters[i-0x4000000]);
        case 0x05:
            return *reinterpret_cast<T*>(&pram[(i-0x5000000) % 0x400]);
        case 0x06:
            i = (i-0x6000000) % 0x20000;
            if(i > 0x17FFF)
                i -= 0x8000;
            return *reinterpret_cast<T*>(&vram[i]);
        case 0x07:
            return *reinterpret_cast<T*>(&oam[(i-0x7000000) % 0x400]);
        case 0x08:
        case 0x09:
            return *reinterpret_cast<T*>(&gamePak[i-0x8000000]);
        case 0x0A:
        case 0x0B:
            return *reinterpret_cast<T*>(&gamePak[i-0xA000000]);
        case 0x0C:
        case 0x0D:
            return *reinterpret_cast<T*>(&gamePak[i-0xC000000]);
        case 0x0E:
        case 0x0F:
            switch(romSaveType) {
                case SRAM_V:
                    return *reinterpret_cast<T*>(&gPakSaveMem[((i - 0xE000000) % 0x10000) % 0x8000]);
                case FLASH_V:
                case FLASH1M_V:
                    return *reinterpret_cast<T*>(&gPakSaveMem[(i - 0xE000000) % 0x10000 + (secondFlashBank * 0x10000)]);
                case EEPROM_V:
                    return *reinterpret_cast<T*>(&ignore);
            }
        default:
            return *reinterpret_cast<T*>(&ignore); // ignore writes to misc addresses, reads are handled
    }
}

template <uint16_t timing>
inline void GBAMemory::delayedDma() {
    for(uint8_t channel = 0; channel < 4; channel++) {
        uint16_t dmaCntH = memoryArray<uint16_t>(0x40000BA + 12*channel);
        if(dmaCntH & 0x8000 && (dmaCntH & 0x3000) == timing)
            dmaTransfer(channel,dmaCntH);
    }
}

inline void GBAMemory::soundDma(uint8_t timerId) {
    uint8_t dmaChannel = timerId + 1;
    uint16_t dmaCntH = memoryArray<uint16_t>(0x40000BA + 12 * dmaChannel); // Use DMA channels 1 & 2 for supplying timer overflow FIFO
    if(dmaCntH & 0x8000 && (dmaCntH & 0x3000) == 0x3000)
        dmaTransfer(dmaChannel, dmaCntH);
}