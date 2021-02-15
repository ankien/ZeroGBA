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

void ARM7TDMI::ARMundefinedInstruction(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undefined Instruction=",r[15]);
    #endif
    r[15]+=4;
}
void ARM7TDMI::ARMemptyInstruction(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undecoded Instruction=",r[15]);
    #endif
    r[15]+=4;
}
void ARM7TDMI::THUMBemptyInstruction(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undecoded Instruction=",r[15]);
    #endif
    r[15]+=2;
}

void ARM7TDMI::ARMbranch(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Branch=",r[15]);
    #endif
    // Implementation dependent sign extend
    int32_t signedOffset = instruction & 0xFFFFFF;
    signedOffset <<= 8;
    signedOffset >>= 6;
    if(instruction & 0x1000000)
        setReg(14,r[15]+4);
    r[15] += 8 + signedOffset;
}
void ARM7TDMI::ARMbranchExchange(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Branch Exchange=",r[15]);
    #endif
    uint32_t rn = instruction & 0xF;
    if(rn == 15)
        rn = r[15]+8;
    else
        rn = getReg(rn);
    if(rn & 1) {
        state = 1;
        rn--;
    }
    r[15] = rn;
}
void ARM7TDMI::ARMsoftwareInterrupt(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X SWI=",r[15]);
    #endif
    //handleException(SoftwareInterrupt,4,Supervisor);
    //stub for Div
    int32_t signedNum = static_cast<int32_t>(getReg(0));
    int32_t signedDenom = static_cast<int32_t>(getReg(1));
    uint32_t unsignedNum = getReg(0);
    uint32_t unsignedDenom = getReg(1);
    setReg(0,signedNum / signedDenom);
    setReg(1,signedNum % signedDenom);
    setReg(3,unsignedNum / unsignedDenom);
    r[15]+=4;
}

void ARM7TDMI::ARMdataProcessing(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Data Proc=",r[15]);
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
            uint8_t rBit = instruction & 0x10; // if rm or rn == 15, and r == 1, r[15]+=12, else r[15]+=8
            uint8_t rm = instruction & 0xF;
            switch(rBit) {
                case 0: // register operand w/ immediate shift
                {
                    uint8_t Is = (instruction & 0xF80) >> 7;
                    if(pcIsRn || (rm == 15))
                        r[15]+=8;

                    op2 = ALUshift(getReg(rm),Is,shiftType,s,1);

                    if(pcIsRn || (rm == 15))
                        r[15]-=8;
                    break;
                }

                default: // register operand w/ register shift
                    uint8_t Rs = (instruction & 0xF00) >> 8;
                    if((rn == 15) || (rm == 15))
                        r[15]+=12;
                    op2 = ALUshift(getReg(rm),getReg(Rs) & 0xFF,shiftType,s,0);
                    if((rn == 15) || (rm == 15))
                        r[15]-=12;
                
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
        setCPSR(getBankedReg(mode,'S'));
        s = 0;
    }    

    if(pcIsRn)
        rn = r[15] + 8;
    else
        rn = getReg(rn);
    // good tip to remember: signed and unsigned integer ops produce the same binary
    uint32_t result;
    switch(opcode) {
        case 0x0: // AND
            result = rn & op2;
            setReg(rd,result);
            break;
        case 0x1: // EOR
            result = rn ^ op2;
            setReg(rd,result);
            break;
        case 0x2: // SUB
            result = sub(rn,op2,s);
            setReg(rd,result);
            break;
        case 0x3: // RSB
            result = sub(op2,rn,s);
            setReg(rd,result);
            break;
        case 0x4: // ADD
            result = add(rn,op2,s);
            setReg(rd,result);
            break;
        case 0x5: // ADC
            result = addCarry(rn,op2,s);
            setReg(rd,result);
            break;
        case 0x6: // SBC
            result = subCarry(rn,op2,1);
            setReg(rd,result);
            break;
        case 0x7: // RSC
            result = subCarry(op2,rn,1);
            setReg(rd,result);
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
            setReg(rd,result);
            break;
        case 0xD: // MOV
            result = op2;
            setReg(rd,result);
            break;
        case 0xE: // BIC
            result = rn & (~op2);
            setReg(rd,result);
            break;
        case 0xF: // MVN
            result = ~op2;
            setReg(rd,result);
    }

    if(s)
        setZeroAndSign(result);
    if(rd != 15)
        r[15]+=4;
}
void ARM7TDMI::ARMmultiplyAndMultiplyAccumulate(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Multiply and MulAcc=",r[15]);
    #endif
    uint8_t opcode = (instruction & 0x1E00000) >> 21;
    bool s = instruction & 0x100000;
    uint8_t rd = (instruction & 0xF0000) >> 16;
    uint8_t rn = (instruction & 0xF000) >> 12;
    uint8_t rs = (instruction & 0xF00) >> 8;
    uint8_t rm = instruction & 0xF;

    uint64_t rdValue = getReg(rd); // hi
    uint64_t rnValue = getReg(rn); // lo
    uint64_t rsValue = getReg(rs);
    uint64_t rmValue = getReg(rm);

    uint64_t result;
    switch(opcode) {
        case 0b0000: // MUL
            result = rmValue * rsValue;
            setReg(rd,result);
            break;
        case 0b0001: // MLA
            result = (rmValue * rsValue) + rnValue;
            setReg(rd,result);
            break;
        case 0b0100: // UMULL 
            result = rmValue * rsValue;
            setReg(rd,result >> 32);
            setReg(rn,result);
            break;
        case 0b0101: // UMLAL
            result = (rmValue * rsValue) + ((rdValue << 32) | rnValue);
            setReg(rd,result >> 32);
            setReg(rn,result);
            break;
        case 0b0110: // SMULL
            result = static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rmValue)) * static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rsValue));
            setReg(rd,result >> 32);
            setReg(rn,result);
            break;
        case 0b0111: // SMLAL
            int64_t hiLo = ((rdValue << 32) | rnValue);
            result = (static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rmValue)) * static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rsValue))) + hiLo;
            setReg(rd,result >> 32);
            setReg(rn,result);
            break;
    }

    if(s && (opcode & 0b0100))
        setZeroAndSign(result);
    else if(s)
        setZeroAndSign(*reinterpret_cast<uint32_t*>(&result));

    if(rd != 15)
        r[15]+=4;
}

void ARM7TDMI::ARMpsrTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X PSR Transfer=",r[15]);
    #endif
    if(((instruction & 0xF0000) != 0xF0000) && ((instruction & 0xF000) != 0xF000))
        return ARMswap(instruction);

    bool psr = instruction & 0x400000;
    bool msr = instruction & 0x200000;
    bool hasSpsr = (mode != System) && (mode != User);

    if(msr) {
        uint32_t op;
        switch(instruction & 0x2000000) {
            case 0: // Reg
                op = getReg(instruction & 0xF);
                break;
            default: // Imm
                uint32_t Imm = instruction & 0xFF;
                uint8_t rotate = (instruction & 0xF00) >> 8;
                op = ALUshift(Imm, rotate * 2, 0b11,1,0);
        }

        uint32_t mask = 0;
        // if bit is there, update the field
        if(instruction & 0x80000)
            mask |= 0xFF000000;
        if(instruction & 0x40000)
            mask |= 0x00FF0000;
        if(instruction & 0x20000)
            mask |= 0x0000FF00;
        if(instruction & 0x10000)
            mask |= 0x000000FF;

        op &= mask;

        switch(psr) {
            case 0:
                if(mode == User) // if in non-priviledged mode
                    mask &= 0xFFFFFF00;
                // if switching mode
                if(mask & 0x1F) {
                    bool oldState = state; // thumb must not be changed in CPSR, undefined behavior
                    switchMode(op & 0x1F);
                    setCPSR((getCPSR() & ~mask) | op);
                    state = oldState;
                } else {
                    setCPSR((getCPSR() & ~mask) | op);
                }
                break;
            default:
                if(hasSpsr)
                    setSPSR(mode,(getSPSR(mode) & ~mask) | op);
        }
    } else {
        uint8_t rd = (instruction & 0xF000) >> 12;
        switch(mode) {
            case 16:
            case 31:
                if(!psr)
                    setReg(rd, getCPSR());
                break;
            default:
                switch(psr) {
                    case 0:
                        setReg(rd, getCPSR());
                        break;
                    default:
                        if(hasSpsr)
                            setReg(rd, getSPSR(mode));
                }
        }
    }
    
    r[15]+=4;
}

void ARM7TDMI::ARMsingleDataTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Single Data Transfer=",r[15]);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    bool b = instruction & 0x400000;
    bool ldr = instruction & 0x100000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getReg(rn);

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
            offset = ALUshift(getReg(rm),Is,shiftType,0,1);
            
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
                    storeValue(getReg(rd),address);
                    break;
                default:
                    storeValue(static_cast<uint8_t>(getReg(rd)),address);
            }
            if(rd == 15)
                storeValue(static_cast<uint8_t>((*systemMemory)[address]+12),address);
            break;
        default: // LDR
                
            switch(b) {
                case 0:
                    setReg(rd, readWordRotate(address));
                    break;
                default:
                    setReg(rd, (*systemMemory)[address]);
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

    if(((instruction & 0x200000) || !p) && ((rd != rn) || !ldr)) // write-back address
        setReg(rn,address);

    if(rd != 15)
        r[15]+=4;
}
void ARM7TDMI::ARMhdsDataSTRH(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X STRH=",r[15]);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getReg(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getReg(instruction & 0xF);
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

    storeValue(static_cast<uint16_t>(getReg(rd)),address);

    if(rd == 15)
        storeValue(static_cast<uint8_t>((*systemMemory)[address]+12),address);

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
        setReg(rn,address);

    r[15]+=4;
}
void ARM7TDMI::ARMhdsDataLDRH(uint32_t instruction) { 
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRH=",r[15]);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getReg(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getReg(instruction & 0xF);
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

    setReg(rd,readHalfWordRotate(address));

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if(((instruction & 0x200000) || !p) && (rd != rn)) // write-back address
        setReg(rn,address);

    if(rd != 15)
        r[15]+=4;
}
void ARM7TDMI::ARMhdsDataLDRSB(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRSB=",r[15]);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getReg(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getReg(instruction & 0xF);
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

    setReg(rd,static_cast<int>(reinterpret_cast<int8_t&>((*systemMemory)[address])));

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if(((instruction & 0x200000) || !p) && (rd != rn)) // write-back address
        setReg(rn,address);

    if(rd != 15)
        r[15]+=4;
}
void ARM7TDMI::ARMhdsDataLDRSH(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRSH=",r[15]);
    #endif
    uint32_t offset;
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = getReg(rn);

    if(rn == 15)
        address += 8;

    switch(instruction & 0x400000) {
        case 0:
            offset = getReg(instruction & 0xF);
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

    setReg(rd,static_cast<int>(static_cast<int16_t>(readHalfWordRotate(address))));

    if(!p) {
        switch(u) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if(((instruction & 0x200000) || !p) && (rd != rn)) // write-back address
        setReg(rn,address);

    if(rd != 15)
        r[15]+=4;
}
void ARM7TDMI::ARMblockDataTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Block Data Transfer=",r[15]);
    #endif
    bool p = instruction & 0x1000000;
    bool u = instruction & 0x800000;
    bool s = instruction & 0x400000;
    bool w = instruction & 0x200000;
    bool l = instruction & 0x100000;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint16_t rListBinary = instruction & 0xFFFF;
    uint32_t address = getReg(rn);
    uint32_t baseAddress = address;
    bool rnInList = rListBinary & (1 << rn);
    bool rnFirstInList = 0;
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
            if(!rnFirstInList && rListVector.empty() && (i == rn))
                rnFirstInList = 1;
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
        if(l && pcInList) // LDM with R15 in transfer list
            setCPSR(getSPSR(mode));
        else { // R15 not in list
            oldMode = mode;
            switchMode(User);
        }
    }

    if(l) { // LDM

        for(uint8_t num : rListVector) {
            if(p)
                address += offset;
            setReg(num,readWord(address));
            if(!p)
                address += offset;
        }
    } else { // STM

        for(uint8_t num : rListVector) {
            
            if(p)
                address += offset;

            if(num == 15)
                storeValue(getReg(num),address+12);
            else
                storeValue(getReg(num),address);
            
            if(!p)
                address += offset;
        }

        if(rnFirstInList)
            storeValue(baseAddress,baseAddress);
    }

    if(oldMode)
        switchMode(oldMode);
    
    // if STM wb bit is set, write-back; don't if LDM and rn is in list
    if(w && !(l && rnInList)) 
        setReg(rn,address);
    
    if(!pcInList)
        r[15]+=4;
}
void ARM7TDMI::ARMswap(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Swap=",r[15]);
    #endif
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint8_t rm = instruction & 0xF;

    uint32_t rnValue = getReg(rn);

    // swap byte
    if(instruction & 0x400000) {
        uint32_t rnAddrValue = (*systemMemory)[rnValue];
        storeValue(static_cast<uint8_t>(getReg(rm)),rnValue);
        setReg(rd,rnAddrValue);
    } else { // swap word
        uint32_t rnAddrValue = readWordRotate(rnValue);
        storeValue(getReg(rm),rnValue);
        setReg(rd,rnAddrValue);
    }

    r[15]+=4;
}

void ARM7TDMI::THUMBmoveShiftedRegister(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move Shifted Reg=",r[15]);
    #endif
    uint8_t offset = (instruction & 0x7C0) >> 6;
    uint32_t rs = (instruction & 0x38) >> 3;
    uint8_t rd = instruction & 0x7;

    rs = getReg(rs);
    switch(instruction & 0x1800) {
        case 0x0000: // LSL
            setReg(rd,ALUshift(rs,offset,0,1,1));
            break;
        case 0x0800: // LSR
            setReg(rd,ALUshift(rs,offset,1,1,1));
            break;
        case 0x1000: // ASR
            setReg(rd,ALUshift(rs,offset,0b10,1,1));
            break;
    }

    setZeroAndSign(getReg(rd));
    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBaddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Subtract=",r[15]);
    #endif
    uint32_t rs = getReg((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;

    uint32_t result;
    switch(instruction & 0x600) {
        case 0x000: // add reg
        {
            uint32_t rn = getReg((instruction & 0x1C0) >> 6);
            result = add(rs,rn,true);
            setReg(rd,result);
            break;
        }
        case 0x200: // sub reg
        {
            uint32_t rn = getReg((instruction & 0x1C0) >> 6);
            result = sub(rs,rn,true);
            setReg(rd,result);
            break;
        }
        case 0x400: // add imm
            result = add(rs,(instruction & 0x1C0) >> 6,true);
            setReg(rd,result);
            break;
        case 0x600: // sub imm
            result = sub(rs,(instruction & 0x1C0) >> 6,true);
            setReg(rd,result);
    }

    setZeroAndSign(result);
    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBmoveCompareAddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move/Compare/Add/Subtract=",r[15]);
    #endif
    uint8_t rd = (instruction &  0x700) >> 8;
    uint8_t nn = instruction & 0xFF;
    uint32_t result;

    switch(instruction & 0x1800) {
        case 0x0000: // mov
            result = nn;
            setReg(rd,result);
            break;
        case 0x0800: // cmp
            result = sub(getReg(rd),nn,1);
            break;
        case 0x1000: // add
            result = add(getReg(rd),nn,1);
            setReg(rd,result);
            break;
        case 0x1800: // sub
            result = sub(getReg(rd),nn,1);
            setReg(rd,result);
            break;
    }

    setZeroAndSign(result);
    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBaluOperations(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X ALU Operation=",r[15]);
    #endif
    uint32_t rs = getReg((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;
    uint32_t rdValue = getReg(rd);

    uint32_t result;
    switch((instruction & 0x3C0) >> 6) {
        case 0x0: // AND
            result = rdValue & rs;
            setReg(rd,result);
            break;
        case 0x1: // EOR
            result = rdValue ^ rs;
            setReg(rd,result);
            break;
        case 0x2: // LSL
            result = ALUshift(rdValue,rs & 0xFF,0,1,0);
            setReg(rd,result);
            break;
        case 0x3: // LSR
            result = ALUshift(rdValue,rs & 0xFF,1,1,0);
            setReg(rd,result);
            break;
        case 0x4: // ASR
            result = ALUshift(rdValue,rs & 0xFF,2,1,0);
            setReg(rd,result);
            break;
        case 0x5: // ADC
            result = addCarry(rdValue,rs,1);
            setReg(rd,result);
            break;
        case 0x6: // SBC
            result = subCarry(rdValue,rs,1);
            setReg(rd,result);
            break;
        case 0x7: // ROR
            result = ALUshift(rdValue,rs & 0xFF,3,1,0);
            setReg(rd,result);
            break;
        case 0x8: // TST
            result = rdValue & rs;
            break;
        case 0x9: // NEG
            result = ~rs + 1;
            setReg(rd,result);
            break;
        case 0xA: // CMP
            result = sub(rdValue,rs,1);
            break;
        case 0xB: // CMN
            result = add(rdValue,rs,1);
            break;
        case 0xC: // ORR
            result = rdValue | rs;
            setReg(rd,result);
            break;
        case 0xD: // MUL
            result = rdValue * rs;
            setReg(rd,result);
            break;
        case 0xE: // BIC
            result = rdValue & (~rs);
            setReg(rd,result);
            break;
        case 0xF: // MVN
            result = ~rs;
            setReg(rd,result);
    }

    setZeroAndSign(result);
    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBhiRegOpsBranchEx(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Hi Register Operation=",r[15]);
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
    uint32_t rsValue = getReg(rs);
    uint32_t rdValue = getReg(rd);

    
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
            setReg(rd,add(rdValue,rsValue,0));
            break;
        case 0x100: // CMP
            sub(rdValue,rsValue,1);
            break;
        case 0x200: // MOV
            setReg(rd,rsValue);
            break;
        case 0x300: // BX
            if(!(rsValue & 1))
                state = 0;
            setReg(15,rsValue);
    }

    if((opcode != 0x300) && (rd != 15))
        r[15]+=2;
}

void ARM7TDMI::THUMBloadPCRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load r[15]-Relative=",r[15]);
    #endif
    uint8_t rd = (instruction & 0x700) >> 8;
    setReg(rd,readWord(((r[15]+4) & ~2) + (instruction & 0xFF) * 4));
    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreRegOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Reg Offset=",r[15]);
    #endif
    uint32_t rd = instruction & 0x7;
    uint32_t rb = getReg((instruction & 0x38) >> 3);
    uint32_t ro = getReg((instruction & 0x1C0) >> 6);

    switch(instruction & 0xC00) {
        case 0x000: // STR
            rd = getReg(rd);
            storeValue(rd,rb+ro);
            break;
        case 0x400: // STRB
            rd = getReg(rd);
            storeValue(*reinterpret_cast<uint8_t*>(&rd),rb+ro);
            break;
        case 0x800: // LDR
            setReg(rd,readWordRotate(rb+ro));
            break;
        case 0xC00: // LDRB
            setReg(rd,(*systemMemory)[rb+ro]);
    }

    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreSignExtendedByteHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Sign Extended Byte/Halfword=",r[15]);
    #endif
    uint32_t rd = instruction & 0x7;
    uint32_t rb = getReg((instruction & 0x38) >> 3);
    uint32_t ro = getReg((instruction & 0x1C0) >> 6);

    switch(instruction & 0xC00) {
        case 0x000: // STRH
            rd = getReg(rd);
            storeValue(*reinterpret_cast<uint16_t*>(&rd),rb+ro);
            break;
        case 0x400: // LDSB
            setReg(rd,static_cast<int8_t>((*systemMemory)[rb+ro]));
            break;
        case 0x800: // LDRH
            setReg(rd,readHalfWordRotate(rb+ro));
            break;
        case 0xC00: // LDSH
            setReg(rd,static_cast<int32_t>(static_cast<int16_t>(readHalfWord(rb+ro))));
    }

    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreImmOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Immediate Offset=",r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = getReg((instruction & 0x38) >> 3);
    uint32_t nn = (instruction & 0x7C0) >> 6;

    switch(instruction & 0x1800) {
        case 0x0000: // STR
            storeValue(getReg(rd),rb+(nn << 2));
            break;
        case 0x0800: // LDR
            setReg(rd,readWordRotate(rb+(nn << 2)));
            break;
        case 0x1000: // STRB
            storeValue(static_cast<uint8_t>(getReg(rd)),rb+nn);
            break;
        case 0x1800: // LDRB
            setReg(rd,(*systemMemory)[rb+nn]);
    }

    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Halfword=",r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = getReg((instruction & 0x38) >> 3);
    uint32_t nn = (instruction & 0x7C0) >> 5;

    switch(instruction & 0x800) {
        case 0x000: // STRH
            storeValue(static_cast<uint16_t>(getReg(rd)),rb+nn);
            break;
        case 0x800: // LDRH
            setReg(rd,readHalfWordRotate(rb+nn));
    }

    if(rd != 15)
        r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreSPRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store SP Relative=",r[15]);
    #endif
    uint8_t rd = (instruction & 0x700) >> 8;
    uint16_t nn = (instruction & 0xFF) << 2;

    if(instruction & 0x800) { // LDR
        setReg(rd,readWordRotate(getReg(13)+nn));
    } else { // STR
        storeValue(getReg(rd),getReg(13)+nn);
    }

    if(rd != 15)
        r[15]+=2;
}

void ARM7TDMI::THUMBgetRelativeAddress(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Get Relative Address=",r[15]);
    #endif
    uint16_t nn = (instruction & 0xFF) << 2;
    uint8_t rd = (instruction & 0x700) >> 8;
    if(instruction & 0x800) // ADD SP
        setReg(rd,getReg(13) + nn);
    else // ADD PC
        setReg(rd,((r[15]+4) & ~2) + nn);
    if(rd != 15)
        r[15]+=2;
}

void ARM7TDMI::THUMBaddOffsetToSP(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Offset To SP=",r[15]);
    #endif
    uint16_t nn = (instruction & 0x7F) << 2;
    if(instruction & 0x80) // - offset
        setReg(13,getReg(13) - nn);
    else // + offset
        setReg(13,getReg(13) + nn);;
    r[15]+=2;
}

void ARM7TDMI::THUMBpushPopRegisters(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Push/Pop Registers=",r[15]);
    #endif
    uint8_t rListBinary = instruction & 0xFF;
    uint8_t rListOffset;
    bool pclrBit = instruction & 0x100;
    bool pop = instruction & 0x800;
    uint32_t address = getReg(13);

    if(pop) { // POP, AKA load from memory
        

        rListOffset = 1;
        for(uint8_t i = 0; i < 8; i++) {
            if(instruction & rListOffset) {
                setReg(i,readWord(address));
                address+=4;
            }
            rListOffset <<= 1;
        }

        if(pclrBit) {
            setReg(15,readWord(address));
            address+=4;
            
        }
    } else { // PUSH, AKA store to memory
        
        if(pclrBit) {
            address-=4;
            storeValue(getReg(14),address);
        }
        
        rListOffset = 0x80;
        for(int8_t i = 7; i > -1; i--) {
            if(instruction & rListOffset) {
                address-=4;
                storeValue(getReg(i),address);
            }
            rListOffset >>= 1;
        }
    }

    setReg(13,address);
    if(!pclrBit || !pop)
        r[15]+=2;
}
void ARM7TDMI::THUMBmultipleLoadStore(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Multiple Load Store=",r[15]);
    #endif
    uint8_t rListBinary = instruction & 0xFF;
    uint8_t rListOffset = 1;
    uint8_t rb = (instruction & 0x700) >> 8;
    uint32_t address = getReg(rb);

    if(!rListBinary) { // todo: handle empty rList
        
        address += 0x40;
    } else {
        if(instruction & 0x800) { // LDMIA
            
            for(uint8_t i = 0; i < 8; i++) {
                if(instruction & rListOffset) {
                    setReg(i, readWord(address));
                    address += 4;
                    
                }
                rListOffset <<= 1;
            }
        } else { // STMIA
            
            for(uint8_t i = 0; i < 8; i++) {
                if(instruction & rListOffset) {
                    storeValue(getReg(i), address);
                    address += 4;    
                }
                rListOffset <<= 1;
            }
        }
    }
    setReg(rb, address);
    r[15]+=2;
}

void ARM7TDMI::THUMBconditionalBranch(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Conditional Branch=",r[15]);
    #endif
    int16_t offset = static_cast<int8_t>(instruction & 0xFF) * 2 + 4;
    bool condMet = false;
    
    condMet = checkCond((instruction & 0xF00) << 20);
    if(condMet)
        r[15] += offset;
    else
        r[15]+=2;
}
void ARM7TDMI::THUMBunconditionalBranch(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Unconditional Branch=",r[15]);
    #endif
    int16_t offset = static_cast<int16_t>((instruction & 0x7FF) << 5) >> 4;
    r[15] += 4 + offset;
}
void ARM7TDMI::THUMBlongBranchWithLink(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Long Branch With Link=",r[15]);
    #endif
    int32_t offset = instruction & 0x7FF;
    bool instrTwo = instruction & 0x800;
    
    if(instrTwo) { // instr 2
        uint32_t oldPC = (r[15] + 2) | 1;
        r[15]=getReg(14)+(offset << 1);
        setReg(14,oldPC);
    } else { // instr 1
        offset = (offset << 21) >> 9;
        setReg(14,r[15]+4+offset);
    }
    
    if(!instrTwo)
        r[15]+=2;
}
void ARM7TDMI::THUMBsoftwareInterrupt(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%XTHUMB Software Interrupt=",r[15]);
    #endif
    //handleException(SoftwareInterrupt,2,Supervisor);
    // stub for Div
    int32_t signedNum = static_cast<int32_t>(getReg(0));
    int32_t signedDenom = static_cast<int32_t>(getReg(1));
    uint32_t unsignedNum = getReg(0);
    uint32_t unsignedDenom = getReg(1);
    setReg(0,signedNum / signedDenom);
    setReg(1,signedNum % signedDenom);
    setReg(3,unsignedNum / unsignedDenom);
    r[15]+=2;
}
