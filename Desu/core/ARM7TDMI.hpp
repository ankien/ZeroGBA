#pragma once
#include <cstdint>
#include <iostream>
#include "../hardware/GBAMemory.hpp"
#include "../Scheduler.hpp"
#include "../hardware/MMIO.h"

struct ARM7TDMI {
    // cycles per instruction
    static const int32_t cycleTicks = 10; // stubbing this for now, todo: implement correct cycle timings

    // lookup tables, array size is the different number of instructions
    // todo: use templates to generate these tables, and their constexpr arguments at compile time
    void (ARM7TDMI::*armTable[4096])(uint32_t);
    void (ARM7TDMI::*thumbTable[256])(uint16_t);

    // could make this a generic pointer for code reuse with another system (the DS has an ARM7TDMI)
    GBAMemory* systemMemory;
    Scheduler* scheduler;

    enum exceptions { Reset, UndefinedInstruction, SoftwareInterrupt, PrefetchAbort, DataAbort,
                      AddressExceeds26Bit, NormalInterrupt, FastInterrupt };

    enum modes { User = 16, FIQ, IRQ, Supervisor, Abort = 23, Undefined = 27, System = 31 };

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
    
    // Used to fill LUTs
    void fillARM();
    void fillTHUMB();
    
    // ARM/THUMB helper functions
    void handleException(uint8_t, int8_t, uint8_t);
    uint16_t fetchARMIndex(uint32_t);
    uint8_t fetchTHUMBIndex(uint16_t);

    // memory helper functions
    template<typename T> T& memoryArray(uint32_t); // address is aligned by bytes for all types
    template<typename T> uint32_t writeable(uint32_t, T);
    void storeValue(uint8_t, uint32_t);
    void storeValue(uint16_t, uint32_t);
    void storeValue(uint32_t, uint32_t);
    // memory getters, rotates are for misaligned ldr+swp
    uint8_t readByte(uint32_t);
    uint16_t readHalfWord(uint32_t);
    uint32_t readHalfWordRotate(uint32_t);
    uint32_t readWord(uint32_t);
    uint32_t readWordRotate(uint32_t);

    // register helpers
    void switchMode(uint8_t);
    uint32_t getBankedReg(uint8_t, uint8_t);
    void setBankedReg(uint8_t, uint8_t, uint32_t);
    uint32_t getReg(uint8_t);
    void setReg(uint8_t, uint32_t);
    uint32_t getSPSR(uint8_t);
    void setSPSR(uint8_t,uint32_t);
    uint32_t getCPSR();
    void setCPSR(uint32_t);
    bool checkCond(uint32_t);
    // ALU Helpers: shift for registers, ALUshift affects carry flag
    uint32_t ror(uint32_t, uint8_t);
    uint32_t ALUshift(uint32_t, uint8_t, uint8_t,bool,bool);
    uint32_t sub(uint32_t,uint32_t,bool);
    uint32_t add(uint32_t,uint32_t,bool);
    uint32_t addCarry(uint32_t,uint32_t,bool,bool);
    void setZeroAndSign(uint32_t);
    void setZeroAndSign(uint64_t);

    // For unimplemented/undefined instructions
    void ARMundefinedInstruction(uint32_t);
    void ARMemptyInstruction(uint32_t);
    void THUMBemptyInstruction(uint16_t);

    /// ARM Instructions ///
    // todo: implement a templated instruction handler
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
    // todo: optimize this instruction with a straight memcpy
    void ARMblockDataTransfer(uint32_t);
    void ARMswap(uint32_t);

    /// THUMB Instructions ///
    void THUMBmoveShiftedRegister(uint16_t);
    void THUMBaddSubtract(uint16_t);
    void THUMBmoveCompareAddSubtract(uint16_t);
    void THUMBaluOperations(uint16_t);
    void THUMBhiRegOpsBranchEx(uint16_t);

    void THUMBloadPCRelative(uint16_t);
    void THUMBloadStoreRegOffset(uint16_t);
    void THUMBloadStoreSignExtendedByteHalfword(uint16_t);
    void THUMBloadStoreImmOffset(uint16_t);
    void THUMBloadStoreHalfword(uint16_t);
    void THUMBloadStoreSPRelative(uint16_t);

    void THUMBgetRelativeAddress(uint16_t);
    void THUMBaddOffsetToSP(uint16_t);

    void THUMBpushPopRegisters(uint16_t);
    void THUMBmultipleLoadStore(uint16_t);

    void THUMBconditionalBranch(uint16_t);
    void THUMBunconditionalBranch(uint16_t);
    void THUMBlongBranchWithLink(uint16_t);
    void THUMBsoftwareInterrupt(uint16_t);

    /// Scheduler Stuff ///
    void scheduleInterruptCheck();
    bool irqsEnabled();
};

#include "cpuHelpers.inl"