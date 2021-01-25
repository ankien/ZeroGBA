#include "ARM7TDMI.hpp"

void ARM7TDMI::fillARM() {
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
void ARM7TDMI::fillTHUMB() {
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

void ARM7TDMI::handleException(uint8_t exception, uint32_t nn, uint8_t newMode) {
    setModeArrayIndex(newMode,14,pc+nn);
    setModeArrayIndex(newMode,'S',getCPSR());
    state = 0;
    mode = newMode;
    irqDisable = 1;
    
    if((mode == Reset) || (mode == FIQ))
        fiqDisable = 1;

    switch(mode) {

        case Supervisor:

            switch(exception) {

                case Reset:
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

uint32_t ARM7TDMI::getModeArrayIndex(uint8_t mode, uint8_t reg) {
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
void ARM7TDMI::setModeArrayIndex(uint8_t mode, uint8_t reg, uint32_t arg) {
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
uint32_t ARM7TDMI::getArrayIndex(uint8_t reg) {
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
void ARM7TDMI::setArrayIndex(uint8_t reg, uint32_t arg) {
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

bool ARM7TDMI::checkCond(uint32_t cond) {
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
            return(!carryFlag) && zeroFlag;
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

void ARM7TDMI::ARMundefinedInstruction(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undefined Instruction=",pc);
    #endif
    pc+=4;
}
void ARM7TDMI::ARMemptyInstruction(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undecoded Instruction=",pc);
    #endif
    pc+=4;
}
void ARM7TDMI::THUMBemptyInstruction(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undecoded Instruction=",pc);
    #endif
    pc+=2;
}

void ARM7TDMI::ARMbranch(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Branch=",pc);
    #endif
    // Implementation dependent sign extend
    int32_t signedOffset = instruction & 0xFFFFFF;
    signedOffset <<= 8;
    signedOffset >>= 6;
    if(instruction & 0x1000000)
        setArrayIndex(14,pc+4);
    pc += 8 + signedOffset;
}
void ARM7TDMI::ARMbranchExchange(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Branch Exchange=",pc);
    #endif
    uint32_t rn = instruction & 0xF;
    if(rn == 15)
        rn = pc+8;
    else
        rn = getArrayIndex(rn);
    if(rn & 1) {
        state = 1;
        rn--;
    }
    pc = rn;
}
void ARM7TDMI::ARMsoftwareInterrupt(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X SWI=",pc);
    #endif
    //handleException(SoftwareInterrupt,4,Supervisor); Stubbed for now
    pc+=4;
}

void ARM7TDMI::ARMdataProcessing(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Data Proc=",pc);
    #endif
    bool I = instruction & 0x2000000;
    bool s = instruction & 0x100000;
    uint8_t opcode = (instruction & 0x1E00000) >> 21;
    uint32_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t op2;
    bool pcIsRn = rn == 15;

    // forcefully set flags for certain opcodes
    switch(opcode) {
        case 0xA:
        case 0xB:
        case 0x9:
        case 0x8:
            s = 1;
    }

    // shifting
    switch(I) {

        case 0: // register is second operand
        {
            uint8_t shiftType = (instruction & 0x60) >> 5;
            uint8_t r = instruction & 0x10; // if rm or rn == 15, and r == 1, pc+=12, else pc+=8
            uint8_t rm = instruction & 0xF;
            switch(r) {
                case 0: // register operand w/ immediate shift
                {
                    uint8_t Is = (instruction & 0xF80) >> 7;
                    if(pcIsRn || (rm == 15))
                        pc+=8;

                    op2 = ALUshift(getArrayIndex(rm),Is,shiftType,s,1);

                    if(pcIsRn || (rm == 15))
                        pc-=8;
                    break;
                }

                default: // register operand w/ register shift
                    uint8_t Rs = (instruction & 0xF00) >> 8;
                    if((rn == 15) || (rm == 15))
                        pc+=12;
                    op2 = ALUshift(getArrayIndex(rm),getArrayIndex(Rs) & 0xFF,shiftType,s,0);
                    if((rn == 15) || (rm == 15))
                        pc-=12;
                
            }
            break;

        }

        default: // immediate is second operand
            uint32_t Imm = instruction & 0xFF;
            uint8_t rotate = (instruction & 0xF00) >> 8;
            op2 = ALUshift(Imm,rotate*2,0b11,s,0);
            break;
    }

    // when writing to R15
    if((s) && (rd == 15)) {
        setCPSR(spsr[getArrayIndex('S')]);
        s = 0;
    }    

    if(pcIsRn)
        rn = pc + 8;
    else
        rn = getArrayIndex(rn);
    // good tip to remember: signed and unsigned integer ops produce the same binary
    uint32_t result;
    switch(opcode) { // could optimize this since we don't need the actual opcode value
        case 0x0: // AND
            result = rn & op2;
            setArrayIndex(rd,result);
            break;
        case 0x1: // EOR
            result = rn ^ op2;
            setArrayIndex(rd,result);
            break;
        case 0x2: // SUB
            result = sub(rn,op2,s);
            setArrayIndex(rd,result);
            break;
        case 0x3: // RSB
            result = sub(op2,rn,s);
            setArrayIndex(rd,result);
            break;
        case 0x4: // ADD
            result = add(rn,op2,s);
            setArrayIndex(rd,result);
            break;
        case 0x5: // ADC
            result = addCarry(rn,op2,s);
            setArrayIndex(rd,result);
            break;
        case 0x6: // SBC
            result = subCarry(rn,op2,1);
            setArrayIndex(rd,result);
            break;
        case 0x7: // RSC
            result = subCarry(op2,rn,1);
            setArrayIndex(rd,result);
            break;
        case 0x8: // TST
            result = rn & op2;
            break;
        case 0x9: // TEQ
            result = rn ^ op2;
            break;
        case 0xA: // CMP
            result = sub(rn,op2,s);
            break;
        case 0xB: // CMN
            result = add(rn,op2,s);
            break;
        case 0xC: // ORR
            result = rn | op2;
            setArrayIndex(rd,result);
            break;
        case 0xD: // MOV
            result = op2;
            setArrayIndex(rd,result);
            break;
        case 0xE: // BIC
            result = rn & (~op2);
            setArrayIndex(rd,result);
            break;
        case 0xF: // MVN
            result = ~op2;
            setArrayIndex(rd,result);
    }

    if(s)
        setZeroAndSign(result);
    if(rd != 15)
        pc+=4;
}
void ARM7TDMI::ARMmultiplyAndMultiplyAccumulate(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Multiply and MulAcc=",pc);
    #endif
    uint8_t opcode = (instruction & 0x1E00000) >> 21;
    bool s = instruction & 0x100000;
    uint8_t rd = (instruction & 0xF0000) >> 16;
    uint8_t rn = (instruction & 0xF000) >> 12;
    uint8_t rs = (instruction & 0xF00) >> 8;
    uint8_t rm = instruction & 0xF;

    uint64_t rdValue = getArrayIndex(rd); // hi
    uint64_t rnValue = getArrayIndex(rn); // lo
    uint64_t rsValue = getArrayIndex(rs);
    uint64_t rmValue = getArrayIndex(rm);

    uint8_t m;
    if(((rsValue >> 8) == 0xFFFFFF) || ((rsValue >> 8) == 0))
        m = 1;
    else if(((rsValue >> 16) == 0xFFFF) || ((rsValue >> 16) == 0))
        m = 2;
    else if(((rsValue >> 24) == 0xFF) || ((rsValue >> 24) == 0))
        m = 3;
    else
        m = 4;

    uint64_t result;
    switch(opcode) {
        case 0b0000: // MUL
            result = rmValue * rsValue;
            setArrayIndex(rd,result);
            break;
        case 0b0001: // MLA
            result = (rmValue * rsValue) + rnValue;
            setArrayIndex(rd,result);
            break;
        case 0b0100: // UMULL
            
            result = rmValue * rsValue;
            setArrayIndex(rd,result >> 32);
            setArrayIndex(rn,result);
            break;
        case 0b0101: // UMLAL
            
            result = (rmValue * rsValue) + ((rdValue << 32) | rnValue);
            setArrayIndex(rd,result >> 32);
            setArrayIndex(rn,result);
            break;
        case 0b0110: // SMULL
            
            result = rmValue * rsValue;
            setArrayIndex(rd,result >> 32);
            setArrayIndex(rn,result);
            break;
        case 0b0111: // SMLAL
            
            int64_t hiLo = ((rdValue << 32) | rnValue);
            result = (rmValue * rsValue) + hiLo;
            setArrayIndex(rd,result >> 32);
            setArrayIndex(rn,result);
            break;
    }

    if(s)
        setZeroAndSign(result);

    if(rd != 15)
        pc+=4;
}

void ARM7TDMI::ARMpsrTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X PSR Transfer=",pc);
    #endif
    
    
    bool psr = instruction & 0x400000;
    bool msr = instruction & 0x200000;

    if(msr) {
        uint32_t op;
        switch(instruction & 0x2000000) {
            case 0: // Reg
                op = getArrayIndex(instruction & 0xF);
                break;
            default: // Imm
                uint32_t Imm = instruction & 0xF;
                uint8_t rotate = (instruction & 0xF00) >> 8;
                op = ALUshift(Imm, rotate * 2, 0b11,1,0);
        }

        if(!(instruction & 0x80000))
            op &= 0xFFFFFF;
        if(!(instruction & 0x40000))
            op &= 0xFF00FFFF;
        if(!(instruction & 0x20000))
            op &= 0xFFFF0FFF;
        if(!(instruction & 0x10000))
            op &= 0xFFFFF000;

        switch(psr) {
            case 0:
                setCPSR(op);
                break;
            default:
                setArrayIndex('S', op);
        }
    } else {
        uint8_t rd = (instruction & 0xF000) >> 12;
        switch(mode) {
            case 16:
            case 31:
                if(!psr)
                    setArrayIndex(rd, getCPSR());
                break;
            default:
                switch(psr) {
                    case 0:
                        setArrayIndex(rd, getCPSR());
                        break;
                    default:
                        setArrayIndex(rd, getArrayIndex('S'));
                }
        }
    }
    
    pc+=4;
}

void ARM7TDMI::ARMsingleDataTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Single Data Transfer=",pc);
    #endif
    
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    bool b = instruction & 0x400000;
    bool ldr = instruction & 0x100000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getArrayIndex(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x2000000) {        
        case 0: // immediate offset
            offset = instruction & 0xFFF;
            break;
        default: // register offset
            uint8_t Is = (instruction & 0xF80) >> 7;
            uint8_t shiftType = (instruction & 0x60) >> 5;
            uint8_t rm = instruction & 0xF;
            offset = ALUshift(getArrayIndex(rm),Is,shiftType,1,1);
            
    }

    if(p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    switch(ldr) {
        case 0: // STR
            
            switch(b) {
                case 0:
                    storeValue(getArrayIndex(rd),address);
                    break;
                default:
                    (*systemMemory)[address] = getArrayIndex(rd);
            }
            if(rd == 15)
                (*systemMemory)[address] = (*systemMemory)[address]+12;
            break;
        default: // LDR
                
            switch(b) {
                case 0:
                    setArrayIndex(rd, readWordRotate(address));
                    break;
                default:
                    setArrayIndex(rd, (*systemMemory)[address]);
            }
    }

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((instruction & 0x200000) || !p) // write-back address
        setArrayIndex(rn,address);

    if(rd != 15)
        pc+=4;
}
void ARM7TDMI::ARMhdsDataSTRH(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X STRH=",pc);
    #endif
    
    
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getArrayIndex(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getArrayIndex(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    storeValue(static_cast<uint16_t>(getArrayIndex(rd)),address);

    if(rd == 15)
        (*systemMemory)[address] = (*systemMemory)[address]+12;

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((instruction & 0x200000) || !p) // write-back address
        setArrayIndex(rn,address);

    pc+=4;
}
void ARM7TDMI::ARMhdsDataLDRH(uint32_t instruction) { 
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRH=",pc);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getArrayIndex(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getArrayIndex(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    setArrayIndex(rd,readHalfWordRotate(address));

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((instruction & 0x200000) || !p) // write-back address
        setArrayIndex(rn,address);

    if(rd != 15)
        pc+=4;
}
void ARM7TDMI::ARMhdsDataLDRSB(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRSB=",pc);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getArrayIndex(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getArrayIndex(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    setArrayIndex(rd,static_cast<int>(reinterpret_cast<int8_t&>((*systemMemory)[address])));

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((instruction & 0x200000) || !p) // write-back address
        setArrayIndex(rn,address);

    if(rd != 15)
        pc+=4;
}
void ARM7TDMI::ARMhdsDataLDRSH(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRSH=",pc);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getArrayIndex(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getArrayIndex(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    setArrayIndex(rd,static_cast<int>(static_cast<int16_t>(readHalfWordRotate(address))));

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((instruction & 0x200000) || !p) // write-back address
        setArrayIndex(rn,address);

    if(rd != 15)
        pc+=4;
}
void ARM7TDMI::ARMblockDataTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Block Data Transfer=",pc);
    #endif
    
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    bool s = instruction & 0x400000;
    bool w = instruction & 0x200000;
    bool l = instruction & 0x100000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint16_t rListBinary = instruction & 0xFFFF;
    uint32_t address = getArrayIndex(rn);
    uint16_t rnInList = rListBinary & (1 << rn);
    bool pcInList = 0;
    uint8_t oldMode = 0;
    int8_t offset;
    std::vector<uint8_t> rListVector;

    /* todo: if rList is empty
    if(!rList) {
        rList = 0x8000;
        
    }
    */

    uint16_t bitPos = 1;
    for(uint8_t i = 0; i < 16; i++) {
        if(rListBinary & bitPos) {
            rListVector.emplace_back(i);
            if(i == 15)
                pcInList = 1;
        }
        bitPos <<= 1;
    }

    if(u) {
        offset = 4;
        // if offset is positive do nothing
    } else {
        offset = -4;
        // if offset is negative, reverse the list order
        std::reverse(rListVector.begin(),rListVector.end());
    }

    if(s) { // If s bit is set and rn is in transfer list
        if(rListBinary & rnInList) {
            if(l)
                setCPSR(getArrayIndex('S'));
            else {
                oldMode = mode;
                mode = User;
            }
        }
    }

    if(l) { // LDM

        for(uint8_t num : rListVector) {
            if(p)
                address += offset;
            setArrayIndex(num,readWord(address));
            if(!p)
                address += offset;
        }
    } else { // STM

        
        
        for(uint8_t num : rListVector) {
            
            if(p)
                address += offset;

            if(num == 15)
                storeValue(getArrayIndex(num),address+12);
            else
                storeValue(getArrayIndex(num),address);
                
            // if base register is not the first store you do, and wb bit is enabled, wb to base register(line 1055) and new base address
            if((num == rn) && (rn != rListVector[0]) && w)
            storeValue(address,address);
            
            if(!p)
                address += offset;
        }

    }

    if(oldMode) {
        mode = oldMode;
        w = 0;
    }
    
    // if STM wb bit is set, write-back; if LDM wb bit is set and rn not in list, write-back
    if(w && !(l && rnInList)) 
        setArrayIndex(rn,address);
    
    if(!pcInList)
        pc+=4;
}
void ARM7TDMI::ARMswap(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Swap=",pc);
    #endif
    
    
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint8_t rm = instruction & 0xF;

    // swap byte
    if(instruction & 0x400000) {
        uint32_t rnValue = getArrayIndex(rn);
        setArrayIndex(rd,(*systemMemory)[rnValue]);
        (*systemMemory)[rnValue] = getArrayIndex(rm);
    } else { // swap word
        uint32_t rnValue = getArrayIndex(rn);
        setArrayIndex(rd,readWordRotate(rnValue));
        storeValue(getArrayIndex(rm),rnValue);
    }

    if(rd != 15)
        pc+=4;
}

void ARM7TDMI::THUMBmoveShiftedRegister(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move Shifted Reg=",pc);
    #endif
    
    
    uint8_t offset = (instruction & 0x7C0) >> 6;
    uint32_t rs = (instruction & 0x38) >> 3;
    uint8_t rd = instruction & 0x7;

    rs = getArrayIndex(rs);
    switch(instruction & 0x1800) {
        case 0x0000: // LSL
            setArrayIndex(rd,ALUshift(rs,offset,0,1,1));
            break;
        case 0x0800: // LSR
            setArrayIndex(rd,ALUshift(rs,offset,1,1,1));
            break;
        case 0x1000: // ASR
            setArrayIndex(rd,ALUshift(rs,offset,0b10,1,1));
            break;
    }

    setZeroAndSign(getArrayIndex(rd));
    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBaddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Subtract=",pc);
    #endif
    
    
    uint32_t rs = getArrayIndex((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;

    uint32_t result;
    switch(instruction & 0x600) {
        case 0x000: // add reg
        {
            uint32_t rn = getArrayIndex((instruction & 0x1C0) >> 6);
            result = add(rs,rn,true);
            setArrayIndex(rd,result);
            break;
        }
        case 0x200: // sub reg
        {
            uint32_t rn = getArrayIndex((instruction & 0x1C0) >> 6);
            result = sub(rs,rn,true);
            setArrayIndex(rd,result);
            break;
        }
        case 0x400: // add imm
            result = add(rs,(instruction & 0x1C0) >> 6,true);
            setArrayIndex(rd,result);
            break;
        case 0x600: // sub imm
            result = sub(rs,(instruction & 0x1C0) >> 6,true);
            setArrayIndex(rd,result);
    }

    setZeroAndSign(result);
    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBmoveCompareAddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move/Compare/Add/Subtract=",pc);
    #endif
    
    
    uint8_t rd = (instruction &  0x700) >> 8;
    uint8_t nn = instruction & 0xFF;
    uint32_t result;

    switch(instruction & 0x1800) {
        case 0x0000: // mov
            result = nn;
            setArrayIndex(rd,result);
            break;
        case 0x0800: // cmp
            result = sub(getArrayIndex(rd),nn,s);
            break;
        case 0x1000: // add
            result = add(getArrayIndex(rd),nn,s);
            setArrayIndex(rd,result);
            break;
        case 0x1800: // sub
            result = sub(getArrayIndex(rd),nn,s);
            setArrayIndex(rd,result);
            break;
    }

    setZeroAndSign(result);
    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBaluOperations(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X ALU Operation=",pc);
    #endif
    
    uint32_t rs = getArrayIndex((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;
    uint32_t rdValue = getArrayIndex(rd);

    uint32_t result;
    switch((instruction & 0x3C0) >> 6) {
        case 0x0: // AND
            
            result = rdValue & rs;
            setArrayIndex(rd,result);
            break;
        case 0x1: // EOR
            
            result = rdValue ^ rs;
            setArrayIndex(rd,result);
            break;
        case 0x2: // LSL
            
            result = ALUshift(rdValue,rs & 0xFF,0,1,0);
            setArrayIndex(rd,result);
            break;
        case 0x3: // LSR
            
            result = ALUshift(rdValue,rs & 0xFF,1,1,0);
            setArrayIndex(rd,result);
            break;
        case 0x4: // ASR
            
            result = ALUshift(rdValue,rs & 0xFF,2,1,0);
            setArrayIndex(rd,result);
            break;
        case 0x5: // ADC
            
            result = addCarry(rdValue,rs,1);
            setArrayIndex(rd,result);
            break;
        case 0x6: // SBC
            
            result = subCarry(rdValue,rs,1);
            setArrayIndex(rd,result);
            break;
        case 0x7: // ROR
            
            result = ALUshift(rdValue,rs & 0xFF,3,1,0);
            setArrayIndex(rd,result);
            break;
        case 0x8: // TST
            
            result = rdValue & rs;
            break;
        case 0x9: // NEG
            
            result = ~rs + 1;
            setArrayIndex(rd,result);
            break;
        case 0xA: // CMP
            
            result = sub(rdValue,rs,1);
            break;
        case 0xB: // CMN
            
            result = add(rdValue,rs,1);
            break;
        case 0xC: // ORR
            
            result = rdValue | rs;
            setArrayIndex(rd,result);
            break;
        case 0xD: // MUL
        {
            uint8_t topNibble = rdValue >> 28;
            uint8_t m = 0;
            
            for(uint8_t i = 1; i < 0x10; i <<= 1) {
                if(i & m)
                    m++;
            }
            
            
            result = rdValue * rs;
            setArrayIndex(rd,result);
            break;
        }
        case 0xE: // BIC
            
            result = rdValue & (~rs);
            setArrayIndex(rd,result);
            break;
        case 0xF: // MVN
            
            result = ~rs;
            setArrayIndex(rd,result);
    }

    setZeroAndSign(result);
    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBhiRegOpsBranchEx(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Hi Register Operation=",pc);
    #endif
    
    uint16_t opcode = instruction & 0x300;
    uint8_t rs = (instruction & 0x38) >> 3;
    uint8_t rd = instruction & 0x7;
    bool h1 = instruction & 0x80;
    bool h2 = instruction & 0x40;
    if(h1)
        rd+=8;
    if(h2)
        rs+=8;
    uint32_t rsValue = getArrayIndex(rs);
    uint32_t rdValue = getArrayIndex(rd);

    
    if(opcode != 0x300) {
        if(rd == 15)
            rdValue += 4;
        if(rs == 15)
            rsValue += 4;
    } else {
        if(rs == 15)
            rsValue+=4;
    }
    

    switch(opcode) {
        case 0x000: // ADD
            
            setArrayIndex(rd,add(rdValue,rsValue,0));
            break;
        case 0x100: // CMP
            
            sub(rdValue,rsValue,1);
            break;
        case 0x200: // MOV
            
            setArrayIndex(rd,rsValue);
            break;
        case 0x300: // BX
            
            if(!(rsValue & 1))
                state = 0;
            setArrayIndex(15,rsValue);
    }

    if((opcode != 0x300) && (rd != 15))
        pc+=2;
}

void ARM7TDMI::THUMBloadPCRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load PC-Relative=",pc);
    #endif
    
    
    uint8_t rd = (instruction & 0x700) >> 8;
    setArrayIndex(rd,readWord(((pc+4) & ~2) + (instruction & 0xFF) * 4));
    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBloadStoreRegOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Reg Offset=",pc);
    #endif
    
    uint32_t rd = instruction & 0x7;
    uint32_t rb = getArrayIndex((instruction & 0x38) >> 3);
    uint32_t ro = getArrayIndex((instruction & 0x1C0) >> 6);

    switch(instruction & 0xC00) {
        case 0x000: // STR
            
            rd = getArrayIndex(rd);
            storeValue(rd,rb+ro);
            break;
        case 0x400: // STRB
            
            rd = getArrayIndex(rd);
            (*systemMemory)[rb+ro] = *reinterpret_cast<uint8_t*>(&rd);
            break;
        case 0x800: // LDR
            
            setArrayIndex(rd,readWordRotate(rb+ro));
            break;
        case 0xC00: // LDRB
            
            setArrayIndex(rd,(*systemMemory)[rb+ro]);
    }

    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBloadStoreSignExtendedByteHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Sign Extended Byte/Halfword=",pc);
    #endif
    
    uint32_t rd = instruction & 0x7;
    uint32_t rb = getArrayIndex((instruction & 0x38) >> 3);
    uint32_t ro = getArrayIndex((instruction & 0x7C0) >> 6);

    switch(instruction & 0xC00) {
        case 0x000: // STRH
            
            rd = getArrayIndex(rd);
            storeValue(*reinterpret_cast<uint16_t*>(&rd),rb+ro);
            break;
        case 0x400: // LDSB
            
            setArrayIndex(rd,(*systemMemory)[rb+ro]);
            break;
        case 0x800: // LDRH
            
            setArrayIndex(rd,readHalfWordRotate(rb+ro));
            break;
        case 0xC00: // LDSH
            
            setArrayIndex(rd,static_cast<int32_t>(static_cast<int16_t>(readHalfWord(rb+ro))));
    }

    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBloadStoreImmOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Immediate Offset=",pc);
    #endif
    
    uint8_t rd = instruction & 0x7;
    uint32_t rb = getArrayIndex((instruction & 0x38) >> 3);
    uint32_t nn = (instruction & 0x7C0) >> 6;

    switch(instruction & 0x1800) {
        case 0x0000: // STR
            
            storeValue(getArrayIndex(rd),rb+(nn << 2));
            break;
        case 0x0800: // LDR
            
            setArrayIndex(rd,readWordRotate(rb+(nn << 2)));
            break;
        case 0x1000: // STRB
            
            (*systemMemory)[rb+nn] = getArrayIndex(rd);
            break;
        case 0x1800: // LDRB
            
            setArrayIndex(rd,(*systemMemory)[rb+nn]);
            break;
    }

    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBloadStoreHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Halfword=",pc);
    #endif
    
    uint8_t rd = instruction & 0x7;
    uint32_t rb = getArrayIndex((instruction & 0x38) >> 3);
    uint32_t nn = (instruction & 0x7C0) >> 5;

    switch(instruction & 0x800) {
        case 0x000: // STRH
            
            storeValue(static_cast<uint16_t>(getArrayIndex(rd)),rb+nn);
            break;
        case 0x800: // LDRH
            
            setArrayIndex(rd,readHalfWordRotate(rb+nn));
            break;
    }

    if(rd != 15)
        pc+=2;
}
void ARM7TDMI::THUMBloadStoreSPRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store SP Relative=",pc);
    #endif
    
    uint8_t rd = (instruction & 0x700) >> 8;
    uint16_t nn = (instruction & 0xFF) << 2;

    if(instruction & 0x800) { // LDR
        
        setArrayIndex(rd,readWordRotate(getArrayIndex(13)+nn));
    } else { // STR
        
        storeValue(getArrayIndex(rd),getArrayIndex(13)+nn);
    }

    if(rd != 15)
        pc+=2;
}

void ARM7TDMI::THUMBgetRelativeAddress(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Get Relative Address=",pc);
    #endif
    
    
    uint16_t nn = (instruction & 0xFF) << 2;
    uint8_t rd = (instruction & 0x700) >> 8;
    if(instruction & 0x800) // ADD SP
        setArrayIndex(rd,getArrayIndex(13) + nn);
    else // ADD PC
        setArrayIndex(rd,((pc+4) & ~2) + nn);
    if(rd != 15)
        pc+=2;
}

void ARM7TDMI::THUMBaddOffsetToSP(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Offset To SP=",pc);
    #endif
    
    
    uint16_t nn = (instruction & 0x7F) << 2;
    if(instruction & 0x80) // - offset
        setArrayIndex(13,getArrayIndex(13) - nn);
    else // + offset
        setArrayIndex(13,getArrayIndex(13) + nn);;
    pc+=2;
}

void ARM7TDMI::THUMBpushPopRegisters(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Push/Pop Registers=",pc);
    #endif
    
    
    uint8_t rListBinary = instruction & 0xFF;
    uint8_t rListOffset;
    bool pclrBit = instruction & 0x100;
    bool pop = instruction & 0x800;
    uint32_t address = getArrayIndex(13);

    if(pop) { // POP, AKA load from memory
        

        rListOffset = 1;
        for(uint8_t i = 0; i < 8; i++) {
            if(instruction & rListOffset) {
                setArrayIndex(i,readWord(address));
                address+=4;
                
            }
            rListOffset <<= 1;
        }

        if(pclrBit) {
            setArrayIndex(15,readWord(address));
            address+=4;
            
        }
    } else { // PUSH, AKA store to memory
        
        if(pclrBit) {
            address-=4;
            storeValue(getArrayIndex(14),address);
        }
        
        rListOffset = 0x80;
        for(int8_t i = 7; i > -1; i--) {
            if(instruction & rListOffset) {
                address-=4;
                storeValue(getArrayIndex(i),address);
            }
            rListOffset >>= 1;
        }
    }

    setArrayIndex(13,address);
    if(!pclrBit || !pop)
        pc+=2;
}
void ARM7TDMI::THUMBmultipleLoadStore(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Multiple Load Store=",pc);
    #endif
    
    
    uint8_t rListBinary = instruction & 0xFF;
    uint8_t rListOffset = 1;
    uint8_t rb = (instruction & 0x700) >> 8;
    uint32_t address = getArrayIndex(rb);

    if(!rListBinary) { // todo: handle empty rList
        
        address += 0x40;
    } else {
        if(instruction & 0x800) { // LDMIA
            
            for(uint8_t i = 0; i < 8; i++) {
                if(instruction & rListOffset) {
                    setArrayIndex(i, readWord(address));
                    address += 4;
                    
                }
                rListOffset <<= 1;
            }
        } else { // STMIA
            
            for(uint8_t i = 0; i < 8; i++) {
                if(instruction & rListOffset) {
                    storeValue(getArrayIndex(i), address);
                    address += 4;
                        
                }
                rListOffset <<= 1;
            }
        }
    }
    setArrayIndex(rb, address);
    pc+=2;
}

void ARM7TDMI::THUMBconditionalBranch(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Conditional Branch=",pc);
    #endif
    
    int16_t offset = static_cast<int8_t>(instruction & 0xFF) * 2 + 4;
    bool condMet = false;

    
    condMet = checkCond((instruction & 0xF00) << 20);
    if(condMet)
        pc += offset;
    else {
        
        pc+=2;
    }
}
void ARM7TDMI::THUMBunconditionalBranch(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Unconditional Branch=",pc);
    #endif
    
    
    int16_t offset = static_cast<int16_t>((instruction & 0x7FF) << 5) >> 4;
    pc += 4 + offset;
}
void ARM7TDMI::THUMBlongBranchWithLink(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Long Branch With Link=",pc);
    #endif
    
    int32_t offset = instruction & 0x7FF;
    bool instrTwo = instruction & 0x800;
    
    if(instrTwo) { // instr 2
        
        uint32_t oldPC = (pc + 2) | 1;
        pc=getArrayIndex(14)+(offset << 1);
        setArrayIndex(14,oldPC);
    } else { // instr 1
        
        offset = (offset << 21) >> 9;
        setArrayIndex(14,pc+4+offset);
    }
    
    if(!instrTwo)
        pc+=2;
}
void ARM7TDMI::THUMBsoftwareInterrupt(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%XTHUMB Software Interrupt=",pc);
    #endif
    
    
    handleException(SoftwareInterrupt,2,Supervisor);
}
