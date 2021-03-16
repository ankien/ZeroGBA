#pragma once
#include <cstdint>
#include <iostream>
#include "../hardware/GBAMemory.hpp"
#include "../Scheduler.hpp"
#include "../hardware/MMIO.h"

struct ARM7TDMI {
    // cycles per instruction
    static const int32_t cycleTicks = 10; // stubbing this for now, todo: implement correct cycle timings

    // lookup tables, array size is the different number of instructions
    // todo: use templates to generate these tables, and their constexpr arguments at compile time
    void (ARM7TDMI::*armTable[4096])(uint32_t);
    void (ARM7TDMI::*thumbTable[256])(uint16_t);

    // could make this a generic pointer for code reuse with another system (the DS has an ARM7TDMI)
    GBAMemory* systemMemory;

    enum exceptions { Reset, UndefinedInstruction, SoftwareInterrupt, PrefetchAbort, DataAbort,
                      AddressExceeds26Bit, NormalInterrupt, FastInterrupt };

    enum modes { User = 16, FIQ, IRQ, Supervisor, Abort = 23, Undefined = 27, System = 31 };

    /// Registers ///
    // CPSR & SPSR = program status registers
    // current regs
    uint32_t r[16]; // R0-15; r13 - SP, r14 - LR, r15 - PC

    // banked regs
    uint32_t sysUserReg[7]; // r8-14
    uint32_t fiqReg[8]; // r8-14 + SPSR
    uint32_t svcReg[3]; // r13-14 + SPSR
    uint32_t abtReg[3]; // r13-14 + SPSR
    uint32_t irqReg[3]; // r13-14 + SPSR
    uint32_t undReg[3]; // r13-14 + SPSR
    
    // CPSR bitfield implementation, least significant bit to most
    uint8_t mode; // 5 : see enum modes
    bool    state, // 1 : 0 = ARM, 1 = THUMB
            fiqDisable, // 1 : 0 = enable, 1 = disable
            irqDisable; // 1 : 0 = enable, 1 = disable
    uint32_t reserved; // 19 : never used?
    bool stickyOverflow, // 1 : 1 = sticky overflow, ARMv5TE and up only
         overflowFlag, // V, 1 : 0 = no overflow, 1 = signed overflow (if the result register is a negative 2's complement, set bit) 
         carryFlag, // C, 1 : 0 = borrow/no carry, 1 = carry/no borrow, addition overflow is carry; sub underflow is borrow
         zeroFlag, // Z, 1 : 0 = not zero, 1 = zero
         signFlag; // N, 1 : 0 = not signed, 1 = signed (31 bit is filled)
    
    // Used to fill LUTs
    void fillARM();
    void fillTHUMB();
    
    // ARM/THUMB helper functions
    void handleException(uint8_t, uint32_t, uint8_t);
    uint16_t fetchARMIndex(uint32_t);
    uint8_t fetchTHUMBIndex(uint16_t);

    // memory helper functions
    bool writeable(uint32_t);
    uint32_t readable(uint32_t); // returns a value from defined behavior if nonreadable/unused memory, else 1 if readable
    void storeValue(uint8_t, uint32_t);
    void storeValue(uint16_t, uint32_t);
    void storeValue(uint32_t, uint32_t);
    // memory getters, rotates are for misaligned ldr+swp
    uint8_t readByte(uint32_t);
    uint16_t readHalfWord(uint32_t);
    uint32_t readHalfWordRotate(uint32_t);
    uint32_t readWord(uint32_t);
    uint32_t readWordRotate(uint32_t);

    // register helpers
    void switchMode(uint8_t);
    uint32_t getBankedReg(uint8_t, uint8_t);
    void setBankedReg(uint8_t, uint8_t, uint32_t);
    uint32_t getReg(uint8_t);
    void setReg(uint8_t, uint32_t);
    uint32_t getSPSR(uint8_t);
    void setSPSR(uint8_t,uint32_t);
    uint32_t getCPSR();
    void setCPSR(uint32_t);
    bool checkCond(uint32_t);
    // ALU Helpers: shift for registers, ALUshift affects carry flag
    uint32_t ror(uint32_t, uint8_t);
    uint32_t ALUshift(uint32_t, uint8_t, uint8_t,bool,bool);
    uint32_t sub(uint32_t,uint32_t,bool);
    uint32_t add(uint32_t,uint32_t,bool);
    uint32_t addCarry(uint32_t,uint32_t,bool,bool);
    void setZeroAndSign(uint32_t);
    void setZeroAndSign(uint64_t);

    // For unimplemented/undefined instructions
    void ARMundefinedInstruction(uint32_t);
    void ARMemptyInstruction(uint32_t);
    void THUMBemptyInstruction(uint16_t);

    /// ARM Instructions ///
    // todo: implement a templated instruction handler
    void ARMbranch(uint32_t);
    void ARMbranchExchange(uint32_t);
    void ARMsoftwareInterrupt(uint32_t);

    void ARMdataProcessing(uint32_t);
    void ARMmultiplyAndMultiplyAccumulate(uint32_t);

    void ARMpsrTransfer(uint32_t);

    void ARMsingleDataTransfer(uint32_t);
    void ARMhdsDataSTRH(uint32_t);
    void ARMhdsDataLDRH(uint32_t);
    void ARMhdsDataLDRSB(uint32_t);
    void ARMhdsDataLDRSH(uint32_t);
    // todo: optimize this instruction with a straight memcpy
    void ARMblockDataTransfer(uint32_t);
    void ARMswap(uint32_t);

    /// THUMB Instructions ///
    void THUMBmoveShiftedRegister(uint16_t);
    void THUMBaddSubtract(uint16_t);
    void THUMBmoveCompareAddSubtract(uint16_t);
    void THUMBaluOperations(uint16_t);
    void THUMBhiRegOpsBranchEx(uint16_t);

    void THUMBloadPCRelative(uint16_t);
    void THUMBloadStoreRegOffset(uint16_t);
    void THUMBloadStoreSignExtendedByteHalfword(uint16_t);
    void THUMBloadStoreImmOffset(uint16_t);
    void THUMBloadStoreHalfword(uint16_t);
    void THUMBloadStoreSPRelative(uint16_t);

    void THUMBgetRelativeAddress(uint16_t);
    void THUMBaddOffsetToSP(uint16_t);

    void THUMBpushPopRegisters(uint16_t);
    void THUMBmultipleLoadStore(uint16_t);

    void THUMBconditionalBranch(uint16_t);
    void THUMBunconditionalBranch(uint16_t);
    void THUMBlongBranchWithLink(uint16_t);
    void THUMBsoftwareInterrupt(uint16_t);

    /// Scheduler Stuff ///
    bool irqsEnabled();
};

/// Helper Methods ///

inline void ARM7TDMI::handleException(uint8_t exception, uint32_t nn, uint8_t newMode) {
    setBankedReg(newMode,1,r[15]+nn); // save old PC
    setBankedReg(newMode,'S',getCPSR()); // save old CPSR
    uint8_t oldMode = mode;
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

    r[15]+=nn;
}
// Bits 27-20 + 7-4
inline uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}
// Bits 15-8
inline uint8_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 8;
}

inline bool ARM7TDMI::writeable(uint32_t address) {
    switch(address) {
        case 0x4000006:
        case 0x4000007:
        case 0x4000130:
        case 0x4000131:
            return false;
    }
    return true;
}
inline uint32_t ARM7TDMI::readable(uint32_t address) {
    /*
    if((address >> 11) <= 3)
        return 1; // todo: implement unpermitted bios reads
    else {
        switch(address >> 24) {
            case 0x00:
            case 0x01:
                if(address > 0x4000)
                    return ;
            case 0x02:
            case 0x03:
                
            case 0x04:
                
            default:
                
        }
    }
    */
    return 1;
}
inline void ARM7TDMI::storeValue(uint8_t value, uint32_t address) {
    if(writeable(address)) {
        switch(address >> 24) {
            case 0x05:
                storeValue(static_cast<uint16_t>(*reinterpret_cast<uint16_t*>(&value) * 0x101), address);
                break;
            case 0x06:
                if(systemMemory->IORegisters[0] < 3) { // bitmap mode writes
                    if(address < 0x6014000)
                        storeValue(static_cast<uint16_t>(*reinterpret_cast<uint16_t*>(&value) * 0x101), address);
                } else {
                    if(address < 0x6010000)
                        storeValue(static_cast<uint16_t>(*reinterpret_cast<uint16_t*>(&value) * 0x101), address);
                }
                return;
            case 0x07:
                return;
            default:
                (*systemMemory)[address] = value;
        }
    }
}
inline void ARM7TDMI::storeValue(uint16_t value, uint32_t address) {
    if(writeable(address)) {
        reinterpret_cast<uint16_t*>(&(*systemMemory)[address & ~1])[0] = value;
    }
}
inline void ARM7TDMI::storeValue(uint32_t value, uint32_t address) {
    if(writeable(address)) {
        reinterpret_cast<uint32_t*>(&(*systemMemory)[address & ~3])[0] = value;
    }
}
inline uint16_t ARM7TDMI::readHalfWord(uint32_t address) {
    uint16_t readValue = readable(address);
    if(readValue == 1) {
        return *reinterpret_cast<uint16_t*>(&(*systemMemory)[address & ~1]);
    }
    return readValue;
}
inline uint8_t ARM7TDMI::readByte(uint32_t address) {
    uint8_t readValue = readable(address);
    if(readable(address) == 1)
        return (*systemMemory)[address];
}
inline uint32_t ARM7TDMI::readHalfWordRotate(uint32_t address) {
    uint8_t rorAmount = (address & 1) << 3;
    return ror(readHalfWord(address),rorAmount);
}
inline uint32_t ARM7TDMI::readWord(uint32_t address) {
    return *reinterpret_cast<uint32_t*>(&(*systemMemory)[address & ~3]);
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

inline bool ARM7TDMI::irqsEnabled() {
    if(IME)
        return !irqDisable;
}