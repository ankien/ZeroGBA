#pragma once

// These are macros for commonly used MMIO regs, I define them when their use is needed.

/// LCD ///
#define DISPCNT_MODE (systemMemory->IORegisters[0] & 0x7)
#define DISPCNT_DISPLAY_FRAME_SELECT (systemMemory->IORegisters[0] & 0x10)
#define DISPCNT_BG2 (systemMemory->IORegisters[1] & 0x4)
#define DISPCNT_OBJ (systemMemory->IORegisters[1] & 0x10)

#define DISPSTAT_VBLANK_FLAG (systemMemory->IORegisters[4] & 0x1)
#define DISPSTAT_VCOUNTER_FLAG (systemMemory->IORegisters[4] & 0x4)
#define DISPSTAT_VBLANK_IRQ (systemMemory->IORegisters[4] & 0x8)
#define DISPSTAT_HBLANK_IRQ (systemMemory->IORegisters[4] & 0x10)
#define DISPSTAT_VCOUNT_IRQ (systemMemory->IORegisters[4] & 0x20)
#define DISPSTAT_VCOUNT_SETTING (systemMemory->IORegisters[5])

#define VCOUNT (systemMemory->IORegisters[6])

#define WIN0V_Y1 (systemMemory->IORegisters[0x45])
#define WIN1V_Y1 (systemMemory->IORegisters[0x47])

/// Interrupt Control ///
#define IME (systemMemory->IORegisters[0x208] & 0x1)