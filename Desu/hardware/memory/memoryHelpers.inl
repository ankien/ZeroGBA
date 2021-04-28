template<typename T>
inline T& GBAMemory::memoryArray(uint32_t i) {
    switch(i >> 24) {
        case 0x00:
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
            return *reinterpret_cast<T*>(&gPakSram[i-0xE000000]);
        default:
            return *reinterpret_cast<T*>(&unusedMemoryAccess);
            break;
    }
}
// return bitmask to write with, also handles special write behavior for certain regions (like IF regs)
template<typename T>
inline uint32_t GBAMemory::writeable(uint32_t address, T value) {
    // todo: add SIO regs
    if(address >> 24 == 0x04) {

        // todo: check if I also need to check for interrupts when respective hardware register IRQ bits are written to
        constexpr uint8_t offset = sizeof(T) - 1;
        if(address >= 0x4000200-offset && address < 0x4000202 || address >= 0x4000208-offset && address <= 0x4000208) { // respective bit range for IO regs
            interrupts->scheduleInterruptCheck();
            return 0xFFFFFFFF;
        }

        // if value overlaps with IF
        if(address >= 0x4000202-offset && address <= 0x4000203) {
            uint16_t oldIF = memoryArray<uint16_t>(0x4000202);
            memoryArray<T>(address) = value;
            memoryArray<uint16_t>(0x4000202) = oldIF & ~memoryArray<uint16_t>(0x4000202);
            return 0x0;
        }

        switch(address) {
            case 0x4000004:
                return 0xFE00FFB8;
            case 0x4000006:
                return 0xFFFFFE00;
            case 0x4000007:
                return 0xFFFFFFFE;
            case 0x4000084:
                return 0xFFFFFFF0;
            case 0x4000130:
                return 0xFFFFFC00;
            case 0x4000131:
                return 0xFFFFFFFC;
        }
    }
    return 0xFFFFFFFF;
}
inline void GBAMemory::storeValue(uint8_t value, uint32_t address) {
    switch(address >> 24) {
        // todo: fix this shit these reinterpret casts look unsafe
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
            uint8_t mask = writeable<uint8_t>(address,value);
            uint8_t* mem = &memoryArray<uint8_t>(address);
            mem[0] = (value & mask) | (mem[0] & ~mask);
        }
    }
}
inline void GBAMemory::storeValue(uint16_t value, uint32_t address) {
    address = address & ~1;
    uint16_t mask = writeable<uint16_t>(address,value);
    uint16_t* mem = &memoryArray<uint16_t>(address);
    mem[0] = (value & mask) | (mem[0] & ~mask);
}
inline void GBAMemory::storeValue(uint32_t value, uint32_t address) {
    address = address & ~3;
    uint32_t mask = writeable<uint32_t>(address,value);
    uint32_t* mem = &memoryArray<uint32_t>(address);
    mem[0] = (value & mask) | (mem[0] & ~mask);
}
inline uint16_t GBAMemory::readHalfWord(uint32_t address) {
    return memoryArray<uint16_t>(address & ~1);
}
inline uint8_t GBAMemory::readByte(uint32_t address) {
    return memoryArray<uint8_t>(address);
}
inline uint32_t GBAMemory::readHalfWordRotate(uint32_t address) {
    uint8_t rorAmount = (address & 1) << 3;
    return ror(readHalfWord(address),rorAmount);
}
inline uint32_t GBAMemory::readWord(uint32_t address) {
    return memoryArray<uint32_t>(address & ~3);
}
inline uint32_t GBAMemory::readWordRotate(uint32_t address) {
    uint8_t rorAmount = (address & 3) << 3;
    return ror(readWord(address),rorAmount);
}

inline uint32_t GBAMemory::ror(uint32_t value, uint8_t shiftAmount) {
    shiftAmount &= 0x1F;
    return (value >> shiftAmount) | (value << (32 - shiftAmount));
}