#include "ARM7TDMI.hpp"

ARM7TDMI::ARM7TDMI(uint8_t* systemMemory) {
    this->systemMemory = systemMemory;
}

void ARM7TDMI::fillARM() {
    
    for(uint16_t i = 0; i < 4096; i++) {

        // least precise at the bottom of the chain
        if((i & 0b111000000000) == 0b101000000000)
            armTable[i] = &ARM7TDMI::ARMbranch;
        else if((i & 0b111111111111) == 0b000100100001)
            armTable[i] = &ARM7TDMI::ARMbranchExchange;
        else if((i & 0b111100000000) == 0b111100000000)
            armTable[i] = &ARM7TDMI::ARMsoftwareInterrupt;
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
        else if((i & 0b111000011111) == 0b000000001101)
            armTable[i] = &ARM7TDMI::ARMhdsDataLDRD;
        else if((i & 0b111000011111) == 0b000000001111)
            armTable[i] = &ARM7TDMI::ARMhdsDataSTRD;
        else if((i & 0b111000011111) == 0b000000011001)
            armTable[i] = &ARM7TDMI::ARMhdsDataLDRH;
        else if((i & 0b111000011111) == 0b000000011011)
            armTable[i] = &ARM7TDMI::ARMhdsDataLDRSB;
        else if((i & 0b111000011111) == 0b000000011111)
            armTable[i] = &ARM7TDMI::ARMhdsDataLDRSH;
        else if((i & 0b110000000000) == 0b000000000000)
            armTable[i] = &ARM7TDMI::ARMdataProcessing;
        else
            armTable[i] = &ARM7TDMI::emptyInstruction;

    }
}
void ARM7TDMI::fillTHUMB() {
    
    for(uint8_t i = 0; i < 256; i++) {

        

    }
}

// Bits 27-20 + 7-4
inline uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}
// Bits 8-15
inline uint8_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 8;
}

inline void ARM7TDMI::storeValue(uint16_t value, uint32_t address) {
    systemMemory[address] = value;
    systemMemory[address + 1] = (value & 0xFF00) >> 8;
}
inline void ARM7TDMI::storeValue(uint32_t value, uint32_t address) {
    systemMemory[address] = value;
    systemMemory[address + 1] = (value & 0xFF00) >> 8;
    systemMemory[address + 2] = (value & 0xFF0000) >> 16;
    systemMemory[address + 3] = (value & 0xFF000000) >> 24;
}
inline uint16_t ARM7TDMI::readHalfWord(uint32_t address) {
    return systemMemory[address] |
           systemMemory[address + 1] << 8;
}
inline uint32_t ARM7TDMI::readWord(uint32_t address) {
    return systemMemory[address] |
           systemMemory[address + 1] << 8 |
           systemMemory[address + 2] << 16 |
           systemMemory[address + 3] << 24;
}
// Memory alignment stuff
inline uint32_t ARM7TDMI::readWordRotate(uint32_t address) {
    uint32_t word = readWord(address);
    uint8_t rorAmount = (address & 3) << 3;
    return shift(word,rorAmount,3);
}

void ARM7TDMI::handleException(uint8_t exception, uint32_t nn, uint8_t newMode) {
    setModeArrayIndex(newMode,14,pc+nn);
    setModeArrayIndex(newMode,'S',getCPSR());
    state = 0;
    mode = newMode;
    irqDisable = 1;
    
    if((mode == Reset) || (mode == FIQ))
        fiqDisable = 1;

    // Might need to implement a return based on mode
    switch(mode) {

        case Supervisor:

            switch(exception) {

                case Reset:
                    // need to change this once i get memory working, too lazy for the others though ;-)
                    pc = 0x0;
                    break;
                case AddressExceeds26Bit:
                    pc = 0x14;
                    break;
                case SoftwareInterrupt:
                    pc = 0x8;
                    break;
            }
            break;

        case Undefined:
            
            switch(exception) {
                
                case UndefinedInstruction:
                    pc = 0x4;
                    break;
            }
            break;

        case Abort:

            switch(exception) {

                case DataAbort:
                    pc = 0x10;
                    break;
                case PrefetchAbort:
                    pc = 0xC;
                    break;
            }
            break;

        case IRQ:
            
            switch(exception) {

                case NormalInterrupt:
                    pc = 0x18;
                    break;
            }
            break;

        case FIQ:
            
            switch(exception) {

                case FastInterrupt:
                    pc = 0x1C;
                    break;
            }
    }


}

inline uint32_t ARM7TDMI::getModeArrayIndex(uint8_t mode, uint8_t reg) {
    uint8_t index = 0;
    switch(mode) {
        case System:
        case User:
            break;
        case FIQ:
            index = 1;
            break;
        case Supervisor:
            index = 2;
            break;
        case Abort:
            index = 3;
            break;
        case IRQ:
            index = 4;
            break;
        case Undefined:
            index = 5;
    }

    switch(reg) {
        case 0:
            return this->reg[0];
        case 1:
            return this->reg[1];
        case 2:
            return this->reg[2];
        case 3:
            return this->reg[3];
        case 4:
            return this->reg[4];
        case 5:
            return this->reg[5];
        case 6:
            return this->reg[6];
        case 7:
            return this->reg[7];
        case 8:
            return this->r8[index];
        case 9:
            return this->r9[index];
        case 10:
            return this->r10[index];
        case 11:
            return this->r11[index];
        case 12:
            return this->r12[index];
        case 13:
            return this->r13[index];
        case 14:
            return this->r14[index];
        case 15:
            return pc;
        case 'S':
            return spsr[index];
    }
}
inline void ARM7TDMI::setModeArrayIndex(uint8_t mode, uint8_t reg, uint32_t arg) {
    uint8_t index = 0;
    switch(mode) {
        case System:
        case User:
            break;
        case FIQ:
            index = 1;
            break;
        case Supervisor:
            index = 2;
            break;
        case Abort:
            index = 3;
            break;
        case IRQ:
            index = 4;
            break;
        case Undefined:
            index = 5;
    }

    switch(reg) {
        case 0:
            this->reg[0] = arg;
            break;
        case 1:
            this->reg[1] = arg;
            break;
        case 2:
            this->reg[2] = arg;
            break;
        case 3:
            this->reg[3] = arg;
            break;
        case 4:
            this->reg[4] = arg;
            break;
        case 5:
            this->reg[5] = arg;
            break;
        case 6:
            this->reg[6] = arg;
            break;
        case 7:
            this->reg[7] = arg;
            break;
        case 8:
            r8[index] = arg;
            break;
        case 9:
            r9[index] = arg;
            break;
        case 10:
            r10[index] = arg;
            break;
        case 11:
            r11[index] = arg;
            break;
        case 12:
            r12[index] = arg;
            break;
        case 13:
            r13[index] = arg;
            break;
        case 14:
            r14[index] = arg;
            break;
        case 15:
            pc = arg;
            break;
        case 'S':
            spsr[index] = arg;
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

inline bool ARM7TDMI::checkCond(uint32_t cond) {
    switch(cond) {
        case 0x00000000:
            if(zeroFlag)
                return 1;
            break;
        case 0x10000000:
            if(!zeroFlag)
                return 1;
            break;
        case 0x20000000:
            if(carryFlag)
                return 1;
            break;
        case 0x30000000:
            if(!carryFlag)
                return 1;
            break;
        case 0x40000000:
            if(signFlag)
                return 1;
            break;
        case 0x50000000:
            if(!signFlag)
                return 1;
            break;
        case 0x60000000:
            if(overflowFlag)
                return 1;
            break;
        case 0x70000000:
            if(!overflowFlag)
                return 1;
            break;
        case 0x80000000:
            if(carryFlag && (!zeroFlag))
                return 1;
            break;
        case 0x90000000:
            if((!carryFlag) && zeroFlag)
                return 1;
            break;
        case 0xA0000000:
            if(signFlag == overflowFlag)
                return 1;
            break;
        case 0xB0000000:
            if(signFlag != overflowFlag)
                return 1;
            break;
        case 0xC0000000:
            if((!zeroFlag) && (signFlag == overflowFlag))
                return 1;
            break;
        case 0xD0000000:
            if(zeroFlag || (signFlag != overflowFlag))
                return 1;
            break;
        case 0xE0000000:
            return 1;
    }
    return 0;
}

template <typename INT>
inline INT ARM7TDMI::shift(INT value, uint8_t shiftAmount, uint8_t shiftType) {
    switch(shiftType) {
        case 0b00: // lsl
            return value << shiftAmount; 
        case 0b01: // lsr
            return value >> shiftAmount;
        case 0b10: // asr
            uint8_t dupeBit = (sizeof(value) * 8) - 1;
            if(value & (1 << dupeBit))
                return (value >> shiftAmount) | (0xFFFFFFFF << shiftAmount);
            return value >> shiftAmount;
        case 0b11: // ror
            return std::rotr(value,shiftAmount);
    }
}
inline void ARM7TDMI::setZeroAndSign(uint32_t arg) {
    (arg == 0) ?  zeroFlag = 1 : zeroFlag = 0;
    (arg | 0x80000000) ? signFlag = 1 : signFlag = 0;
}

void ARM7TDMI::emptyInstruction(uint32_t instruction) {}

void ARM7TDMI::ARMbranch(uint32_t instruction) {
    int32_t signedOffset = instruction & 0xFFFFFF;
    signedOffset <<= 8;
    signedOffset >>= 6;
    if(instruction & 0x1000000)
        r14[mode] = pc + 4;
    pc += 8 + signedOffset;
}
void ARM7TDMI::ARMbranchExchange(uint32_t instruction) {
    uint8_t rn = instruction & 0xF;
    if(rn == 0)
        state = 1;
    pc = getModeArrayIndex(mode,rn);
}
// Need to implement exception handling!
void ARM7TDMI::ARMsoftwareInterrupt(uint32_t instruction) {
    
}

void ARM7TDMI::ARMdataProcessing(uint32_t instruction) {
    uint32_t I = instruction & 0x2000000;
    uint32_t s = instruction & 0x100000;
    uint8_t opcode = (instruction & 0x1E00000) >> 21;
    uint32_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t op2;

    // shifting
    switch(I) {

        case 0: // register is second operand
            uint8_t shiftType = (instruction & 0x60) >> 5;
            uint8_t r = instruction & 0x10; // if rm or rn == 15, and r == 1, pc+=12, else pc+=8
            uint8_t rm = instruction & 0xF;
            switch(r) {
                case 16: // register operand w/ register shift
                    uint8_t Rs = (instruction & 0xF00) >> 8;
                    if((rn == 15) || (rm == 15))
                        pc+=12;
                    op2 = shift(getModeArrayIndex(mode,rm),(uint8_t)getModeArrayIndex(mode,Rs),shiftType);
                    if((rn == 15) || (rm == 15))
                        pc-=12;
                    break;
                case 0: // register operand w/ immediate shift
                    uint8_t Is = (instruction & 0xF80) >> 7;
                    if((rn == 15) || (rm == 15))
                        pc+=8;
                    if(Is == 0) // special behavior when shift amount is 0
                        switch(shiftType) {
                            case 0b00:
                                op2 = getModeArrayIndex(mode,rm);
                                break;
                            case 0b01:
                                op2 = 0;
                                carryFlag = (getModeArrayIndex(mode,rm) & 0x80000000) >> 31;
                                break;
                            case 0b10:
                                op2 = shift(getModeArrayIndex(mode,rm),32,shiftType);
                                carryFlag = op2 & 1;
                                break;
                            case 0b11:
                                op2 = shift(getModeArrayIndex(mode,rm),1,0b11);
                                op2 |= carryFlag << 31;
                                break;
                        }
                    else
                        op2 = shift(getModeArrayIndex(mode,rm),Is,shiftType);
                    if((rn == 15) || (rm == 15))
                        pc-=8;
            }
            break;

        case 0x2000000: // immediate is second operand
            uint8_t Imm = instruction & 0xF;
            uint8_t rotate = (instruction & 0xF00) >> 8;
            op2 = shift(Imm,rotate*2,0b11);
            pc+=8;
            break;
    }

    // when writing to R15
    if((s) && (rd == 15)) {
        setCPSR(spsr[getModeArrayIndex(mode,'S')]);
        s = 0;
    }    

    rn = getModeArrayIndex(mode,rn);
    uint32_t result;
    switch(opcode) { // could optimize this since we don't need the actual opcode value
        case 0x0: // AND
            result = rn & op2;
            setModeArrayIndex(mode,rd,result);
            break;
        case 0x1: // EOR
            result = rn ^ op2;
            setModeArrayIndex(mode,rd,result);
            break;
        case 0x2: // SUB
            result = rn - op2;
            setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0x3: // RSB
            result = op2 - rn;
            setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0x4: // ADD
            result = rn + op2;
            setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0x5: // ADC
            result = rn + op2 + (carryFlag << 31);
            setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0x6: // SBC
            result = rn - op2 + (carryFlag << 31) - 1;
            setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0x7: // RSC
            result = rn + op2 + (carryFlag << 31);
            setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0x8: // TST
            result = rn & op2;
            if(rd != 15)
                setModeArrayIndex(mode,rd,result);
            break;
        case 0x9: // TEQ
            result = rn ^ op2;
            if(rd != 15)
                setModeArrayIndex(mode,rd,result);
            break;
        case 0xA: // CMP
            result = rn - op2;
            if(rd != 15)
                setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0xB: // CMN
            result = rn + op2;
            if(rd != 15)
                setModeArrayIndex(mode,rd,result);
            if(s)
                carryFlag = (result & 0x80000000) >> 31;
            break;
        case 0xC: // ORR
            result = rn | op2;
            setModeArrayIndex(mode,rd,result);
            break;
        case 0xD: // MOV
            setModeArrayIndex(mode,rd,op2);
            break;
        case 0xE: // BIC
            result = rn & (~op2);
            setModeArrayIndex(mode,rd,result);
            break;
        case 0xF: // MVN
            setModeArrayIndex(mode,rd,~op2);
    }

    if(s)
        setZeroAndSign(result);
    pc+=4;
}
void ARM7TDMI::ARMmultiplyAndMultiplyAccumulate(uint32_t instruction) {
    uint8_t opcode = (instruction & 0x1E00000) >> 21;
    uint8_t s = (instruction & 0x100000) >> 20;
    uint64_t rd = (instruction & 0xF0000) >> 16;
    uint64_t rn = (instruction & 0xF000) >> 12;
    uint64_t rs = (instruction & 0xF00) >> 8;
    uint64_t rm = instruction & 0xF;

    rd = getModeArrayIndex(mode,rd); // hi
    rn = getModeArrayIndex(mode,rn); // lo
    rs = getModeArrayIndex(mode,rs);
    rm = getModeArrayIndex(mode,rm);
    uint64_t result;
    switch(opcode) {
        case 0b0000: // MUL
            result = rm * rs;
            setModeArrayIndex(mode,rd,result);
            break;
        case 0b0001: // MLA
            result = (rm * rs) + rn;
            setModeArrayIndex(mode,rd,result);
            break;
        case 0b0100: // UMULL
            result = rm * rs;
            setModeArrayIndex(mode,rd,result >> 32);
            setModeArrayIndex(mode,rn,result);
            break;
        case 0b0101: // UMLAL
            result = (rm * rs) + ((rd << 32) | rn);
            setModeArrayIndex(mode,rd,result >> 32);
            setModeArrayIndex(mode,rn,result);
            break;
        case 0b0110: // SMULL
            result = rm * rs;
            setModeArrayIndex(mode,rd,result >> 32);
            setModeArrayIndex(mode,rn,result);
            break;
        case 0b0111: // SMLAL
            int64_t hiLo = ((rd << 32) | rn);
            result = (rm * rs) + hiLo;
            setModeArrayIndex(mode,rd,result >> 32);
            setModeArrayIndex(mode,rn,result);
            break;
    }

    if(s)
        setZeroAndSign(result);

    pc+=4;
}

void ARM7TDMI::ARMpsrTransfer(uint32_t instruction) {
    uint8_t psr = 0;
    if(instruction & 0x400000)
        psr = 1;

    switch(instruction & 0x200000) {
        case 0x200000: // MSR
            uint32_t op;
            switch(instruction & 0x2000000) {
                case 0x2000000: // Imm
                    uint8_t Imm = instruction & 0xF;
                    uint8_t rotate = (instruction & 0xF00) >> 8;
                    op = shift(Imm,rotate*2,0b11);
                    break;
                case 0: // Reg
                    op = getModeArrayIndex(mode,instruction & 0xF);
            }

            if(!(instruction & 0x80000))
                op &= 0xFFFFFF;
            if(!(instruction & 0x40000))
                op &= 0xFF00FFFF;
            if(!(instruction & 0x20000))
                op &= 0xFFFF0FFF;
            if(!(instruction & 0x10000))
                op &= 0xFFFFF000;

            if(psr)
                setModeArrayIndex(mode,'S',op);
            else
                setCPSR(op);
            break;
        case 0: // MRS
            uint8_t rd = (instruction & 0xF000) >> 12;
            switch(mode) {
                case 16:
                case 31:
                    if(!psr)
                        setModeArrayIndex(mode,rd,getCPSR());
                    break;
                default:
                    if(psr)
                        setModeArrayIndex(mode,rd,getModeArrayIndex(mode,'S'));
                    else
                        setModeArrayIndex(mode,rd,getCPSR());
            }
    }

    pc+=4;
}
void ARM7TDMI::ARMsingleDataTransfer(uint32_t instruction) {
    uint32_t offset;
    uint32_t p = instruction & 0x1000000;
    uint32_t u = instruction & 0x800000;
    uint32_t b = instruction & 0x400000;
    uint8_t rn = 0xF0000 >> 16;
    uint8_t rd = 0xF000 >> 12;
    uint32_t address = getModeArrayIndex(mode,rn);

    if(instruction & 0x2000000) { // register offset
        uint8_t Is = (instruction & 0xF80) >> 7;
        uint8_t shiftType = (instruction & 0x60) >> 5;
        uint8_t rm = instruction & 0xF;
        offset = shift(getModeArrayIndex(mode,rm),Is,shiftType);
    } else // immediate offset
        offset = instruction & 0xFFF;

    if(p) {
        if(u)
            address += offset;
        else
            address -= offset;
    }

    switch(instruction & 0x100000) {
        case 0x100000: // LDR
            if(b)
                setModeArrayIndex(mode,rd,systemMemory[address]);
            else
                setModeArrayIndex(mode,rd,readWordRotate(address));
            break;
        case 0: // STR
            if(b)
                systemMemory[address] = getModeArrayIndex(mode,rd);
            else
                storeValue(getModeArrayIndex(mode,rd),address);
    }

    if(!p) {
        if(u)
            address += offset;
        else
            address -= offset;
    }

    if((instruction & 0x200000) || !p) // write-back address
        setModeArrayIndex(mode,rn,address);

    pc+=4;
}
void ARM7TDMI::ARMhdsDataSTRH(uint32_t instruction) {
    
}