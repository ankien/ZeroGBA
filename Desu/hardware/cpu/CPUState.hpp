#pragma once
#include <cstdint>
#include "cpuGlobals.inl"

struct CPUState {

    /// Registers ///
    // CPSR & SPSR = program status registers
    // current regs
    uint32_t r[16]; // R0-15; r13 - SP, r14 - LR, r15 - PC

    // banked regs
    uint32_t sysUserReg[7]; // r8-14
    uint32_t fiqReg[8]; // r8-14 + SPSR
    uint32_t svcReg[3]; // r13-14 + SPSR
    uint32_t abtReg[3]; // r13-14 + SPSR
    uint32_t irqReg[3]; // r13-14 + SPSR
    uint32_t undReg[3]; // r13-14 + SPSR

    // CPSR bitfield implementation, least significant bit to most
    uint8_t mode; // 5 : see enum modes
    bool    state, // 1 : 0 = ARM, 1 = THUMB
            fiqDisable, // 1 : 0 = enable, 1 = disable
            irqDisable; // 1 : 0 = enable, 1 = disable
    uint32_t reserved; // 19 : never used?
    bool stickyOverflow, // 1 : 1 = sticky overflow, ARMv5TE and up only
         overflowFlag, // V, 1 : 0 = no overflow, 1 = signed overflow (if the result register is a negative 2's complement, set bit) 
         carryFlag, // C, 1 : 0 = borrow/no carry, 1 = carry/no borrow, addition overflow is carry; sub underflow is borrow
         zeroFlag, // Z, 1 : 0 = not zero, 1 = zero
         signFlag; // N, 1 : 0 = not signed, 1 = signed (31 bit is filled)

    // register helpers
    uint32_t getCPSR();
    void setCPSR(uint32_t);
    void switchMode(uint8_t);
    uint32_t getBankedReg(uint8_t, uint8_t);
    void setBankedReg(uint8_t, uint8_t, uint32_t);
    uint32_t getReg(uint8_t);
    void setReg(uint8_t, uint32_t);
    uint32_t getSPSR(uint8_t);
    void setSPSR(uint8_t,uint32_t);

    // for IRQs and SWIs
    void handleException(uint8_t, int8_t, uint8_t);
};

#include "cpuStateHelpers.inl"