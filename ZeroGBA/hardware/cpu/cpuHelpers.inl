#pragma once

// Bits 27-20 + 7-4
inline uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}
// Bits 15-6
inline uint16_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 6;
}

inline bool ARM7TDMI::checkCond(uint32_t cond) {
    switch(cond) {
        case 0:
            return cpuState.zeroFlag;
        case 1:
            return !cpuState.zeroFlag;
        case 2:
            return cpuState.carryFlag;
        case 3:
            return !cpuState.carryFlag;
        case 4:
            return cpuState.signFlag;
        case 5:
            return !cpuState.signFlag;
        case 6:
            return cpuState.overflowFlag;
        case 7:
            return !cpuState.overflowFlag;
        case 8:
            return cpuState.carryFlag && (!cpuState.zeroFlag);
        case 9:
            return(!cpuState.carryFlag) || cpuState.zeroFlag;
        case 0xA:
            return cpuState.signFlag == cpuState.overflowFlag;
        case 0xB:
            return cpuState.signFlag != cpuState.overflowFlag;
        case 0xC:
            return (!cpuState.zeroFlag) && (cpuState.signFlag == cpuState.overflowFlag);
        case 0xD:
            return cpuState.zeroFlag || (cpuState.signFlag != cpuState.overflowFlag);
        case 0xE:
            return 1;
    }
    return 0;
}

inline uint32_t ARM7TDMI::ALUshift(uint32_t value, uint8_t shiftAmount, uint8_t shiftType, bool setFlags, bool registerShiftByImmediate) {
    
    switch(shiftType) {
        case 0b00: // lsl
            if(shiftAmount == 0)
                return value;
            if(!registerShiftByImmediate) {
                if(shiftAmount == 32) {
                    cpuState.carryFlag = 1 & value;
                    return 0;
                }
                if(shiftAmount > 32) {
                    cpuState.carryFlag = 0;
                    return cpuState.carryFlag;
                }
            }
            value <<= shiftAmount - 1;
            if(setFlags)
                cpuState.carryFlag = 0x80000000 & value;
            return value << 1; 
        case 0b01: // lsr
            if(registerShiftByImmediate) {
                if(shiftAmount == 0)
                    shiftAmount = 32;
            } else {
                if(shiftAmount == 0)
                    return value;
                if(shiftAmount == 32) {
                    cpuState.carryFlag = 0x80000000 & value;
                    return 0;
                }
                if(shiftAmount > 32) {
                    cpuState.carryFlag = 0;
                    return cpuState.carryFlag;
                }
            }
            value >>= shiftAmount - 1;
            if(setFlags)
                cpuState.carryFlag = 1 & value;
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
                cpuState.carryFlag = 0x80000000 & value;
                return static_cast<int32_t>(value) >> 31;
            }
            int32_t result = static_cast<int32_t>(value) >> (shiftAmount - 1);
            if(setFlags)
                cpuState.carryFlag = 1 & result;
            return result >> 1;
        }
        case 0b11: // ror
        {
            if(registerShiftByImmediate) {
                if(shiftAmount == 0) {
                    bool oldCarry = cpuState.carryFlag;
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
                cpuState.carryFlag = value & 0x80000000;
            return value;
        }
    }
}
inline uint32_t ARM7TDMI::sub(uint32_t op1, uint32_t op2, bool setFlags) {
    uint32_t result = op1 - op2;
    if(setFlags) {
        cpuState.carryFlag = op1 >= op2;
        op1 >>= 31; op2 >>= 31;
        cpuState.overflowFlag = (op1 ^ op2) ? (result >> 31) ^ op1 : 0; // if rn and op2 bits 31 are diff, check for overflow
    }
    return result;
}
inline uint32_t ARM7TDMI::add(uint32_t op1, uint32_t op2, bool setFlags) {
    uint32_t result = op1 + op2;
    if(setFlags) {
        cpuState.carryFlag = result < op1;
        op1 >>= 31; op2 >>= 31;
        cpuState.overflowFlag = (op1 ^ op2) ? 0 : (result >> 31) ^ op1;
    }
    return result;
}
inline uint32_t ARM7TDMI::addCarry(uint32_t op1, uint32_t op2, bool setFlags, bool oldCarry){
    uint32_t result = op1 + op2 + oldCarry;
    if(setFlags) {
        cpuState.carryFlag = result < (static_cast<uint64_t>(op1) + oldCarry);
        op1 >>= 31; op2 >>= 31;
        cpuState.overflowFlag = (op1 ^ op2) ? 0 : (result >> 31) ^ op1;
    }
    return result;
}
inline void ARM7TDMI::setZeroAndSign(uint32_t arg) {
    (arg == 0) ?  cpuState.zeroFlag = 1 : cpuState.zeroFlag = 0;
    (arg & 0x80000000) ? cpuState.signFlag = 1 : cpuState.signFlag = 0;
}
inline void ARM7TDMI::setZeroAndSign(uint64_t arg) {
    (arg == 0) ?  cpuState.zeroFlag = 1 : cpuState.zeroFlag = 0;
    (arg & 0x8000000000000000) ? cpuState.signFlag = 1 : cpuState.signFlag = 0;
}