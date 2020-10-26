#include "ARM7TDMI.hpp"

ARM7TDMI::ARM7TDMI() {
    memset(&IOmap,0,sizeof IOmap);
}

void ARM7TDMI::interpretARMCycle(uint8_t romMemory[]) {
    uint32_t opcode = romMemory[];

}

void ARM7TDMI::interpretTHUMBCycle(uint8_t romMemory[]) {
    uint16_t opcode = ;

}