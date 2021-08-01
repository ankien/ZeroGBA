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

