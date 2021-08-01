#pragma once
#include <cstdint>
#include <iostream>
#include <array>
#include "../../common/bits.hpp"
#include "../../common/staticFor.hpp"
#include "CPUState.hpp"
#include "../memory/GBAMemory.hpp"
#include "../../Scheduler.hpp"
#include "../Interrupts.hpp"
#include "../MMIO.h"

#include "cpuGlobals.inl"

struct ARM7TDMI final {
    // cycles per instruction
    static constexpr int32_t cycleTicks = 2; // stubbing this for now, 2 CPI for semi-accuracy and compatibility, todo: implement correct cycle timings

    // could make this a generic pointer for code reuse with another system (the DS has an ARM7TDMI)
    GBAMemory* systemMemory;
    Scheduler* scheduler;
    Interrupts* interrupts;
    
    CPUState cpuState{};
    
    // ARM/THUMB helper functions
    uint16_t fetchARMIndex(uint32_t);
    uint16_t fetchTHUMBIndex(uint16_t);

    bool checkCond(uint32_t);
    // ALU Helpers: shift for registers, ALUshift affects carry flag
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
    template<bool> void ARMbranch(uint32_t);
    void ARMbranchExchange(uint32_t);
    void ARMsoftwareInterrupt(uint32_t);

    template<bool,uint8_t,bool,bool> void ARMdataProcessing(uint32_t);
    template<uint8_t,bool> void ARMmultiplyAndMultiplyAccumulate(uint32_t);

    template<bool,bool,bool> void ARMpsrTransfer(uint32_t);

    template<bool,bool,bool,bool,bool,bool,uint8_t> void ARMsingleDataTransfer(uint32_t);
    template<bool,bool,bool,bool> void ARMhdsDataSTRH(uint32_t);
    template<bool,bool,bool,bool> void ARMhdsDataLDRH(uint32_t);
    template<bool,bool,bool,bool> void ARMhdsDataLDRSB(uint32_t);
    template<bool,bool,bool,bool> void ARMhdsDataLDRSH(uint32_t);
    // todo: optimize this instruction with a straight memcpy
    template<bool,bool,bool,bool,bool> void ARMblockDataTransfer(uint32_t);
    template<bool> void ARMswap(uint32_t);

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
};

#include "cpuHelpers.inl"