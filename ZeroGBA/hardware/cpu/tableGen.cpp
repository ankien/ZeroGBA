#include "ARM7TDMI.hpp"
#include <cstdint>

#include "tableGen.inl"

template<uint16_t indexes>
static constexpr auto generateArmLut() {
    std::array<void (ARM7TDMI::*)(uint32_t), 4096> table{};
    // hash on bits 27-20 + 7-4
    staticFor<uint16_t, 0, 4096>([&](auto i) {
        if((i & 0b111000000000) == 0b101000000000) {
            constexpr bool branchLink = i & 0x100;
            table[i] = &ARM7TDMI::ARMbranch<branchLink>;
        } else if((i & 0b111000000000) == 0b100000000000) {
            constexpr bool prePost = i & 0x100;
            constexpr bool upDown = i & 0x80;
            constexpr bool psr = i & 0x40;
            constexpr bool writeBack = i & 0x20;
            constexpr bool loadStore = i & 0x10;
            table[i] = &ARM7TDMI::ARMblockDataTransfer<prePost,upDown,psr,writeBack,loadStore>;
        } else if((i & 0b111111111111) == 0b000100100001)
            table[i] = &ARM7TDMI::ARMbranchExchange;
        else if((i & 0b111100000000) == 0b111100000000)
            table[i] = &ARM7TDMI::ARMsoftwareInterrupt;
        else if((i & 0b111000000001) == 0b011000000001)
            table[i] = &ARM7TDMI::ARMundefinedInstruction; // undefined opcode
        else if((i & 0b110000000000) == 0b010000000000) {
            constexpr bool immediateOffset = i & 0x200;
            constexpr bool prePost = i & 0x100;
            constexpr bool upDown = i & 0x80;
            constexpr bool byteWord = i & 0x40;
            constexpr bool writeBack = i & 0x20;
            constexpr bool loadStore = i & 0x10;
            constexpr uint8_t shiftType = (i & 0x6) >> 1;
            table[i] = &ARM7TDMI::ARMsingleDataTransfer<immediateOffset,prePost,upDown,byteWord,writeBack,loadStore,shiftType>;
        } else if((i & 0b111110111111) == 0b000100001001) {
            constexpr bool byteWord = i & 0x40;
            table[i] = &ARM7TDMI::ARMswap<byteWord>;
        } else if((i & 0b111100001111) == 0b000000001001) {
            constexpr uint8_t opcode = (i & 0x1E0) >> 5;
            constexpr bool condCode = i & 0x10;
            table[i] = &ARM7TDMI::ARMmultiplyAndMultiplyAccumulate<opcode,condCode>;
        } else if((i & 0b111000011111) == 0b000000001011) {
            constexpr bool prePost = i & 0x100;
            constexpr bool upDown = i & 0x80;
            constexpr bool immediateOffset = i & 0x40;
            constexpr bool writeBack = i & 0x20;
            table[i] = &ARM7TDMI::ARMhdsDataSTRH<prePost,upDown,immediateOffset,writeBack>;
        } else if((i & 0b111000011111) == 0b000000011011) {
            constexpr bool prePost = i & 0x100;
            constexpr bool upDown = i & 0x80;
            constexpr bool immediateOffset = i & 0x40;
            constexpr bool writeBack = i & 0x20;
            table[i] = &ARM7TDMI::ARMhdsDataLDRH<prePost,upDown,immediateOffset,writeBack>;
        } else if((i & 0b111000011111) == 0b000000011101) {
            constexpr bool prePost = i & 0x100;
            constexpr bool upDown = i & 0x80;
            constexpr bool immediateOffset = i & 0x40;
            constexpr bool writeBack = i & 0x20;
            table[i] = &ARM7TDMI::ARMhdsDataLDRSB<prePost,upDown,immediateOffset,writeBack>;
        } else if((i & 0b111000011111) == 0b000000011111) {
            constexpr bool prePost = i & 0x100;
            constexpr bool upDown = i & 0x80;
            constexpr bool immediateOffset = i & 0x40;
            constexpr bool writeBack = i & 0x20;
            table[i] = &ARM7TDMI::ARMhdsDataLDRSH<prePost,upDown,immediateOffset,writeBack>;
        } else if((i & 0b110110010000) == 0b000100000000) {
            constexpr bool immediate = i & 0x200;
            constexpr bool psr = i & 0x40;
            constexpr bool msr = i & 0x20;
            table[i] = &ARM7TDMI::ARMpsrTransfer<immediate,psr,msr>;
        } else if((i & 0b110000000000) == 0b000000000000) {
            constexpr bool immediate = i & 0x200;
            constexpr uint8_t opcode = (i & 0x1E0) >> 5;
            constexpr bool condCode = i & 0x10;
            constexpr bool shiftByReg = i & 1;
            table[i] = &ARM7TDMI::ARMdataProcessing<immediate,opcode,condCode,shiftByReg>;
        } else
            table[i] = &ARM7TDMI::ARMemptyInstruction; // undecoded opcode
        });

    return table;
}

template<uint16_t indexes>
static constexpr auto generateThumbLut() {
    std::array<void (ARM7TDMI::*)(uint16_t), 1024> table{};
    // hash on bits 15-6, 10 bits are used for generating functions, but only 8 (15-8) are needed for decoding
    staticFor<uint16_t, 0, 1024>([&](auto i) {
        if((i & 0b1111111100) == 0b1101111100)
            table[i] = &ARM7TDMI::THUMBsoftwareInterrupt;
        else if((i & 0b1111111100) == 0b1011000000)
            table[i] = &ARM7TDMI::THUMBaddOffsetToSP;
        else if((i & 0b1111110000) == 0b0100000000)
            table[i] = &ARM7TDMI::THUMBaluOperations;
        else if((i & 0b1111110000) == 0b0100010000)
            table[i] = &ARM7TDMI::THUMBhiRegOpsBranchEx;
        else if((i & 0b1111011000) == 0b1011010000)
            table[i] = &ARM7TDMI::THUMBpushPopRegisters;
        else if((i & 0b1111000000) == 0b1111000000)
            table[i] = &ARM7TDMI::THUMBlongBranchWithLink;
        else if((i & 0b1111100000) == 0b1110000000)
            table[i] = &ARM7TDMI::THUMBunconditionalBranch;
        else if((i & 0b1111100000) == 0b0001100000)
            table[i] = &ARM7TDMI::THUMBaddSubtract;
        else if((i & 0b1111100000) == 0b0100100000)
            table[i] = &ARM7TDMI::THUMBloadPCRelative;
        else if((i & 0b1111000000) == 0b1100000000)
            table[i] = &ARM7TDMI::THUMBmultipleLoadStore;
        else if((i & 0b1111000000) == 0b1101000000)
            table[i] = &ARM7TDMI::THUMBconditionalBranch;
        else if((i & 0b1111000000) == 0b1000000000)
            table[i] = &ARM7TDMI::THUMBloadStoreHalfword;
        else if((i & 0b1111000000) == 0b1010000000)
            table[i] = &ARM7TDMI::THUMBgetRelativeAddress;
        else if((i & 0b1111000000) == 0b1001000000)
            table[i] = &ARM7TDMI::THUMBloadStoreSPRelative;
        else if((i & 0b1111001000) == 0b0101000000)
            table[i] = &ARM7TDMI::THUMBloadStoreRegOffset;
        else if((i & 0b1111001000) == 0b0101001000)
            table[i] = &ARM7TDMI::THUMBloadStoreSignExtendedByteHalfword;
        else if((i & 0b1110000000) == 0b0010000000)
            table[i] = &ARM7TDMI::THUMBmoveCompareAddSubtract;
        else if((i & 0b1110000000) == 0b0000000000)
            table[i] = &ARM7TDMI::THUMBmoveShiftedRegister;
        else if((i & 0b1110000000) == 0b0110000000)
            table[i] = &ARM7TDMI::THUMBloadStoreImmOffset;
        else
            table[i] = &ARM7TDMI::THUMBemptyInstruction;
        });

    return table;
}

auto armLut = generateArmLut<4096>();
auto thumbLut = generateThumbLut<1024>();