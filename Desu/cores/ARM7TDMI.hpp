#pragma once
#include <string.h>
#include <stdint.h>
#include <bit>
#include <iostream>
#include <vector>
#include <algorithm>
#include "../hardware/GBA/GBAMemory.hpp"

struct ARM7TDMI {
    // todo: instruction timimg
    // cycles per second
    static const uint32_t CLOCK_RATE = 16780000;
    static const uint32_t DS_CLOCK_RATE = 33000000;

    // lookup tables, array size is the different number of instructions
    void (ARM7TDMI::*armTable[4096])(uint32_t);
    void (ARM7TDMI::*thumbTable[256])(uint32_t);

    // could make this a generic pointer for code reuse
    GBAMemory* systemMemory;

    enum exceptions { Reset, UndefinedInstruction, SoftwareInterrupt, PrefetchAbort, DataAbort,
                      AddressExceeds26Bit, NormalInterrupt, FastInterrupt };

    enum modes { User = 16, FIQ, IRQ, Supervisor, Abort = 23, Undefined = 27, System = 31 };

    /// Registers ///
    // CPSR & SPSR = program status registers
    // registers are banked
    uint32_t reg[8]; // R0-7
    uint32_t r8[2]; // sys/user-fiq
    uint32_t r9[2]; // sys/user-fiq
    uint32_t r10[2]; // sys/user-fiq
    uint32_t r11[2]; // sys/user-fiq
    uint32_t r12[2]; // sys/user-fiq
    uint32_t r13[6]; // sys/user, fiq, svc, abt, irq, und
    uint32_t r14[6]; // sys/user, fiq, svc, abt, irq, und
    uint32_t pc = 0x8000000; // R15
    // CPSR bitfield implementation
    uint8_t mode, // 5 : see enum modes
            state, // 1 : 0 = ARM, 1 = THUMB
            fiqDisable, // 1 : 0 = enable, 1 = disable
            irqDisable; // 1 : 0 = enable, 1 = disable
    uint32_t reserved; // 19 : never used?
    uint8_t stickyOverflow, // 1 : 1 = sticky overflow, ARMv5TE and up only
            overflowFlag, // V, 1 : 0 = no overflow, 1 = overflow 
            carryFlag, // C, 1 : 0 = borrow/no carry, 1 = carry/no borrow
            zeroFlag, // Z, 1 : 0 = not zero, 1 = zero
            signFlag; // N, 1 : 0 = not signed, 1 = signed
    uint32_t spsr[6]; // N/A, fiq, svc, abt, irq, und

    ARM7TDMI(GBAMemory*);

    /// Helper functions ///
    void handleException(uint8_t, uint32_t, uint8_t);
    void fillARM();
    void fillTHUMB();
    uint16_t fetchARMIndex(uint32_t);
    uint8_t fetchTHUMBIndex(uint16_t);
    void storeValue(uint16_t, uint32_t);
    void storeValue(uint32_t, uint32_t);
    uint16_t readHalfWord(uint32_t);
    uint16_t readHalfWordRotate(uint32_t);
    uint32_t readWord(uint32_t);
    uint32_t readWordRotate(uint32_t);
    uint32_t getModeArrayIndex(uint8_t, uint8_t);
    void setModeArrayIndex(uint8_t, uint8_t, uint32_t);
    uint32_t getCPSR();
    void setCPSR(uint32_t);
    bool checkCond(uint32_t);
    template<typename INT>
    INT shift(INT, uint8_t, uint8_t);
    void setZeroAndSign(uint32_t);

    // For unimplemented/undefined instructions
    void emptyInstruction(uint32_t);

    /// ARM Instructions ///
    void ARMbranch(uint32_t);
    void ARMbranchExchange(uint32_t);
    void ARMsoftwareInterrupt(uint32_t);

    void ARMdataProcessing(uint32_t);
    void ARMmultiplyAndMultiplyAccumulate(uint32_t);

    void ARMpsrTransfer(uint32_t);
    void ARMsingleDataTransfer(uint32_t);
    void ARMhdsDataSTRH(uint32_t);
    void ARMhdsDataLDRH(uint32_t);
    void ARMhdsDataLDRSB(uint32_t);
    void ARMhdsDataLDRSH(uint32_t);
    void ARMblockDataTransfer(uint32_t);
    void ARMswap(uint32_t);

    /// THUMB Instructions ///

};

// Bits 27-20 + 7-4
inline uint16_t ARM7TDMI::fetchARMIndex(uint32_t instruction) {
    return ((instruction >> 16) & 0xFF0) | ((instruction >> 4) & 0xF);
}
// Bits 8-15
inline uint8_t ARM7TDMI::fetchTHUMBIndex(uint16_t instruction) {
    return instruction >> 8;
}

inline void ARM7TDMI::storeValue(uint16_t value, uint32_t address) {
    (*systemMemory).setByte(address,value);
    (*systemMemory).setByte(address + 1, (value & 0xFF00) >> 8);
}
inline void ARM7TDMI::storeValue(uint32_t value, uint32_t address) {
    (*systemMemory).setByte(address, value);
    (*systemMemory).setByte(address + 1, (value & 0xFF00) >> 8);
    (*systemMemory).setByte(address + 2, (value & 0xFF0000) >> 16);
    (*systemMemory).setByte(address + 3, (value & 0xFF000000) >> 24);
}
inline uint16_t ARM7TDMI::readHalfWord(uint32_t address) {
    return (*systemMemory)[address] |
           (*systemMemory)[address + 1] << 8;
}
inline uint16_t ARM7TDMI::readHalfWordRotate(uint32_t address) {
    uint16_t halfWord = readHalfWord(address);
    uint8_t rorAmount = (address & 1) << 3;
    return shift(halfWord,rorAmount,3);
}
inline uint32_t ARM7TDMI::readWord(uint32_t address) {
    return (*systemMemory)[address] |
           (*systemMemory)[address + 1] << 8 |
           (*systemMemory)[address + 2] << 16 |
           (*systemMemory)[address + 3] << 24;
}
// Memory alignment stuff
inline uint32_t ARM7TDMI::readWordRotate(uint32_t address) {
    uint32_t word = readWord(address);
    uint8_t rorAmount = (address & 3) << 3;
    return shift(word,rorAmount,3);
}

inline uint32_t ARM7TDMI::getCPSR() {
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
inline void ARM7TDMI::setCPSR(uint32_t num) {
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

template<typename INT>
INT ARM7TDMI::shift(INT value, uint8_t shiftAmount, uint8_t shiftType) {
    switch(shiftType) {
        case 0b00: // lsl
            return value << shiftAmount; 
        case 0b01: // lsr
            return value >> shiftAmount;
        case 0b10: // asr
        {
            uint8_t dupeBit = (sizeof(value) * 8) - 1;
            if(value & (1 << dupeBit))
                return (value >> shiftAmount) | (0xFFFFFFFF << shiftAmount);
            return value >> shiftAmount;
        }
        case 0b11: // ror
            return std::rotr(value,shiftAmount);
    }
}
inline void ARM7TDMI::setZeroAndSign(uint32_t arg) {
    (arg == 0) ?  zeroFlag = 1 : zeroFlag = 0;
    (arg | 0x80000000) ? signFlag = 1 : signFlag = 0;
}