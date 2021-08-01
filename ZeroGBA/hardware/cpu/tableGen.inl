#pragma once

void ARM7TDMI::ARMundefinedInstruction(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undefined Instruction=",cpuState.r[15]);
    #endif
    cpuState.r[15]+=4;
}

void ARM7TDMI::ARMemptyInstruction(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undecoded Instruction=",cpuState.r[15]);
    #endif
    cpuState.r[15]+=4;
}

void ARM7TDMI::THUMBemptyInstruction(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Undecoded Instruction=",cpuState.r[15]);
    #endif
    cpuState.r[15]+=2;
}

template<bool branchLink>
void ARM7TDMI::ARMbranch(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Branch=",cpuState.r[15]);
    #endif
    int32_t signedOffset = signExtend<int32_t,24>(instruction & 0xFFFFFF) * 4;
    if(branchLink)
        cpuState.setReg(14,cpuState.r[15]+4);
    cpuState.r[15] += 8 + signedOffset;
}

void ARM7TDMI::ARMbranchExchange(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Branch Exchange=",cpuState.r[15]);
    #endif
    uint32_t rn = instruction & 0xF;
    if(rn == 15)
        rn = cpuState.r[15]+8;
    else
        rn = cpuState.getReg(rn);
    if(rn & 1) {
        cpuState.state = 1;
        rn--;
    }
    cpuState.r[15] = rn;
}

void ARM7TDMI::ARMsoftwareInterrupt(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X SWI=",cpuState.r[15]);
    #endif
    cpuState.handleException(SoftwareInterrupt,4,Supervisor);
}

template<bool immediate, uint8_t opcode, bool condCode, bool shiftByReg>
void ARM7TDMI::ARMdataProcessing(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Data Proc=",cpuState.r[15]);
    #endif
    uint32_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t op2;
    bool oldCarry = cpuState.carryFlag;
    bool voidRd = 0;
    bool pcIsRn = rn == 15;

    // shifting
    switch(immediate) {

        case 0: // register is second operand
        {
            uint8_t shiftType = (instruction & 0x60) >> 5;
            uint8_t rm = instruction & 0xF;
            // if rm or rn == 15, and r == 1, cpuState.r[15]+=12, else cpuState.r[15]+=8
            switch(shiftByReg) {
                case 0: // register operand w/ immediate shift
                {
                    uint8_t Is = (instruction & 0xF80) >> 7;
                    if(rm == 15)
                        cpuState.r[15]+=8;

                    op2 = ALUshift(cpuState.getReg(rm),Is,shiftType,condCode,1);

                    if(rm == 15)
                        cpuState.r[15]-=8;
                    break;
                }

                default: // register operand w/ register shift
                    uint8_t Rs = (instruction & 0xF00) >> 8;
                    if(rm == 15)
                        cpuState.r[15]+=12;
                    op2 = ALUshift(cpuState.getReg(rm),cpuState.getReg(Rs) & 0xFF,shiftType,condCode,0);
                    if(rm == 15)
                        cpuState.r[15]-=12;
                
            }
            break;

        }

        default: // immediate is second operand
            uint32_t Imm = instruction & 0xFF;
            uint8_t rotate = (instruction & 0xF00) >> 8;
            op2 = ALUshift(Imm,rotate*2,0b11,condCode,0);
            break;
    }

    if(pcIsRn) {
        if(!immediate && (instruction & 0x10))
            rn = cpuState.r[15] + 12;
        else
            rn = cpuState.r[15] + 8;
    } else
        rn = cpuState.getReg(rn);
    // good tip to remember: signed and unsigned integer ops produce the same binary
    uint32_t result;
    switch(opcode) {
        case 0x0: // AND
            result = rn & op2;
            cpuState.setReg(rd,result);
            break;
        case 0x1: // EOR
            result = rn ^ op2;
            cpuState.setReg(rd,result);
            break;
        case 0x2: // SUB
            result = sub(rn,op2,condCode);
            cpuState.setReg(rd,result);
            break;
        case 0x3: // RSB
            result = sub(op2,rn,condCode);
            cpuState.setReg(rd,result);
            break;
        case 0x4: // ADD
            result = add(rn,op2,condCode);
            cpuState.setReg(rd,result);
            break;
        case 0x5: // ADC
            result = addCarry(rn,op2,condCode,oldCarry);
            cpuState.setReg(rd,result);
            break;
        case 0x6: // SBC
            result = addCarry(rn,~op2,1,oldCarry);
            cpuState.setReg(rd,result);
            break;
        case 0x7: // RSC
            result = addCarry(op2,~rn,1,oldCarry);
            cpuState.setReg(rd,result);
            break;
        case 0x8: // TST
            voidRd = true;
            result = rn & op2;
            break;
        case 0x9: // TEQ
            voidRd = true;
            result = rn ^ op2;
            break;
        case 0xA: // CMP
            voidRd = true;
            result = sub(rn,op2,condCode);
            break;
        case 0xB: // CMN
            voidRd = true;
            result = add(rn,op2,condCode);
            break;
        case 0xC: // ORR
            result = rn | op2;
            cpuState.setReg(rd,result);
            break;
        case 0xD: // MOV
            result = op2;
            cpuState.setReg(rd,result);
            break;
        case 0xE: // BIC
            result = rn & (~op2);
            cpuState.setReg(rd,result);
            break;
        case 0xF: // MVN
            result = ~op2;
            cpuState.setReg(rd,result);
    }
    
    if(condCode)
        setZeroAndSign(result);
    
    if((condCode) && (rd == 15)) {
        if(cpuState.mode == IRQ && cpuState.r[15] >> 24 == 0x00)
            systemMemory->stateRelativeToBios = systemMemory->AfterIRQ;
        else if(cpuState.mode == Supervisor && cpuState.r[15] >> 24 == 0x00)
            systemMemory->stateRelativeToBios = systemMemory->AfterSWI;
        uint32_t spsrVal = cpuState.getBankedReg(cpuState.mode,'S');
        cpuState.switchMode(spsrVal & 0x1F);
        cpuState.setCPSR(spsrVal);
    }

    if(rd != 15 || voidRd)
        cpuState.r[15]+=4;
}

template<uint8_t opcode, bool condCode>
void ARM7TDMI::ARMmultiplyAndMultiplyAccumulate(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Multiply and MulAcc=",cpuState.r[15]);
    #endif
    uint8_t rd = (instruction & 0xF0000) >> 16;
    uint8_t rn = (instruction & 0xF000) >> 12;
    uint8_t rs = (instruction & 0xF00) >> 8;
    uint8_t rm = instruction & 0xF;

    uint64_t rdValue = cpuState.getReg(rd); // hi
    uint64_t rnValue = cpuState.getReg(rn); // lo
    uint64_t rsValue = cpuState.getReg(rs);
    uint64_t rmValue = cpuState.getReg(rm);

    uint64_t result;
    switch(opcode) {
        case 0b0000: // MUL
            result = rmValue * rsValue;
            cpuState.setReg(rd,result);
            break;
        case 0b0001: // MLA
            result = (rmValue * rsValue) + rnValue;
            cpuState.setReg(rd,result);
            break;
        case 0b0100: // UMULL 
            result = rmValue * rsValue;
            cpuState.setReg(rd,result >> 32);
            cpuState.setReg(rn,result);
            break;
        case 0b0101: // UMLAL
            result = (rmValue * rsValue) + ((rdValue << 32) | rnValue);
            cpuState.setReg(rd,result >> 32);
            cpuState.setReg(rn,result);
            break;
        case 0b0110: // SMULL
            result = static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rmValue)) * static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rsValue));
            cpuState.setReg(rd,result >> 32);
            cpuState.setReg(rn,result);
            break;
        case 0b0111: // SMLAL
            int64_t hiLo = ((rdValue << 32) | rnValue);
            result = (static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rmValue)) * static_cast<int64_t>(*reinterpret_cast<int32_t*>(&rsValue))) + hiLo;
            cpuState.setReg(rd,result >> 32);
            cpuState.setReg(rn,result);
            break;
    }

    if(condCode && (opcode & 0b0100))
        setZeroAndSign(result);
    else if(condCode)
        setZeroAndSign(*reinterpret_cast<uint32_t*>(&result));

    if(rd != 15)
        cpuState.r[15]+=4;
}

template<bool immediate, bool psr, bool msr>
void ARM7TDMI::ARMpsrTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X PSR Transfer=",cpuState.r[15]);
    #endif
    if(((instruction & 0xF0000) != 0xF0000) && ((instruction & 0xF000) != 0xF000))
        return ARMswap<psr>(instruction);

    bool hasSpsr = (cpuState.mode != System) && (cpuState.mode != User);

    if(msr) {
        uint32_t op;
        switch(immediate) {
            case 0: // Reg
                op = cpuState.getReg(instruction & 0xF);
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
                if(cpuState.mode == User) // if in non-priviledged mode
                    mask &= 0xFFFFFF00;
                // if switching mode
                if(mask & 0x1F) {
                    bool oldState = cpuState.state; // thumb must not be changed in CPSR, undefined behavior
                    cpuState.switchMode(op & 0x1F);
                    cpuState.setCPSR((cpuState.getCPSR() & ~mask) | op);
                    cpuState.state = oldState;
                } else {
                    cpuState.setCPSR((cpuState.getCPSR() & ~mask) | op);
                }
                break;
            default:
                if(hasSpsr)
                    cpuState.setSPSR(cpuState.mode,(cpuState.getSPSR(cpuState.mode) & ~mask) | op);
        }
    } else {
        uint8_t rd = (instruction & 0xF000) >> 12;
        switch(cpuState.mode) {
            case 16:
            case 31:
                cpuState.setReg(rd, cpuState.getCPSR());
                break;
            default:
                switch(psr) {
                    case 0:
                        cpuState.setReg(rd, cpuState.getCPSR());
                        break;
                    default:
                        cpuState.setReg(rd, cpuState.getSPSR(cpuState.mode));
                }
        }
    }
    
    cpuState.r[15]+=4;
}

template<bool immediateOffset, bool prePost, bool upDown, bool byteWord, bool writeBack, bool loadStore, uint8_t shiftType>
void ARM7TDMI::ARMsingleDataTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Single Data Transfer=",cpuState.r[15]);
    #endif
    uint32_t offset;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = cpuState.getReg(rn);

    if(rn == 15)
        address += 8;

    switch(immediateOffset) {        
        case 0: // immediate offset
            offset = instruction & 0xFFF;
            break;
        default: // register offset
            uint8_t Is = (instruction & 0xF80) >> 7;
            uint8_t rm = instruction & 0xF;
            offset = ALUshift(cpuState.getReg(rm),Is,shiftType,0,1);
            
    }

    if(prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    switch(loadStore) {
        case 0: // STR
            
            switch(byteWord) {
                case 0:
                    systemMemory->storeValue(cpuState.getReg(rd),address);
                    break;
                default:
                    systemMemory->storeValue(static_cast<uint8_t>(cpuState.getReg(rd)),address);
            }
            if(rd == 15)
                systemMemory->storeValue(cpuState.r[15]+12,address);
            break;
        default: // LDR
                
            switch(byteWord) {
                case 0:
                    cpuState.setReg(rd, systemMemory->readWordRotate(address));
                    break;
                default:
                    cpuState.setReg(rd, systemMemory->readByte(address));
            }
    }

    if(!prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((writeBack || !prePost) && ((rd != rn) || !loadStore)) // write-back address
        cpuState.setReg(rn,address);

    if(rd != 15 || !loadStore)
        cpuState.r[15]+=4;
}

template<bool prePost, bool upDown, bool immediateOffset, bool writeBack>
void ARM7TDMI::ARMhdsDataSTRH(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X STRH=",cpuState.r[15]);
    #endif
    uint32_t offset;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = cpuState.getReg(rn);

    if(rn == 15)
        address += 8;

    switch(immediateOffset) {
        case 0:
            offset = cpuState.getReg(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    systemMemory->storeValue(static_cast<uint16_t>(cpuState.getReg(rd)),address);

    if(rd == 15)
        systemMemory->storeValue(cpuState.r[15]+12,address);

    if(!prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if(writeBack || !prePost) // write-back address
        cpuState.setReg(rn,address);

    cpuState.r[15]+=4;
}

template<bool prePost, bool upDown, bool immediateOffset, bool writeBack>
void ARM7TDMI::ARMhdsDataLDRH(uint32_t instruction) { 
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRH=",cpuState.r[15]);
    #endif
    uint32_t offset;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = cpuState.getReg(rn);

    if(rn == 15)
        address += 8;

    switch(immediateOffset) {
        case 0:
            offset = cpuState.getReg(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    cpuState.setReg(rd,systemMemory->readHalfWordRotate(address));

    if(!prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((writeBack || !prePost) && (rd != rn)) // write-back address
        cpuState.setReg(rn,address);

    if(rd != 15)
        cpuState.r[15]+=4;
}

template<bool prePost, bool upDown, bool immediateOffset, bool writeBack>
void ARM7TDMI::ARMhdsDataLDRSB(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRSB=",cpuState.r[15]);
    #endif
    uint32_t offset;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = cpuState.getReg(rn);

    if(rn == 15)
        address += 8;

    switch(immediateOffset) {
        case 0:
            offset = cpuState.getReg(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    cpuState.setReg(rd,static_cast<int32_t>((int8_t)systemMemory->readByte(address)));

    if(!prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((writeBack || !prePost) && (rd != rn)) // write-back address
        cpuState.setReg(rn,address);

    if(rd != 15)
        cpuState.r[15]+=4;
}

template<bool prePost, bool upDown, bool immediateOffset, bool writeBack>
void ARM7TDMI::ARMhdsDataLDRSH(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X LDRSH=",cpuState.r[15]);
    #endif
    uint32_t offset;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t address = cpuState.getReg(rn);

    if(rn == 15)
        address += 8;

    switch(immediateOffset) {
        case 0:
            offset = cpuState.getReg(instruction & 0xF);
            break;
        default:
            offset = ((instruction & 0xF00) >> 4) | (instruction & 0xF);
    }

    if(prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    uint8_t shiftAmount = ((address & 1) << 3) & 0x1F;
    int16_t value = systemMemory->readHalfWord(address);
    value = (value >> shiftAmount) | (value << (32 - shiftAmount));
    cpuState.setReg(rd,static_cast<int32_t>(value));

    if(!prePost) {
        switch(upDown) {
            case 0:
                address -= offset;
                break;
            default:
                address += offset;
        }
    }

    if((writeBack || !prePost) && (rd != rn)) // write-back address
        cpuState.setReg(rn,address);

    if(rd != 15)
        cpuState.r[15]+=4;
}

template<bool prePost, bool upDown, bool psr, bool writeBack, bool loadStore>
void ARM7TDMI::ARMblockDataTransfer(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Block Data Transfer=",cpuState.r[15]);
    #endif
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint16_t rListBinary = instruction & 0xFFFF;
    uint32_t address = cpuState.getReg(rn);
    bool rnInList = rListBinary & (1 << rn);
    bool pcInList = rListBinary & 0x8000;

    if(!rListBinary) {
        if(upDown) {
            if(prePost)
                address+=0x4; // ib
            else
                address+=0x0; // ia
        } else {
            if(prePost)
                address-=0x40; // db
            else
                address-=0x3C; // da
        }

        if(loadStore) {
            cpuState.setReg(15,systemMemory->readWord(address));
            pcInList = 1;
        } else
            systemMemory->storeValue(cpuState.r[15]+12,address);
            // pcInList = 1; # technically not needed, but just making a note :p

        if(upDown) {
            if(prePost)
                address += 0x3C; // ib
            else
                address += 0x40; // ia
        }
        if(!upDown)
            if(!prePost)
                address -= 0x04; // da
        
    } else {
        uint32_t baseAddress = address;
        uint32_t rnStoreLoc;
        uint8_t oldMode = 0;
        int8_t offset;
        uint16_t listStartBinary;
        
        if(upDown) {
            offset = 4;
            listStartBinary = 0x1;
        } else {
            offset = -4;
            // if offset is negative, reverse the list order
            listStartBinary = 0x8000;
        }
        
        if(psr) { // If s bit is set and rn is in transfer list
            if(loadStore && pcInList) // LDM with R15 in transfer list
                cpuState.setCPSR(cpuState.getSPSR(cpuState.mode));
            else { // R15 not in list
                oldMode = cpuState.mode;
                cpuState.switchMode(User);
            }
        }

        if(loadStore) { // LDM

            if(upDown) {
                for(int8_t i = 0; i < 16; i++) {
                    if(listStartBinary & rListBinary) {
                        if(prePost)
                            address += offset;
                        cpuState.setReg(i, systemMemory->readWord(address));
                        if(!prePost)
                            address += offset;
                    }
                    listStartBinary <<= 1;
                }
            } else {
                for(int8_t i = 15; i > -1; i--) {
                    if(listStartBinary & rListBinary) {
                        if(prePost)
                            address += offset;
                        cpuState.setReg(i, systemMemory->readWord(address));
                        if(!prePost)
                            address += offset;
                    }
                    listStartBinary >>= 1;
                }
            }

        } else { // STM

            uint16_t rnListLoc = rListBinary & (1 << rn);
            bool rnFirstInList = (rListBinary & (rnListLoc - 1)) ? 0 : 1;

            if(upDown) {

                for(int8_t i = 0; i < 16; i++) {
                    
                    if(rListBinary & listStartBinary) {
                        if(prePost)
                            address += offset;
                        if(i == 15) {
                            if(rn == 15) {
                                systemMemory->storeValue(baseAddress, address);
                                rnStoreLoc = address;
                            } else
                                systemMemory->storeValue(cpuState.getReg(i) + 12, address);

                        } else {
                            if(i == rn) {
                                systemMemory->storeValue(baseAddress, address);
                                rnStoreLoc = address;
                            } else
                                systemMemory->storeValue(cpuState.getReg(i), address);

                        }
                        if(!prePost)
                            address += offset;
                    }

                    listStartBinary <<= 1;

                    
                }
            } else {

                for(int8_t i = 15; i > -1; i--) {
                    
                    if(rListBinary & listStartBinary) {
                        if(prePost)
                            address += offset;
                        if(i == 15) {
                            if(rn == 15) {
                                systemMemory->storeValue(baseAddress, address);
                                rnStoreLoc = address;
                            } else
                                systemMemory->storeValue(cpuState.getReg(i) + 12, address);

                        } else {
                            if(i == rn) {
                                systemMemory->storeValue(baseAddress, address);
                                rnStoreLoc = address;
                            } else
                                systemMemory->storeValue(cpuState.getReg(i), address);

                        }
                        if(!prePost)
                            address += offset;
                    }

                    listStartBinary >>= 1;
                }
            }

            if(!rnFirstInList && rnInList)
                systemMemory->storeValue(address, rnStoreLoc);
        }

        if(oldMode)
            cpuState.switchMode(oldMode);
    }
    
    // if STM wb bit is set, write-back; don't if LDM and rn is in list
    if(writeBack && !(loadStore && rnInList)) 
        cpuState.setReg(rn,address);
    
    if(!(pcInList && loadStore))
        cpuState.r[15]+=4;
}

template<bool byteWord>
void ARM7TDMI::ARMswap(uint32_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Swap=",cpuState.r[15]);
    #endif
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint8_t rm = instruction & 0xF;

    uint32_t rnValue = cpuState.getReg(rn);

    // swap byte
    if(byteWord) {
        uint32_t rnAddrValue = systemMemory->readByte(rnValue);
        systemMemory->storeValue(static_cast<uint8_t>(cpuState.getReg(rm)),rnValue);
        cpuState.setReg(rd,rnAddrValue);
    } else { // swap word
        uint32_t rnAddrValue = systemMemory->readWordRotate(rnValue);
        systemMemory->storeValue(cpuState.getReg(rm),rnValue);
        cpuState.setReg(rd,rnAddrValue);
    }

    cpuState.r[15]+=4;
}

template<uint8_t opcode, uint8_t offset>
void ARM7TDMI::THUMBmoveShiftedRegister(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move Shifted Reg=",cpuState.r[15]);
    #endif
    uint32_t rs = (instruction & 0x38) >> 3;
    uint8_t rd = instruction & 0x7;

    rs = cpuState.getReg(rs);
    switch(opcode) {
        case 0b00: // LSL
            cpuState.setReg(rd,ALUshift(rs,offset,0,1,1));
            break;
        case 0b01: // LSR
            cpuState.setReg(rd,ALUshift(rs,offset,1,1,1));
            break;
        case 0b10: // ASR
            cpuState.setReg(rd,ALUshift(rs,offset,0b10,1,1));
            break;
    }

    setZeroAndSign(cpuState.getReg(rd));
    if(rd != 15)
        cpuState.r[15]+=2;
}

template<uint8_t opcode, uint8_t rnNn>
void ARM7TDMI::THUMBaddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Subtract=",cpuState.r[15]);
    #endif
    uint32_t rs = cpuState.getReg((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;

    uint32_t result;
    switch(opcode) {
        case 0: // add reg
        {
            uint32_t rn = cpuState.getReg(rnNn);
            result = add(rs,rn,true);
            cpuState.setReg(rd,result);
            break;
        }
        case 1: // sub reg
        {
            uint32_t rn = cpuState.getReg(rnNn);
            result = sub(rs,rn,true);
            cpuState.setReg(rd,result);
            break;
        }
        case 2: // add imm
            result = add(rs,rnNn,true);
            cpuState.setReg(rd,result);
            break;
        case 3: // sub imm
            result = sub(rs,rnNn,true);
            cpuState.setReg(rd,result);
    }

    setZeroAndSign(result);
    if(rd != 15)
        cpuState.r[15]+=2;
}

template<uint8_t opcode, uint8_t rd>
void ARM7TDMI::THUMBmoveCompareAddSubtract(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Move/Compare/Add/Subtract=",cpuState.r[15]);
    #endif
    uint8_t nn = instruction & 0xFF;
    uint32_t result;

    switch(opcode) {
        case 0b00: // mov
            result = nn;
            cpuState.setReg(rd,result);
            break;
        case 0b01: // cmp
            result = sub(cpuState.getReg(rd),nn,1);
            break;
        case 0b10: // add
            result = add(cpuState.getReg(rd),nn,1);
            cpuState.setReg(rd,result);
            break;
        case 0b11: // sub
            result = sub(cpuState.getReg(rd),nn,1);
            cpuState.setReg(rd,result);
            break;
    }

    setZeroAndSign(result);
    if(rd != 15)
        cpuState.r[15]+=2;
}

template<uint8_t opcode>
void ARM7TDMI::THUMBaluOperations(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X ALU Operation=",cpuState.r[15]);
    #endif
    uint32_t rs = cpuState.getReg((instruction & 0x38) >> 3);
    uint8_t rd = instruction & 0x7;
    uint32_t rdValue = cpuState.getReg(rd);
    bool oldCarry = cpuState.carryFlag;

    uint32_t result;
    switch(opcode) {
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

template<uint8_t opcode, bool h1, bool h2>
void ARM7TDMI::THUMBhiRegOpsBranchEx(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Hi Register Operation=",cpuState.r[15]);
    #endif
    uint8_t rs = (instruction & 0x38) >> 3;
    uint8_t rd = instruction & 0x7;
    if(h1)
        rd+=8;
    if(h2)
        rs+=8;
    uint32_t rsValue = cpuState.getReg(rs);
    uint32_t rdValue = cpuState.getReg(rd);

    
    if(opcode != 3) {
        if(rd == 15)
            rdValue += 4;
        if(rs == 15)
            rsValue += 4;
    } else {
        if(rs == 15)
            rsValue+=4;
    }
    

    switch(opcode) {
        case 0: // ADD
            cpuState.setReg(rd,add(rdValue,rsValue,0));
            break;
        case 1: // CMP
            setZeroAndSign(sub(rdValue,rsValue,1));
            break;
        case 2: // MOV
            cpuState.setReg(rd,rsValue);
            break;
        case 3: // BX
            if(!(rsValue & 1))
                cpuState.state = 0;
            cpuState.setReg(15,rsValue);
    }

    if((opcode != 3) && (rd != 15))
        cpuState.r[15]+=2;
}

template<uint8_t rd>
void ARM7TDMI::THUMBloadPCRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load cpuState.r[15]-Relative=",cpuState.r[15]);
    #endif
    cpuState.setReg(rd,systemMemory->readWord(((cpuState.r[15]+4) & ~2) + (instruction & 0xFF) * 4));
    if(rd != 15)
        cpuState.r[15]+=2;
}

template<uint8_t opcode, uint8_t roReg>
void ARM7TDMI::THUMBloadStoreRegOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Reg Offset=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);
    uint32_t ro = cpuState.getReg(roReg);

    switch(opcode) {
        case 0: // STR
            systemMemory->storeValue(cpuState.getReg(rd),rb+ro);
            break;
        case 1: // STRB
            systemMemory->storeValue(static_cast<uint8_t>(cpuState.getReg(rd)),rb+ro);
            break;
        case 2: // LDR
            cpuState.setReg(rd,systemMemory->readWordRotate(rb+ro));
            break;
        case 3: // LDRB
            cpuState.setReg(rd,systemMemory->readByte(rb+ro));
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}

template<uint8_t opcode, uint8_t roReg>
void ARM7TDMI::THUMBloadStoreSignExtendedByteHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Sign Extended Byte/Halfword=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);
    uint32_t ro = cpuState.getReg(roReg);

    switch(opcode) {
        case 0: // STRH
            systemMemory->storeValue(static_cast<uint16_t>(cpuState.getReg(rd)),rb+ro);
            break;
        case 1: // LDSB
            cpuState.setReg(rd,static_cast<int8_t>(systemMemory->readByte(rb+ro)));
            break;
        case 2: // LDRH
            cpuState.setReg(rd,systemMemory->readHalfWordRotate(rb+ro));
            break;
        case 3: // LDSH
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

template<uint8_t opcode, uint32_t nn>
void ARM7TDMI::THUMBloadStoreImmOffset(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Immediate Offset=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);

    switch(opcode) {
        case 0: // STR
            systemMemory->storeValue(cpuState.getReg(rd),rb+(nn << 2));
            break;
        case 1: // LDR
            cpuState.setReg(rd,systemMemory->readWordRotate(rb+(nn << 2)));
            break;
        case 2: // STRB
            systemMemory->storeValue(static_cast<uint8_t>(cpuState.getReg(rd)),rb+nn);
            break;
        case 3: // LDRB
            cpuState.setReg(rd,systemMemory->readByte(rb+nn));
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}

template<bool opcode, uint32_t nn>
void ARM7TDMI::THUMBloadStoreHalfword(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store Halfword=",cpuState.r[15]);
    #endif
    uint8_t rd = instruction & 0x7;
    uint32_t rb = cpuState.getReg((instruction & 0x38) >> 3);

    switch(opcode) {
        case 0: // STRH
            systemMemory->storeValue(static_cast<uint16_t>(cpuState.getReg(rd)),rb+nn);
            break;
        case 1: // LDRH
            cpuState.setReg(rd,systemMemory->readHalfWordRotate(rb+nn));
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}

template<bool opcode, uint8_t rd>
void ARM7TDMI::THUMBloadStoreSPRelative(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Load Store SP Relative=",cpuState.r[15]);
    #endif
    uint16_t nn = (instruction & 0xFF) << 2;

    if(opcode) { // LDR
        cpuState.setReg(rd,systemMemory->readWordRotate(cpuState.getReg(13)+nn));
    } else { // STR
        systemMemory->storeValue(cpuState.getReg(rd),cpuState.getReg(13)+nn);
    }

    if(rd != 15)
        cpuState.r[15]+=2;
}

template<bool opcode, uint8_t rd>
void ARM7TDMI::THUMBgetRelativeAddress(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Get Relative Address=",cpuState.r[15]);
    #endif
    uint16_t nn = (instruction & 0xFF) << 2;
    if(opcode) // ADD SP
        cpuState.setReg(rd,cpuState.getReg(13) + nn);
    else // ADD PC
        cpuState.setReg(rd,((cpuState.r[15]+4) & ~2) + nn);
    if(rd != 15)
        cpuState.r[15]+=2;
}

template<bool opcode>
void ARM7TDMI::THUMBaddOffsetToSP(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Add Offset To SP=",cpuState.r[15]);
    #endif
    uint16_t nn = (instruction & 0x7F) << 2;
    if(opcode) // - offset
        cpuState.setReg(13,cpuState.getReg(13) - nn);
    else // + offset
        cpuState.setReg(13,cpuState.getReg(13) + nn);;
    cpuState.r[15]+=2;
}

template<bool opcode, bool pcLr>
void ARM7TDMI::THUMBpushPopRegisters(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Push/Pop Registers=",cpuState.r[15]);
    #endif
    uint8_t rListBinary = instruction & 0xFF;
    uint8_t rListOffset;
    uint32_t address = cpuState.getReg(13);

    if(opcode) { // POP, AKA load from memory
        

        rListOffset = 1;
        for(int8_t i = 0; i < 8; i++) {
            if(instruction & rListOffset) {
                cpuState.setReg(i,systemMemory->readWord(address));
                address+=4;
            }
            rListOffset <<= 1;
        }

        if(pcLr) {
            cpuState.setReg(15,systemMemory->readWord(address));
            address+=4;
            
        }
    } else { // PUSH, AKA store to memory
        
        if(pcLr) {
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
    if(!pcLr || !opcode)
        cpuState.r[15]+=2;
}

template<bool opcode, uint8_t rb>
void ARM7TDMI::THUMBmultipleLoadStore(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Multiple Load Store=",cpuState.r[15]);
    #endif
    uint8_t rListBinary = instruction & 0xFF;
    uint32_t address = cpuState.getReg(rb);
    
    if(!rListBinary) {
        if(opcode)
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

        if(opcode) { // LDMIA
            
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
    if(rListBinary || !opcode)
        cpuState.r[15]+=2;
}

template<uint8_t opcode>
void ARM7TDMI::THUMBconditionalBranch(uint16_t instruction) {
    #if defined(PRINT_INSTR)
        printf("at pc=%X Conditional Branch=",cpuState.r[15]);
    #endif
    int16_t offset = static_cast<int8_t>(instruction & 0xFF) * 2 + 4;
    bool condMet = false;
    
    condMet = checkCond(opcode);
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