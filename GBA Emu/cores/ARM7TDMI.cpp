#include "ARM7TDMI.hpp"

void ARM7TDMI::fillARM(uint8_t* romMemory) {
    
    for(int i = 0; i < 4096; i++) {

        if((i & 0b111000000000) == 0b101000000000) {
            armTable[i] = &ARM7TDMI::ARMbranch;
        } else if(i & 0b111111111111 == 0b000100100001) {
            armTable[i] = &ARM7TDMI::ARMbranchExchange;
        } else if(i & 0b111100000000 == 0b111100000000) {
            armTable[i] = &ARM7TDMI::ARMsoftwareInterrupt;
        } else if() { 
            
        } else if(i & 0b110000000000 == 0b000000000000) {
            armTable[i] = &ARM7TDMI::ARMdataProcessing;
        } else {
            armTable[i] = &ARM7TDMI::emptyInstruction;
        }

    }
}
void ARM7TDMI::fillTHUMB(uint8_t* romMemory) {
    
    for(int i = 0; i < 256; i++) {

        

    }
}

// Bits 27-20 + 7-4
uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}
// Bits 8-15
uint8_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 8;
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
            break;
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

uint32_t ARM7TDMI::getCPSR() {
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
void ARM7TDMI::setCPSR(uint32_t num) {
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

template <typename INT>
INT ARM7TDMI::shift(INT value, uint8_t shiftAmount, uint8_t shiftType) {
    switch(shiftType) {
        case 0b00: // lsl
            return value << shiftAmount; 
            break;
        case 0b01: // lsr
            return value >> shiftAmount;
            break;
        case 0b10: // asr
            uint8_t dupeBit = (sizeof(value) * 8) - 1;
            if(value & (1 << dupeBit))
                return (value >> shiftAmount) | (0xFFFFFFFF << shiftAmount);
            return value >> shiftAmount;
            break;
        case 0b11: // ror
            return std::rotr(value,shiftAmount);
            break;
    }
}

// For unimplemented instructions of either state
void ARM7TDMI::emptyInstruction(uint32_t instruction) {}

/// ARM:Branch ///
void ARM7TDMI::ARMbranch(uint32_t instruction) {
    uint32_t offset = instruction & 0xFFFFFF;
    if(instruction & 0x1000000)
        r14[mode] = pc + 4;
    pc += 8 + (offset * 4);
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

/// ARM:Data Processing ///
void ARM7TDMI::ARMdataProcessing(uint32_t instruction) {
    uint8_t I = instruction & 0x2000000;
    uint8_t s = instruction & 0x100000;
    uint8_t opcode = (instruction & 0x1E00000) >> 21;
    uint8_t rn = (instruction & 0xF0000) >> 16;
    uint8_t rd = (instruction & 0xF000) >> 12;
    uint32_t op2;

    // shifting
    switch(I) {

        case 0: // register is second operand
            uint8_t shiftType = (instruction & 0x60) >> 5;
            uint8_t r = instruction & 0x10; // if r == 1, pc+=12, else pc+=8
            uint8_t Rm = instruction & 0xF;
            switch(r) {
                case 16: // register operand w/ register shift
                    uint8_t Rs = (instruction & 0xF00) >> 8;
                    op2 = shift(getModeArrayIndex(mode,Rm),(uint8_t)getModeArrayIndex(mode,Rs),shiftType);
                    pc+=12;
                    break;
                case 0: // register operand w/ immediate shift
                    uint8_t Is = (instruction & 0xF80) >> 7;
                    if(Is == 0)
                        switch(shiftType) {
                            case 0b00:
                                op2 = getModeArrayIndex(mode,Rm);
                                break;
                            case 0b01:
                                op2 = 0;
                                carryFlag = (getModeArrayIndex(mode,Rm) & 0x80000000) >> 31;
                                break;
                            case 0b10:
                                op2 = shift(getModeArrayIndex(mode,Rm),32,shiftType);
                                carryFlag = op2 & 1;
                                break;
                            case 0b11:
                                op2 = shift(getModeArrayIndex(mode,Rm),1,0b11);
                                op2 &= carryFlag << 31;
                                break;
                        }
                    else
                        op2 = shift(getModeArrayIndex(mode,Rm),Is,shiftType);
                    pc+=8;
                    break;
            }
            break;

        case 0x2000000: // immediate is second operand
            uint8_t Imm = instruction & 0xF;
            uint8_t rotate = (instruction & 0xF00) >> 8;
            op2 = shift(Imm,rotate,0b11);
            pc+=8;
            break;
    }

    // when writing to R15
    if((s) && (rd == 15)) {
        setCPSR(spsr[getModeArrayIndex(mode,'S')]);
        s = 0;
    }    

    // do i need to do something with the C flag???
    uint32_t op1 = getModeArrayIndex(mode,rn);
    switch(opcode) {
        case 0x0: // AND
            setModeArrayIndex(mode,rd,op1 & op2);
            if(s) {
                ;
                (op1 == 0) ?  zeroFlag = 1 : zeroFlag = 0;
                ;
            }
            break;
        case 0x1: // EOR
            break;
        case 0x2: // SUB
            break;
        case 0x3: // RSB
            break;
        case 0x4: // ADD
            break;
        case 0x5: // ADC
            break;
        case 0x6: // SBC
            break;
        case 0x7: // RSC
            break;
        case 0x8: // TST
            break;
        case 0x9: // TEQ
            break;
        case 0xA: // CMP
            break;
        case 0xB: // CMN
            break;
        case 0xC: // ORR
            break;
        case 0xD: // MOV
            break;
        case 0xE: // BIC
            break;
        case 0xF: // MVN
            break;
    }
}
