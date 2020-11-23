#pragma once
#include <string.h>
#include <stdint.h>
#include <iostream>

struct ARM7TDMI {
    // cycles per second
    static const uint32_t CLOCK_RATE = 16780000;
    static const uint32_t DS_CLOCK_RATE = 33000000;

    // lookup tables, array size is the different number of instructions
    void (ARM7TDMI::*armTable[4096])(uint32_t);
    void (ARM7TDMI::*thumbTable[256])(uint32_t);

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
    struct {
        uint32_t mode : 5,
                 state : 1,
                 fiqDisable : 1,
                 irqDisable : 1,
                 reserved : 19,
                 stickyOverflow: 1,
                 overflowFlag: 1,
                 carryFlag : 1,
                 zeroFlag : 1,
                 signFlag : 1;
    } cpsr;
    uint32_t spsr[6]; // N/A, fiq, svc, abt, irq, und

    void handleException(uint8_t, uint32_t, uint8_t);
    void fillARM(uint8_t*);
    void fillTHUMB(uint8_t*);
    uint16_t fetchARMIndex(uint32_t);
    uint8_t fetchTHUMBIndex(uint16_t);
    uint8_t getModeIndex(uint8_t);
    uint32_t getCPSR();
    void setCPSR(uint32_t);

    // For unimplemented/undefined instructions
    void emptyInstruction(uint32_t);

    /// Branch ///
    void branch(uint32_t);
    void branchExchange(uint32_t);
    void softwareInterrupt(uint32_t);

    /// Data Processing ///
};