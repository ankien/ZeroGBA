
inline void ARM7TDMI::handleException(uint8_t exception, int8_t nn, uint8_t newMode) {
    setBankedReg(newMode,1,r[15]+nn); // save old PC
    setBankedReg(newMode,'S',getCPSR()); // save old CPSR
    switchMode(newMode); // switch mode
    // new bits!
    mode = newMode;
    state = 0;
    irqDisable = 1;
    
    if((newMode == Reset) || (newMode == FIQ))
        fiqDisable = 1;

    switch(newMode) {

        case Supervisor:

            switch(exception) {

                case Reset:
                    r[15] = 0x0;
                    break;
                case AddressExceeds26Bit:
                    r[15] = 0x14;
                    break;
                case SoftwareInterrupt:
                    r[15] = 0x8;
                    break;
            }
            break;

        case Undefined:
            
            switch(exception) {
                
                case UndefinedInstruction:
                    r[15] = 0x4;
                    break;
            }
            break;

        case Abort:

            switch(exception) {

                case DataAbort:
                    r[15] = 0x10;
                    break;
                case PrefetchAbort:
                    r[15] = 0xC;
                    break;
            }
            break;

        case IRQ:
            
            switch(exception) {

                case NormalInterrupt:
                    r[15] = 0x18;
                    break;
            }
            break;

        case FIQ:
            
            switch(exception) {

                case FastInterrupt:
                    r[15] = 0x1C;
                    break;
            }
    }
}
// Bits 27-20 + 7-4
inline uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}
// Bits 15-8
inline uint8_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 8;
}

template<typename T>
inline T& ARM7TDMI::memoryArray(uint32_t i) {
    switch(i >> 24) {
        case 0x00:
            return *reinterpret_cast<T*>(&systemMemory->bios[i]);
        case 0x02:
            return *reinterpret_cast<T*>(&systemMemory->wramOnBoard[(i-0x2000000) % 0x40000]);
        case 0x03:
            return *reinterpret_cast<T*>(&systemMemory->wramOnChip[(i-0x3000000) % 0x8000]);
        case 0x04:
            return *reinterpret_cast<T*>(&systemMemory->IORegisters[i-0x4000000]);
        case 0x05:
            return *reinterpret_cast<T*>(&systemMemory->pram[(i-0x5000000) % 0x400]);
        case 0x06:
            i = (i-0x6000000) % 0x20000;
            if(i > 0x17FFF)
                i -= 0x8000;
            return *reinterpret_cast<T*>(&systemMemory->vram[i]);
        case 0x07:
            return *reinterpret_cast<T*>(&systemMemory->oam[(i-0x7000000) % 0x400]);
        case 0x08:
        case 0x09:
            return *reinterpret_cast<T*>(&systemMemory->gamePak[i-0x8000000]);
        case 0x0A:
        case 0x0B:
            return *reinterpret_cast<T*>(&systemMemory->gamePak[i-0xA000000]);
        case 0x0C:
        case 0x0D:
            return *reinterpret_cast<T*>(&systemMemory->gamePak[i-0xC000000]);
        case 0x0E:
            return *reinterpret_cast<T*>(&systemMemory->gPakSram[i-0xE000000]);
        default:
            return *reinterpret_cast<T*>(&systemMemory->unusedMemoryAccess);
            break;
    }
}
// return bitmask to write with, also handles special write behavior for certain regions (like IF regs)
template<typename T>
inline uint32_t ARM7TDMI::writeable(uint32_t address, T value) {
    // todo: add SIO regs
    if(address >> 24 == 0x04) {

        // todo: check if I also need to check for interrupts when respective hardware register IRQ bits are written to
        constexpr uint8_t offset = sizeof(T) - 1;
        if(address >= 0x4000200-offset && address < 0x4000202 || address >= 0x4000208-offset && address <= 0x4000208) { // respective bit range for IO regs
            scheduleInterruptCheck();
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
inline void ARM7TDMI::storeValue(uint8_t value, uint32_t address) {
    switch(address >> 24) {
        // todo: fix this shit these reinterpret casts look unsafe
        case 0x05:
            storeValue(static_cast<uint16_t>(static_cast<uint16_t>(value) * 0x101), address);
            break;
        case 0x06:
            if(systemMemory->IORegisters[0] < 3) { // bitmap mode writes
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
inline void ARM7TDMI::storeValue(uint16_t value, uint32_t address) {
    address = address & ~1;
    uint16_t mask = writeable<uint16_t>(address,value);
    uint16_t* mem = &memoryArray<uint16_t>(address);
    mem[0] = (value & mask) | (mem[0] & ~mask);
}
inline void ARM7TDMI::storeValue(uint32_t value, uint32_t address) {
    address = address & ~3;
    uint32_t mask = writeable<uint32_t>(address,value);
    uint32_t* mem = &memoryArray<uint32_t>(address);
    mem[0] = (value & mask) | (mem[0] & ~mask);
}
inline uint16_t ARM7TDMI::readHalfWord(uint32_t address) {
    return memoryArray<uint16_t>(address & ~1);
}
inline uint8_t ARM7TDMI::readByte(uint32_t address) {
    return memoryArray<uint8_t>(address);
}
inline uint32_t ARM7TDMI::readHalfWordRotate(uint32_t address) {
    uint8_t rorAmount = (address & 1) << 3;
    return ror(readHalfWord(address),rorAmount);
}
inline uint32_t ARM7TDMI::readWord(uint32_t address) {
    return memoryArray<uint32_t>(address & ~3);
}
inline uint32_t ARM7TDMI::readWordRotate(uint32_t address) {
    uint8_t rorAmount = (address & 3) << 3;
    return ror(readWord(address),rorAmount);
}

inline void ARM7TDMI::switchMode(uint8_t newMode) {
    // bank current regs
    switch(mode) {
        case User:
        case System:
            for(uint8_t i = 8; i < 15; i++)
                sysUserReg[i-8] = r[i];
            break;
        case FIQ:
            for(uint8_t i = 8; i < 15; i++)
                fiqReg[i-8] = r[i];
            fiqReg[7] = getCPSR();
            break;
        case Supervisor:
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                svcReg[i-13] = r[i];
            svcReg[2] = getCPSR();
            break;
        case Abort:
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                abtReg[i-13] = r[i];
            abtReg[2] = getCPSR();
            break;
        case IRQ:
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                irqReg[i-13] = r[i];
            irqReg[2] = getCPSR();
            break;
        case Undefined:
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                undReg[i-13] = r[i];
            undReg[2] = getCPSR();
    }

    mode = newMode;

    // get new regs
    switch(newMode) {
        case User:
        case System:
            for(uint8_t i = 8; i < 15; i++)
                r[i] = sysUserReg[i-8];
            break;
        case FIQ:
            for(uint8_t i = 8; i < 15; i++)
                r[i] = fiqReg[i-8];
            setCPSR(fiqReg[7]);
            break;
        case Supervisor:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = svcReg[i-13];
            setCPSR(svcReg[2]);
            break;
        case Abort:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = abtReg[i-13];
            setCPSR(abtReg[2]);
            break;
        case IRQ:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = irqReg[i-13];
            setCPSR(irqReg[2]);
            break;
        case Undefined:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = undReg[i-13];
            setCPSR(undReg[2]);
            break;
    }
}
inline uint32_t ARM7TDMI::getBankedReg(uint8_t mode, uint8_t reg) {
    if(reg == 'S')
        mode == FIQ ? reg = 7 : reg = 2;
    switch(mode) {
        case User:
        case System:
            return sysUserReg[reg];
            break;
        case FIQ:
            return fiqReg[reg];
            break;
        case Supervisor:
            return svcReg[reg];
            break;
        case Abort:
            return abtReg[reg];
            break;
        case IRQ:
            return irqReg[reg];
            break;
        case Undefined:
            return undReg[reg];
    }
}
inline void ARM7TDMI::setBankedReg(uint8_t mode, uint8_t reg, uint32_t arg) {
    if(reg == 'S')
        mode == FIQ ? reg = 7 : reg = 2;
    switch(mode) {
        case User:
        case System:
            sysUserReg[reg] = arg;
            break;
        case FIQ:
            fiqReg[reg] = arg;
            break;
        case Supervisor:
            svcReg[reg] = arg;
            break;
        case Abort:
            abtReg[reg] = arg;
            break;
        case IRQ:
            irqReg[reg] = arg;
            break;
        case Undefined:
            undReg[reg] = arg;
    }
}
inline uint32_t ARM7TDMI::getReg(uint8_t reg) {
    return r[reg];
}
inline void ARM7TDMI::setReg(uint8_t reg, uint32_t arg) {
    r[reg] = arg;
}
inline bool ARM7TDMI::checkCond(uint32_t cond) {
    switch(cond) {
        case 0x00000000:
            return zeroFlag;
        case 0x10000000:
            return !zeroFlag;
        case 0x20000000:
            return carryFlag;
        case 0x30000000:
            return !carryFlag;
        case 0x40000000:
            return signFlag;
        case 0x50000000:
            return !signFlag;
        case 0x60000000:
            return overflowFlag;
        case 0x70000000:
            return !overflowFlag;
        case 0x80000000:
            return carryFlag && (!zeroFlag);
        case 0x90000000:
            return(!carryFlag) || zeroFlag;
        case 0xA0000000:
            return signFlag == overflowFlag;
        case 0xB0000000:
            return signFlag != overflowFlag;
        case 0xC0000000:
            return (!zeroFlag) && (signFlag == overflowFlag);
        case 0xD0000000:
            return zeroFlag || (signFlag != overflowFlag);
        case 0xE0000000:
            return 1;
    }
    return 0;
}
inline uint32_t ARM7TDMI::getSPSR(uint8_t mode) {
    switch(mode) {
        case FIQ:
            return fiqReg[7];
        case Supervisor:
            return svcReg[2];
        case Abort:
            return abtReg[2];
        case IRQ:
            return irqReg[2];
        case Undefined:
            return undReg[2];
    }
}
inline void ARM7TDMI::setSPSR(uint8_t mode,uint32_t arg) {
    switch(mode) {
        case FIQ:
            fiqReg[7] = arg;
            break;
        case Supervisor:
            svcReg[2] = arg;
            break;
        case Abort:
            abtReg[2] = arg;
            break;
        case IRQ:
            irqReg[2] = arg;
            break;
        case Undefined:
            undReg[2] = arg;
            break;
    }
}
inline uint32_t ARM7TDMI::getCPSR() {
    return mode |
           (state << 5) |
           (fiqDisable << 6) |
           (irqDisable << 7) |
           (reserved << 8) |
           (stickyOverflow << 27) |
           (overflowFlag << 28) |
           (carryFlag << 29) |
           (zeroFlag << 30) |
           (signFlag << 31);
}
inline void ARM7TDMI::setCPSR(uint32_t num) {
    mode = (num & 0x1F);
    state = (num & 0x20) >> 5;
    fiqDisable = (num & 0x40) >> 6;
    irqDisable = (num & 0x80) >> 7;
    reserved = (num & 0x7FFFF00) >> 8;
    stickyOverflow = (num & 0x8000000) >> 27;
    overflowFlag = (num & 0x10000000) >> 28;
    carryFlag = (num & 0x20000000) >> 29;
    zeroFlag = (num & 0x40000000) >> 30;
    signFlag = (num & 0x80000000) >> 31;
}

inline uint32_t ARM7TDMI::ror(uint32_t value, uint8_t shiftAmount) {
    shiftAmount &= 0x1F;
    return (value >> shiftAmount) | (value << (32 - shiftAmount));
}
inline uint32_t ARM7TDMI::ALUshift(uint32_t value, uint8_t shiftAmount, uint8_t shiftType, bool setFlags, bool registerShiftByImmediate) {
    
    switch(shiftType) {
        case 0b00: // lsl
            if(shiftAmount == 0)
                return value;
            if(!registerShiftByImmediate) {
                if(shiftAmount == 32) {
                    carryFlag = 1 & value;
                    return 0;
                }
                if(shiftAmount > 32) {
                    carryFlag = 0;
                    return carryFlag;
                }
            }
            value <<= shiftAmount - 1;
            if(setFlags)
                carryFlag = 0x80000000 & value;
            return value << 1; 
        case 0b01: // lsr
            if(registerShiftByImmediate) {
                if(shiftAmount == 0)
                    shiftAmount = 32;
            } else {
                if(shiftAmount == 0)
                    return value;
                if(shiftAmount == 32) {
                    carryFlag = 0x80000000 & value;
                    return 0;
                }
                if(shiftAmount > 32) {
                    carryFlag = 0;
                    return carryFlag;
                }
            }
            value >>= shiftAmount - 1;
            if(setFlags)
                carryFlag = 1 & value;
            return value >> 1;
        case 0b10: // asr
        {
            if(registerShiftByImmediate) {
                if(shiftAmount == 0)
                    shiftAmount = 32;
            } else {
                if(shiftAmount == 0)
                    return value;
            }
            if(shiftAmount >= 32) {
                carryFlag = 0x80000000 & value;
                return static_cast<int32_t>(value) >> 31;
            }
            int32_t result = static_cast<int32_t>(value) >> (shiftAmount - 1);
            if(setFlags)
                carryFlag = 1 & result;
            return result >> 1;
        }
        case 0b11: // ror
        {
            if(registerShiftByImmediate) {
                if(shiftAmount == 0) {
                    bool oldCarry = carryFlag;
                    uint32_t result = ALUshift(value,1,1,setFlags,registerShiftByImmediate);
                    return oldCarry ? 0x80000000 | result : result;
                }
            } else {
                if(shiftAmount == 0)
                    return value;
            }
            shiftAmount &= 0x1F;
            value = (value >> shiftAmount) | (value << (32 - shiftAmount));
            if(setFlags)
                carryFlag = value & 0x80000000;
            return value;
        }
    }
}
inline uint32_t ARM7TDMI::sub(uint32_t op1, uint32_t op2, bool setFlags) {
    uint32_t result = op1 - op2;
    if(setFlags) {
        carryFlag = op1 >= op2;
        op1 >>= 31; op2 >>= 31;
        overflowFlag = (op1 ^ op2) ? (result >> 31) ^ op1 : 0; // if rn and op2 bits 31 are diff, check for overflow
    }
    return result;
}
inline uint32_t ARM7TDMI::add(uint32_t op1, uint32_t op2, bool setFlags) {
    uint32_t result = op1 + op2;
    if(setFlags) {
        carryFlag = result < op1;
        op1 >>= 31; op2 >>= 31;
        overflowFlag = (op1 ^ op2) ? 0 : (result >> 31) ^ op1;
    }
    return result;
}
inline uint32_t ARM7TDMI::addCarry(uint32_t op1, uint32_t op2, bool setFlags, bool oldCarry){
    uint32_t result = op1 + op2 + oldCarry;
    if(setFlags) {
        carryFlag = result < (static_cast<uint64_t>(op1) + oldCarry);
        op1 >>= 31; op2 >>= 31;
        overflowFlag = (op1 ^ op2) ? 0 : (result >> 31) ^ op1; // todo: check if overflow calc for carry opcodes are correct
    }
    return result;
}
inline void ARM7TDMI::setZeroAndSign(uint32_t arg) {
    (arg == 0) ?  zeroFlag = 1 : zeroFlag = 0;
    (arg & 0x80000000) ? signFlag = 1 : signFlag = 0;
}
inline void ARM7TDMI::setZeroAndSign(uint64_t arg) {
    (arg == 0) ?  zeroFlag = 1 : zeroFlag = 0;
    (arg & 0x8000000000000000) ? signFlag = 1 : signFlag = 0;
}

inline void ARM7TDMI::scheduleInterruptCheck() {
    scheduler->scheduleInterruptCheck([&]() {
        if((*reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x200]) & 0x3FFF) &
           (*reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x202]) & 0x3FFF))
            if(irqsEnabled())
                handleException(NormalInterrupt,4,IRQ);
        return 0;
    },0);
}
inline bool ARM7TDMI::irqsEnabled() {
    if(IME)
        return !irqDisable;
    return false;
}