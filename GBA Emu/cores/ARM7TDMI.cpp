#include "ARM7TDMI.hpp"

void ARM7TDMI::fillARM(uint8_t* romMemory) {
    
    for(int i = 0; i < 4096; i++) {

        if((i & 0b111000000000) == 0b101000000000) {
            armTable[i] = &ARM7TDMI::branch;
        } else if(i & 0b111111111111 == 0b000100100001) {
            armTable[i] = &ARM7TDMI::branchExchange;
        } else if(i & 0b111100000000 == 0b111100000000) {
            armTable[i] = &ARM7TDMI::softwareInterrupt;
        } else if(i &) {
            
        } else {
            armTable[i] = &ARM7TDMI::emptyInstruction;
        }

    }
}

void ARM7TDMI::fillTHUMB(uint8_t* romMemory) {
    
    for(int i = 0; i < 256; i++) {

        

    }
}

// need to check if this is left or right shift depending on how instr is loaded...
// Bits 27-20 + 7-4
uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}

// Bits 8-15
uint8_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 8;
}

/* series of actions performed when entering an exception
void ARM7TDMI::handleException(uint8_t exception, uint32_t nn, uint8_t newMode) {
    const uint8_t index = getModeIndex(newMode);
    r14[index] = pc + nn; // save old PC, always in ARM-style format
    spsr[index] = getCPSR();
    cpsr.state = 0;
    cpsr.mode = newMode;
    cpsr.irqDisable = 1;
    
    if((cpsr.mode == Reset) || (cpsr.mode == FIQ))
        cpsr.fiqDisable = 1;

    // Might need to implement a return based on mode
    switch(cpsr.mode) {

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
            break;
    }


}
*/

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
        case 15:
            pc = arg;
    }
}

uint32_t ARM7TDMI::getCPSR() {
    return cpsr.mode |
           (cpsr.state << 5) |
           (cpsr.fiqDisable << 6) |
           (cpsr.irqDisable << 7) |
           (cpsr.reserved << 8) |
           (cpsr.stickyOverflow << 27) |
           (cpsr.overflowFlag << 28) |
           (cpsr.carryFlag << 29) |
           (cpsr.zeroFlag << 30) |
           (cpsr.signFlag << 31);
}

void ARM7TDMI::setCPSR(uint32_t num) {
    cpsr.mode = (num & 0x1F);
    cpsr.state = (num & 0x20) >> 5;
    cpsr.fiqDisable = (num & 0x40) >> 6;
    cpsr.irqDisable = (num & 0x80) >> 7;
    cpsr.reserved = (num & 0x7FFFF00) >> 8;
    cpsr.stickyOverflow = (num & 0x8000000) >> 27;
    cpsr.overflowFlag = (num & 0x10000000) >> 28;
    cpsr.carryFlag = (num & 0x20000000) >> 29;
    cpsr.zeroFlag = (num & 0x40000000) >> 30;
    cpsr.signFlag = (num & 0x80000000) >> 31;
}

// For unimplemented instructions
void ARM7TDMI::emptyInstruction(uint32_t instruction) {}

/// Branch ///
void ARM7TDMI::branch(uint32_t instruction) {
    uint32_t offset = instruction & 0xFFFFFF;
    if(instruction & 0x1000000)
        r14[cpsr.mode] = pc + 4;
    pc += 8 + (offset * 4);
}
void ARM7TDMI::branchExchange(uint32_t instruction) {
    uint8_t rn = instruction & 0xF;
    if(rn == 0)
        cpsr.state = 1;
    pc = getModeArrayIndex(cpsr.mode,rn);
}
// Need to implement exception handling!
void ARM7TDMI::softwareInterrupt(uint32_t instruction) {
    
}
