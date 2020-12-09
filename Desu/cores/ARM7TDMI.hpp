#pragma once
#include <string.h>
#include <stdint.h>
#include <bit>
#include <iostream>

struct ARM7TDMI {
    // cycles per second
    static const uint32_t CLOCK_RATE = 16780000;
    static const uint32_t DS_CLOCK_RATE = 33000000;

    // lookup tables, array size is the different number of instructions
    void (ARM7TDMI::*armTable[4096])(uint32_t);
    void (ARM7TDMI::*thumbTable[256])(uint32_t);

    uint8_t* systemMemory;

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
    uint32_t pc; // R15
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

    ARM7TDMI(uint8_t*);

    /// Helper functions ///
    void handleException(uint8_t, uint32_t, uint8_t);
    void fillARM();
    void fillTHUMB();
    inline uint16_t fetchARMIndex(uint32_t);
    inline uint8_t fetchTHUMBIndex(uint16_t);
    inline void storeValue(uint16_t,uint32_t);
    inline void storeValue(uint32_t,uint32_t);
    inline uint16_t readHalfWord(uint32_t);
    inline uint16_t readHalfWordRotate(uint32_t);
    inline uint32_t readWord(uint32_t);
    inline uint32_t readWordRotate(uint32_t);
    inline uint32_t getModeArrayIndex(uint8_t, uint8_t);
    inline void setModeArrayIndex(uint8_t, uint8_t, uint32_t);
    inline uint32_t getCPSR();
    inline void setCPSR(uint32_t);
    inline bool checkCond(uint32_t);
    template <typename INT>
    inline INT shift(INT, uint8_t, uint8_t);
    inline void setZeroAndSign(uint32_t);

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

    void ARMswap(uint32_t);

    /// THUMB Instructions ///

};