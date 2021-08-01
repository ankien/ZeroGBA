#include "ARM7TDMI.hpp"

void ARM7TDMI::THUMBmoveShiftedRegister(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move Shifted Reg=",cpuState.r[15]);
    #endif
    uint8_t offset = (instruction & 0x7C0) >> 6;
    uint32_t rs = (instruction & 0x38) >> 3;
    uint8_t rd = instruction & 0x7;

    rs = cpuState.getReg(rs);
    switch(instruction & 0x1800) {
        case 0x0000: // LSL
            cpuState.setReg(rd,ALUshift(rs,offset,0,1,1));
            break;
        case 0x0800: // LSR
            cpuState.setReg(rd,ALUshift(rs,offset,1,1,1));
            break;
        case 0x1000: // ASR
            cpuState.setReg(rd,ALUshift(rs,offset,0b10,1,1));
            break;
    }

    setZeroAndSign(cpuState.getReg(rd));
    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBaddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Subtract=",cpuState.r[15]);
    #endif
    uint32_t rs = cpuState.getReg((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;

    uint32_t result;
    switch(instruction & 0x600) {
        case 0x000: // add reg
        {
            uint32_t rn = cpuState.getReg((instruction & 0x1C0) >> 6);
            result = add(rs,rn,true);
            cpuState.setReg(rd,result);
            break;
        }
        case 0x200: // sub reg
        {
            uint32_t rn = cpuState.getReg((instruction & 0x1C0) >> 6);
            result = sub(rs,rn,true);
            cpuState.setReg(rd,result);
            break;
        }
        case 0x400: // add imm
            result = add(rs,(instruction & 0x1C0) >> 6,true);
            cpuState.setReg(rd,result);
            break;
        case 0x600: // sub imm
            result = sub(rs,(instruction & 0x1C0) >> 6,true);
            cpuState.setReg(rd,result);
    }

    setZeroAndSign(result);
    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBmoveCompareAddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move/Compare/Add/Subtract=",cpuState.r[15]);
    #endif
    uint8_t rd = (instruction &  0x700) >> 8;
    uint8_t nn = instruction & 0xFF;
    uint32_t result;

    switch(instruction & 0x1800) {
        case 0x0000: // mov
            result = nn;
            cpuState.setReg(rd,result);
            break;
        case 0x0800: // cmp
            result = sub(cpuState.getReg(rd),nn,1);
            break;
        case 0x1000: // add
            result = add(cpuState.getReg(rd),nn,1);
            cpuState.setReg(rd,result);
            break;
        case 0x1800: // sub
            result = sub(cpuState.getReg(rd),nn,1);
            cpuState.setReg(rd,result);
            break;
    }

    setZeroAndSign(result);
    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBaluOperations(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X ALU Operation=",cpuState.r[15]);
    #endif
    uint32_t rs = cpuState.getReg((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;
    uint32_t rdValue = cpuState.getReg(rd);
    bool oldCarry = cpuState.carryFlag;

    uint32_t result;
    switch((instruction & 0x3C0) >> 6) {
        case 0x0: // AND
            result = rdValue & rs;
            cpuState.setReg(rd,result);
            break;
        case 0x1: // EOR
            result = rdValue ^ rs;
            cpuState.setReg(rd,result);
            break;
        case 0x2: // LSL
            result = ALUshift(rdValue,rs & 0xFF,0,1,0);
            cpuState.setReg(rd,result);
            break;
        case 0x3: // LSR
            result = ALUshift(rdValue,rs & 0xFF,1,1,0);
            cpuState.setReg(rd,result);
            break;
        case 0x4: // ASR
            result = ALUshift(rdValue,rs & 0xFF,2,1,0);
            cpuState.setReg(rd,result);
            break;
        case 0x5: // ADC
            result = addCarry(rdValue,rs,1,oldCarry);
            cpuState.setReg(rd,result);
            break;
        case 0x6: // SBC
            result = addCarry(rdValue,~rs,1,oldCarry);
            cpuState.setReg(rd,result);
            break;
        case 0x7: // ROR
            result = ALUshift(rdValue,rs & 0xFF,3,1,0);
            cpuState.setReg(rd,result);
            break;
        case 0x8: // TST
            result = rdValue & rs;
            break;
        case 0x9: // NEG
            result = sub(0,rs,1);
            cpuState.setReg(rd,result);
            break;
        case 0xA: // CMP
            result = sub(rdValue,rs,1);
            break;
        case 0xB: // CMN
            result = add(rdValue,rs,1);
            break;
        case 0xC: // ORR
            result = rdValue | rs;
            cpuState.setReg(rd,result);
            break;
        case 0xD: // MUL
            result = rdValue * rs;
            cpuState.setReg(rd,result);
            break;
        case 0xE: // BIC
            result = rdValue & (~rs);
            cpuState.setReg(rd,result);
            break;
        case 0xF: // MVN
            result = ~rs;
            cpuState.setReg(rd,result);
    }

    setZeroAndSign(result);
    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBhiRegOpsBranchEx(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Hi Register Operation=",cpuState.r[15]);
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
    uint32_t rsValue = cpuState.getReg(rs);
    uint32_t rdValue = cpuState.getReg(rd);

    
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
            cpuState.setReg(rd,add(rdValue,rsValue,0));
            break;
        case 0x100: // CMP
            setZeroAndSign(sub(rdValue,rsValue,1));
            break;
        case 0x200: // MOV
            cpuState.setReg(rd,rsValue);
            break;
        case 0x300: // BX
            if(!(rsValue & 1))
                cpuState.state = 0;
            cpuState.setReg(15,rsValue);
    }

    if((opcode != 0x300) && (rd != 15))
        cpuState.r[15]+=2;
}

void ARM7TDMI::THUMBloadPCRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load cpuState.r[15]-Relative=",cpuState.r[15]);
    #endif
    uint8_t rd = (instruction & 0x700) >> 8;
    cpuState.setReg(rd,systemMemory->readWord(((cpuState.r[15]+4) & ~2) + (instruction & 0xFF) * 4));
    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreRegOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Reg Offset=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);
    uint32_t ro = cpuState.getReg((instruction & 0x1C0) >> 6);

    switch(instruction & 0xC00) {
        case 0x000: // STR
            systemMemory->storeValue(cpuState.getReg(rd),rb+ro);
            break;
        case 0x400: // STRB
            systemMemory->storeValue(static_cast<uint8_t>(cpuState.getReg(rd)),rb+ro);
            break;
        case 0x800: // LDR
            cpuState.setReg(rd,systemMemory->readWordRotate(rb+ro));
            break;
        case 0xC00: // LDRB
            cpuState.setReg(rd,systemMemory->readByte(rb+ro));
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreSignExtendedByteHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Sign Extended Byte/Halfword=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);
    uint32_t ro = cpuState.getReg((instruction & 0x1C0) >> 6);

    switch(instruction & 0xC00) {
        case 0x000: // STRH
            systemMemory->storeValue(static_cast<uint16_t>(cpuState.getReg(rd)),rb+ro);
            break;
        case 0x400: // LDSB
            cpuState.setReg(rd,static_cast<int8_t>(systemMemory->readByte(rb+ro)));
            break;
        case 0x800: // LDRH
            cpuState.setReg(rd,systemMemory->readHalfWordRotate(rb+ro));
            break;
        case 0xC00: // LDSH
        {
            uint32_t address = rb+ro;
            uint8_t shiftAmount = ((address & 1) << 3) & 0x1F;
            int16_t value = systemMemory->readHalfWord(address);
            value = (value >> shiftAmount) | (value << (32 - shiftAmount));
            cpuState.setReg(rd,static_cast<int32_t>(value));
        }
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreImmOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Immediate Offset=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);
    uint32_t nn = (instruction & 0x7C0) >> 6;

    switch(instruction & 0x1800) {
        case 0x0000: // STR
            systemMemory->storeValue(cpuState.getReg(rd),rb+(nn << 2));
            break;
        case 0x0800: // LDR
            cpuState.setReg(rd,systemMemory->readWordRotate(rb+(nn << 2)));
            break;
        case 0x1000: // STRB
            systemMemory->storeValue(static_cast<uint8_t>(cpuState.getReg(rd)),rb+nn);
            break;
        case 0x1800: // LDRB
            cpuState.setReg(rd,systemMemory->readByte(rb+nn));
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Halfword=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);
    uint32_t nn = (instruction & 0x7C0) >> 5;

    switch(instruction & 0x800) {
        case 0x000: // STRH
            systemMemory->storeValue(static_cast<uint16_t>(cpuState.getReg(rd)),rb+nn);
            break;
        case 0x800: // LDRH
            cpuState.setReg(rd,systemMemory->readHalfWordRotate(rb+nn));
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBloadStoreSPRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store SP Relative=",cpuState.r[15]);
    #endif
    uint8_t rd = (instruction & 0x700) >> 8;
    uint16_t nn = (instruction & 0xFF) << 2;

    if(instruction & 0x800) { // LDR
        cpuState.setReg(rd,systemMemory->readWordRotate(cpuState.getReg(13)+nn));
    } else { // STR
        systemMemory->storeValue(cpuState.getReg(rd),cpuState.getReg(13)+nn);
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}

void ARM7TDMI::THUMBgetRelativeAddress(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Get Relative Address=",cpuState.r[15]);
    #endif
    uint16_t nn = (instruction & 0xFF) << 2;
    uint8_t rd = (instruction & 0x700) >> 8;
    if(instruction & 0x800) // ADD SP
        cpuState.setReg(rd,cpuState.getReg(13) + nn);
    else // ADD PC
        cpuState.setReg(rd,((cpuState.r[15]+4) & ~2) + nn);
    if(rd != 15)
        cpuState.r[15]+=2;
}

void ARM7TDMI::THUMBaddOffsetToSP(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Offset To SP=",cpuState.r[15]);
    #endif
    uint16_t nn = (instruction & 0x7F) << 2;
    if(instruction & 0x80) // - offset
        cpuState.setReg(13,cpuState.getReg(13) - nn);
    else // + offset
        cpuState.setReg(13,cpuState.getReg(13) + nn);;
    cpuState.r[15]+=2;
}

void ARM7TDMI::THUMBpushPopRegisters(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Push/Pop Registers=",cpuState.r[15]);
    #endif
    uint8_t rListBinary = instruction & 0xFF;
    uint8_t rListOffset;
    bool pclrBit = instruction & 0x100;
    bool pop = instruction & 0x800;
    uint32_t address = cpuState.getReg(13);

    if(pop) { // POP, AKA load from memory
        

        rListOffset = 1;
        for(int8_t i = 0; i < 8; i++) {
            if(instruction & rListOffset) {
                cpuState.setReg(i,systemMemory->readWord(address));
                address+=4;
            }
            rListOffset <<= 1;
        }

        if(pclrBit) {
            cpuState.setReg(15,systemMemory->readWord(address));
            address+=4;
            
        }
    } else { // PUSH, AKA store to memory
        
        if(pclrBit) {
            address-=4;
            systemMemory->storeValue(cpuState.getReg(14),address);
        }
        
        rListOffset = 0x80;
        for(int8_t i = 7; i > -1; i--) {
            if(instruction & rListOffset) {
                address-=4;
                systemMemory->storeValue(cpuState.getReg(i),address);
            }
            rListOffset >>= 1;
        }
    }

    cpuState.setReg(13,address);
    if(!pclrBit || !pop)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBmultipleLoadStore(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Multiple Load Store=",cpuState.r[15]);
    #endif
    uint8_t rListBinary = instruction & 0xFF;
    uint8_t rb = (instruction & 0x700) >> 8;
    uint32_t address = cpuState.getReg(rb);
    
    bool ldm = instruction & 0x800;

    if(!rListBinary) {
        if(ldm)
            cpuState.setReg(15,systemMemory->readWord(address));
        else
            systemMemory->storeValue(cpuState.r[15]+6,address);
        address += 0x40;
    } else {
        uint8_t rListOffset = 1;
        uint8_t rbInList = rListBinary & (1 << rb);
        bool rbFirstInList = (rListBinary & (rbInList - 1)) ? 0 : 1;
        uint32_t baseAddress = address;
        uint32_t rbStoreLoc;

        if(ldm) { // LDMIA
            
            for(uint8_t i = 0; i < 8; i++) {
                if(rListBinary & rListOffset) {
                    cpuState.setReg(i, systemMemory->readWord(address));
                    address += 4;
                    
                }
                rListOffset <<= 1;
            }
        } else { // STMIA
            
            for(uint8_t i = 0; i < 8; i++) {
                if(rListBinary & rListOffset) {
                    if(i == rb) {
                        systemMemory->storeValue(baseAddress, address);
                        rbStoreLoc = address;
                    } else
                        systemMemory->storeValue(cpuState.getReg(i), address);
                    address += 4;    
                }
                rListOffset <<= 1;
            }
            
            if(!rbFirstInList && rbInList)
                systemMemory->storeValue(address,rbStoreLoc);
        }
    }

    cpuState.setReg(rb,address);
    if(rListBinary || !ldm)
        cpuState.r[15]+=2;
}

void ARM7TDMI::THUMBconditionalBranch(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Conditional Branch=",cpuState.r[15]);
    #endif
    int16_t offset = static_cast<int8_t>(instruction & 0xFF) * 2 + 4;
    bool condMet = false;
    
    condMet = checkCond((instruction & 0xF00) << 20);
    if(condMet)
        cpuState.r[15] += offset;
    else
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBunconditionalBranch(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Unconditional Branch=",cpuState.r[15]);
    #endif
    int16_t offset = static_cast<int16_t>((instruction & 0x7FF) << 5) >> 4;
    cpuState.r[15] += 4 + offset;
}
void ARM7TDMI::THUMBlongBranchWithLink(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Long Branch With Link=",cpuState.r[15]);
    #endif
    int32_t offset = instruction & 0x7FF;
    bool instrTwo = instruction & 0x800;
    
    if(instrTwo) { // instr 2
        uint32_t oldPC = (cpuState.r[15] + 2) | 1;
        cpuState.r[15]=cpuState.getReg(14)+(offset << 1);
        cpuState.setReg(14,oldPC);
    } else { // instr 1
        offset = (offset << 21) >> 9;
        cpuState.setReg(14,cpuState.r[15]+4+offset);
    }
    
    if(!instrTwo)
        cpuState.r[15]+=2;
}
void ARM7TDMI::THUMBsoftwareInterrupt(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%XTHUMB Software Interrupt=",cpuState.r[15]);
    #endif
    cpuState.handleException(SoftwareInterrupt,2,Supervisor);
}
