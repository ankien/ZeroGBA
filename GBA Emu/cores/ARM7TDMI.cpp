#include "ARM7TDMI.hpp"

void ARM7TDMI::fillARM(uint8_t* romMemory) {
    uint32_t instruction = romMemory[pc];

    uint16_t armIndex = fetchARMIndex(instruction);
    for(int i = 0; i < 4096; i++) {

        if(armIndex) {
            
        } else {
            armTable[armIndex] = &emptyInstruction;
        }

        pc += 4;
    }
}

void ARM7TDMI::fillTHUMB(uint8_t* romMemory) {
    uint16_t instruction = romMemory[pc];
    
    uint8_t thumbIndex = fetchTHUMBIndex(instruction);
    for() {
        
        pc += 2;
    }
}

// For unimplemented/unused instructions
void emptyInstruction() {}

// need to check if this is left or right shift depending on how instr is loaded...
// Bits 27-20 + 7-4
uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}

// Bits 8-15
uint8_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 8;
}

void ARM7TDMI::interpretARMCycle(uint8_t* romMemory) {
    uint16_t armIndex = fetchARMIndex(romMemory[pc]);
    armTable[armIndex]();
}

void ARM7TDMI::interpretTHUMBCycle(uint8_t* romMemory) {
    uint8_t thumbIndex = fetchTHUMBIndex(romMemory[pc]);
    thumbTable[thumbIndex]();
}

// series of actions performed when entering an exception
void ARM7TDMI::handleException(uint8_t exception, uint32_t nn, uint8_t newMode) {
    const uint8_t index = getModeIndex(newMode);
    r14[index] = pc + nn; // save old PC, always in ARM-style format
    spsr[index] = cpsr;
    cpsr = ((cpsr & 0xFFFFFFC0) | (newMode));
    cpsr |= 0x80;

    if((cpsr & 0x11) || (exception == Reset))
        cpsr |= 0x4;

    // Might need to implement a return based on mode
    switch(cpsr & 0x1F) {

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

uint8_t ARM7TDMI::getModeIndex(uint8_t mode) {
    switch(mode) {
        case System:
        case User:
            return 0;
        case FIQ:
            return 1;
        case Supervisor:
            return 2;
        case Abort:
            return 3;
        case IRQ:
            return 4;
        case Undefined:
            return 5;
    }
}