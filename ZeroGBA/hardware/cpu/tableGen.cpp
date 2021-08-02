#include "ARM7TDMI.hpp"
#include <cstdint>

#include "tableGen.inl"

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

static constexpr auto generateThumbLut() {
    std::array<void (ARM7TDMI::*)(uint16_t), 1024> table{};
    // hash on bits 15-6, 10 bits are used for generating functions, but only 8 (15-8) are needed for decoding
    staticFor<uint16_t, 0, 1024>([&](auto i) {
        if((i & 0b1111111100) == 0b1101111100)
            table[i] = &ARM7TDMI::THUMBsoftwareInterrupt;
        else if((i & 0b1111111100) == 0b1011000000) {
            constexpr bool opcode = i & 0x2;
            table[i] = &ARM7TDMI::THUMBaddOffsetToSP<opcode>;
        } else if((i & 0b1111110000) == 0b0100000000) {
            constexpr uint8_t opcode = i & 0xF;
            table[i] = &ARM7TDMI::THUMBaluOperations<opcode>;
        } else if((i & 0b1111110000) == 0b0100010000) {
            constexpr uint8_t opcode = (i & 0xC) >> 2;
            constexpr bool h1 = i & 2;
            constexpr bool h2 = i & 1;
            table[i] = &ARM7TDMI::THUMBhiRegOpsBranchEx<opcode,h1,h2>;
        } else if((i & 0b1111011000) == 0b1011010000) {
            constexpr bool opcode = i & 0x20;
            constexpr bool pcLr = i & 4;
            table[i] = &ARM7TDMI::THUMBpushPopRegisters<opcode,pcLr>;
        } else if((i & 0b1111000000) == 0b1111000000)
            table[i] = &ARM7TDMI::THUMBlongBranchWithLink;
        else if((i & 0b1111100000) == 0b1110000000)
            table[i] = &ARM7TDMI::THUMBunconditionalBranch;
        else if((i & 0b1111100000) == 0b0001100000) {
            constexpr uint8_t opcode = (i & 0x18) >> 3;
            constexpr uint8_t rnNn = i & 0x7;
            table[i] = &ARM7TDMI::THUMBaddSubtract<opcode,rnNn>;
        } else if((i & 0b1111100000) == 0b0100100000) {
            constexpr uint8_t rd = (i & 0x1C) >> 2;
            table[i] = &ARM7TDMI::THUMBloadPCRelative<rd>;
        } else if((i & 0b1111000000) == 0b1100000000) {
            constexpr bool opcode = i & 0x20;
            constexpr uint8_t rb = (i & 0x1C) >> 2;
            table[i] = &ARM7TDMI::THUMBmultipleLoadStore<opcode,rb>;
        } else if((i & 0b1111000000) == 0b1101000000) {
            constexpr uint8_t opcode = (i & 0x3C) >> 2;
            table[i] = &ARM7TDMI::THUMBconditionalBranch<opcode>;
        } else if((i & 0b1111000000) == 0b1000000000) {
            constexpr bool opcode = i & 0x20;
            constexpr uint32_t nn = (i & 0x1F) << 1;
            table[i] = &ARM7TDMI::THUMBloadStoreHalfword<opcode,nn>;
        } else if((i & 0b1111000000) == 0b1010000000) {
            constexpr bool opcode = i & 0x20;
            constexpr uint8_t rd = (i & 0x1C) >> 2;
            table[i] = &ARM7TDMI::THUMBgetRelativeAddress<opcode,rd>;
        } else if((i & 0b1111000000) == 0b1001000000) {
            constexpr bool opcode = i & 0x20;
            constexpr uint8_t rd = (i & 0x1C) >> 2;
            table[i] = &ARM7TDMI::THUMBloadStoreSPRelative<opcode,rd>;
        } else if((i & 0b1111001000) == 0b0101000000) {
            constexpr uint8_t opcode = (i & 0x30) >> 4;
            constexpr uint8_t ro = i & 0x7;
            table[i] = &ARM7TDMI::THUMBloadStoreRegOffset<opcode,ro>;
        } else if((i & 0b1111001000) == 0b0101001000) {
            constexpr uint8_t opcode = (i & 0x30) >> 4;
            constexpr uint8_t ro = i & 0x7;
            table[i] = &ARM7TDMI::THUMBloadStoreSignExtendedByteHalfword<opcode,ro>;
        } else if((i & 0b1110000000) == 0b0010000000) {
            constexpr uint8_t opcode = (i & 0x60) >> 5;
            constexpr uint8_t rd = (i & 0x1C) >> 2;
            table[i] = &ARM7TDMI::THUMBmoveCompareAddSubtract<opcode,rd>;
        } else if((i & 0b1110000000) == 0b0000000000) {
            constexpr uint8_t opcode = (i & 0x60) >> 5;
            constexpr uint8_t offset = i & 0x1F;
            table[i] = &ARM7TDMI::THUMBmoveShiftedRegister<opcode,offset>;
        } else if((i & 0b1110000000) == 0b0110000000) {
            constexpr uint8_t opcode = (i & 0x60) >> 5;
            constexpr uint32_t nn = i & 0x1F;
            table[i] = &ARM7TDMI::THUMBloadStoreImmOffset<opcode,nn>;
        } else
            table[i] = &ARM7TDMI::THUMBemptyInstruction;
        });

    return table;
}

auto armLut = generateArmLut();
auto thumbLut = generateThumbLut();