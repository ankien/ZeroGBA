#include "ARM7TDMI.hpp"

void ARM7TDMI::interpretARMCycle(uint8_t romMemory[]) {
    uint32_t opcode = romMemory[];

}

void ARM7TDMI::interpretTHUMBCycle(uint8_t romMemory[]) {
    uint16_t opcode = ;

}

// series of actions performed when entering an exception
void ARM7TDMI::handleException(uint8_t exception, uint32_t nn, uint8_t newMode) {
    const uint8_t index = getModeIndex(newMode);
    r14[index] = reg[15] + nn; // save old PC, always in ARM-style format
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
                    reg[15] = 0x0;

                    break;
                case AddressExceeds26Bit:
                    reg[15] = 0x14;
                    break;
                case SoftwareInterrupt:
                    reg[15] = 0x8;
                    break;
            }
            break;

        case Undefined:
            
            switch(exception) {
                
                case UndefinedInstruction:
                    reg[15] = 0x4;
                    break;
            }
            break;

        case Abort:

            switch(exception) {

                case DataAbort:
                    reg[15] = 0x10;
                    break;
                case PrefetchAbort:
                    reg[15] = 0xC;
                    break;
            }
            break;

        case IRQ:
            
            switch(exception) {

                case NormalInterrupt:
                    reg[15] = 0x18;
                    break;
            }
            break;

        case FIQ:
            
            switch(exception) {

                case FastInterrupt:
                    reg[15] = 0x1C;
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