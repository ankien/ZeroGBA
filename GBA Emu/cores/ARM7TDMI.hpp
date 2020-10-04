#pragma once
#include <string.h>
#include <stdint.h>

struct ARM7TDMI {
    static const uint32_t CLOCK_RATE = 16780000;
    static const uint32_t DS_CLOCK_RATE = 33000000;

    struct {
        uint8_t DISPCNT:16, DISPSTAT:16, VCOUNT:16, BG0CNT:16, BG1CNT:16, BG3CNT:16, BG0HOFS:16, BG0VOFS:16, BG1HOFS:16,
                BG1VOFS:16, BG2HOFS:16, BG2VOFS:16, BG3HOFS:16, BG3VOFS:16, BG2PA:16, BG2PB:16,
                BG2PC:16, BG2PD:16, BG2X:32, BG2Y:32, BG3PA:16, BG3PB:16, BG3PC:16, BG3PD:16, BG3X:32,
                BG3Y:32, WIN0H:16, WIN1H:16, WIN0V:16, WIN1V:16, WIN1N:16, WINOUT:16, MOSAIC:16,
                BLDCNT:16, BLDALPHA:16, BLDY:16, SOUND1CNT_L:16, SOUND1CNT_H:16, SOUND1CNT_X:16,
                SOUND2CNT_L:16, SOUND2CNT_H:16, SOUND3CNT_L:16, SOUND3CNT_H:16, SOUND3CNT_X:16,
                SOUND4CNT_L:16, SOUND4CNT_H:16, SOUNDCNT_L:16, SOUNDCNT_H:16, SOUNDCNT_X:16, SOUNDBIAS:16,
                WAVE_RAM:32 /*WAVE_RAM has 2 banks*/, FIFO_A:32, FIFO_B:32, DMA0SAD:32, DMA0DAD:32, DMA0CNT_L:16, DMA0CNT_H:16,
                DMA1SAD:32, DMA1DAD:32, DMA1CNT_L:16, DMA1CNT_H:16, DMA2SAD:32, DMA2DAD:32, DMA2CNT_L:16,
                DMA2CNT_H:16, DMA3SAD:32, DMA3DAD:32, DMA3CNT_L:16, DMA3CNT_H:16, TM0CNT_L:16, TM0CNT_H:16,
                TM1CNT_L:16, TM1CNT_H:16, TM2CNT_L:16, TM2CNT_H:16, TM3CNT_L:16, TM3CNT_H:16,
                SIODATA32:32, SIOMULTI0:16, SIOMULTI1:16, SIOMULTI2:16, SIOMULTI3:16, SIOCNT:16, SIOMLT_SEND:16,
                SIODATA8:16, KEYINPUT:16, KEYCNT:16, RCNT:16, JOYCNT:16, JOY_RECV:32, JOY_TRANS:32, JOYSTAT:16,
                IE:16, IF:16, WAITCNT:16, IME:16, POSTFLG:8, HALTCNT:8;
    } IOmap;

    /// Registers ///
    uint32_t reg[16]; // R0-15, 13-SP, 14-LR, 15-PC
    uint32_t figreg[7]; // R8-14
    uint32_t svcreg[2]; // R13-14
    uint32_t abtreg[2]; // R13-14
    uint32_t irqreg[2]; // R13-14
    uint32_t undreg[2]; // R13-14
    uint32_t cpsr = 1 << 30; // flags: 31-sign, 30-zero, 29-carry, 28-overflow, 27-sticky overflow, 26-8-reserved, 
    uint32_t spsr[5]; // fiq,svc,abt,irq,und


    void interpretCycle(,);

    ARM7TDMI(,) {

    }
};