#pragma once
#include <string.h>
#include <stdint.h>

struct ARM7TDMI {
    // cycles per second
    static const uint32_t CLOCK_RATE = 16780000;
    static const uint32_t DS_CLOCK_RATE = 33000000;

    // lookup tables, array size is the different number of instructions
    void (*armTable[4096])();
    void (*thumbTable[256])();

    // might not end up using this, these are the I/O registers in I/O Map
    struct {
        uint32_t DISPCNT:16, DISPSTAT:16, VCOUNT:16, BG0CNT:16, BG1CNT:16, BG3CNT:16, BG0HOFS:16, BG0VOFS:16, BG1HOFS:16,
                BG1VOFS:16, BG2HOFS:16, BG2VOFS:16, BG3HOFS:16, BG3VOFS:16, BG2PA:16, BG2PB:16,
                BG2PC:16, BG2PD:16, BG2X:32, BG2Y:32, BG3PA:16, BG3PB:16, BG3PC:16, BG3PD:16, BG3X:32,
                BG3Y:32, WIN0H:16, WIN1H:16, WIN0V:16, WIN1V:16, WIN1N:16, WINOUT:16, MOSAIC:16,
                BLDCNT:16, BLDALPHA:16, BLDY:16, SOUND1CNT_L:16, SOUND1CNT_H:16, SOUND1CNT_X:16,
                SOUND2CNT_L:16, SOUND2CNT_H:16, SOUND3CNT_L:16, SOUND3CNT_H:16, SOUND3CNT_X:16,
                SOUND4CNT_L:16, SOUND4CNT_H:16, SOUNDCNT_L:16, SOUNDCNT_H:16, SOUNDCNT_X:16, SOUNDBIAS:16,
                WAVE_RAM1:16, WAVE_RAM2:16, FIFO_A:32, FIFO_B:32, DMA0SAD:32, DMA0DAD:32, DMA0CNT_L:16, DMA0CNT_H:16,
                DMA1SAD:32, DMA1DAD:32, DMA1CNT_L:16, DMA1CNT_H:16, DMA2SAD:32, DMA2DAD:32, DMA2CNT_L:16,
                DMA2CNT_H:16, DMA3SAD:32, DMA3DAD:32, DMA3CNT_L:16, DMA3CNT_H:16, TM0CNT_L:16, TM0CNT_H:16,
                TM1CNT_L:16, TM1CNT_H:16, TM2CNT_L:16, TM2CNT_H:16, TM3CNT_L:16, TM3CNT_H:16,
                SIODATA32:32, SIOMULTI0:16, SIOMULTI1:16, SIOMULTI2:16, SIOMULTI3:16, SIOCNT:16, SIOMLT_SEND:16,
                SIODATA8:16, KEYINPUT:16, KEYCNT:16, RCNT:16, JOYCNT:16, JOY_RECV:32, JOY_TRANS:32, JOYSTAT:16,
                IE:16, IF:16, WAITCNT:16, IME:16, POSTFLG:8, HALTCNT:8;
    } IOmap;

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
    uint32_t cpsr;
    uint32_t spsr[6]; // N/A, fiq, svc, abt, irq, und

    void handleException(uint8_t, uint32_t, uint8_t);
    void fillARM(uint8_t[]);
    void fillTHUMB(uint8_t[]);
    void interpretARMCycle();
    void interpretTHUMBCycle();
    uint8_t getModeIndex(uint8_t);
};