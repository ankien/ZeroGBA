inline void ARM7TDMI::fillARM() {
    for(uint16_t i = 0; i < 4096; i++) {
        // harder to distinguish opcodes at the bottom
        // no coprocessor instructions on GBA
        if((i & 0b111000000000) == 0b101000000000)
            armTable[i] = &ARM7TDMI::ARMbranch;
        else if((i & 0b111000000000) == 0b100000000000)
            armTable[i] = &ARM7TDMI::ARMblockDataTransfer;
        else if((i & 0b111111111111) == 0b000100100001)
            armTable[i] = &ARM7TDMI::ARMbranchExchange;
        else if((i & 0b111100000000) == 0b111100000000)
            armTable[i] = &ARM7TDMI::ARMsoftwareInterrupt;
        else if((i & 0b111000000001) == 0b011000000001)
            armTable[i] = &ARM7TDMI::ARMundefinedInstruction; // undefined opcode
        else if((i & 0b110000000000) == 0b010000000000)
            armTable[i] = &ARM7TDMI::ARMsingleDataTransfer;
        else if((i & 0b110110010000) == 0b000100000000)
            armTable[i] = &ARM7TDMI::ARMpsrTransfer;
        else if((i & 0b111100001111) == 0b000000001001)
            armTable[i] = &ARM7TDMI::ARMmultiplyAndMultiplyAccumulate;
        else if((i & 0b111110111111) == 0b000100001001)
            armTable[i] = &ARM7TDMI::ARMswap;
        else if((i & 0b111000011111) == 0b000000001011)
            armTable[i] = &ARM7TDMI::ARMhdsDataSTRH;
        else if((i & 0b111000011111) == 0b000000011011)
            armTable[i] = &ARM7TDMI::ARMhdsDataLDRH;
        else if((i & 0b111000011111) == 0b000000011101)
            armTable[i] = &ARM7TDMI::ARMhdsDataLDRSB;
        else if((i & 0b111000011111) == 0b000000011111)
            armTable[i] = &ARM7TDMI::ARMhdsDataLDRSH;
        else if((i & 0b110000000000) == 0b000000000000)
            armTable[i] = &ARM7TDMI::ARMdataProcessing;
        else
            armTable[i] = &ARM7TDMI::ARMemptyInstruction; // undecoded opcode
    }
}
inline void ARM7TDMI::fillTHUMB() {
    for(uint16_t i = 0; i < 256; i++) {
        if((i & 0b11111111) == 0b11011111)
            thumbTable[i] = &ARM7TDMI::THUMBsoftwareInterrupt;
        else if((i & 0b11111111) == 0b10110000)
            thumbTable[i] = &ARM7TDMI::THUMBaddOffsetToSP;
        else if((i & 0b11111100) == 0b01000000)
            thumbTable[i] = &ARM7TDMI::THUMBaluOperations;
        else if((i & 0b11111100) == 0b01000100)
            thumbTable[i] = &ARM7TDMI::THUMBhiRegOpsBranchEx;
        else if((i & 0b11110110) == 0b10110100)
            thumbTable[i] = &ARM7TDMI::THUMBpushPopRegisters;
        else if((i & 0b11110000) == 0b11110000)
            thumbTable[i] = &ARM7TDMI::THUMBlongBranchWithLink;
        else if((i & 0b11111000) == 0b11100000)
            thumbTable[i] = &ARM7TDMI::THUMBunconditionalBranch;
        else if((i & 0b11111000) == 0b00011000)
            thumbTable[i] = &ARM7TDMI::THUMBaddSubtract;
        else if((i & 0b11111000) == 0b01001000)
            thumbTable[i] = &ARM7TDMI::THUMBloadPCRelative;
        else if((i & 0b11110000) == 0b11000000)
            thumbTable[i] = &ARM7TDMI::THUMBmultipleLoadStore;
        else if((i & 0b11110000) == 0b11010000)
            thumbTable[i] = &ARM7TDMI::THUMBconditionalBranch;
        else if((i & 0b11110000) == 0b10000000)
            thumbTable[i] = &ARM7TDMI::THUMBloadStoreHalfword;
        else if((i & 0b11110000) == 0b10100000)
            thumbTable[i] = &ARM7TDMI::THUMBgetRelativeAddress;
        else if((i & 0b11110000) == 0b10010000)
            thumbTable[i] = &ARM7TDMI::THUMBloadStoreSPRelative;
        else if((i & 0b11110010) == 0b01010000)
            thumbTable[i] = &ARM7TDMI::THUMBloadStoreRegOffset;
        else if((i & 0b11110010) == 0b01010010)
            thumbTable[i] = &ARM7TDMI::THUMBloadStoreSignExtendedByteHalfword;
        else if((i & 0b11100000) == 0b00100000)
            thumbTable[i] = &ARM7TDMI::THUMBmoveCompareAddSubtract;
        else if((i & 0b11100000) == 0b00000000)
            thumbTable[i] = &ARM7TDMI::THUMBmoveShiftedRegister;
        else if((i & 0b11100000) == 0b01100000)
            thumbTable[i] = &ARM7TDMI::THUMBloadStoreImmOffset;
        else
            thumbTable[i] = &ARM7TDMI::THUMBemptyInstruction;
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

inline bool ARM7TDMI::checkCond(uint32_t cond) {
    switch(cond) {
        case 0x00000000:
            return cpuState.zeroFlag;
        case 0x10000000:
            return !cpuState.zeroFlag;
        case 0x20000000:
            return cpuState.carryFlag;
        case 0x30000000:
            return !cpuState.carryFlag;
        case 0x40000000:
            return cpuState.signFlag;
        case 0x50000000:
            return !cpuState.signFlag;
        case 0x60000000:
            return cpuState.overflowFlag;
        case 0x70000000:
            return !cpuState.overflowFlag;
        case 0x80000000:
            return cpuState.carryFlag && (!cpuState.zeroFlag);
        case 0x90000000:
            return(!cpuState.carryFlag) || cpuState.zeroFlag;
        case 0xA0000000:
            return cpuState.signFlag == cpuState.overflowFlag;
        case 0xB0000000:
            return cpuState.signFlag != cpuState.overflowFlag;
        case 0xC0000000:
            return (!cpuState.zeroFlag) && (cpuState.signFlag == cpuState.overflowFlag);
        case 0xD0000000:
            return cpuState.zeroFlag || (cpuState.signFlag != cpuState.overflowFlag);
        case 0xE0000000:
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
        cpuState.overflowFlag = (op1 ^ op2) ? 0 : (result >> 31) ^ op1; // todo: check if overflow calc for carry opcodes are correct
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