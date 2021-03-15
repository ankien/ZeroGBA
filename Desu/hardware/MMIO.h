#pragma once

/// LCD ///
#define DISPCNT_MODE (systemMemory->IORegisters[0] & 0x7)
#define DISPCNT_CGB (systemMemory->IORegisters[0] & 0x8)
#define DISPCNT_DISPLAY_FRAME_SELECT (systemMemory->IORegisters[0] & 0x10)
#define DISPCNT_HBLANK_INTERVAL_FREE (systemMemory->IORegisters[0] & 0x20)
#define DISPCNT_OBJ_VRAM_MAPPING (systemMemory->IORegisters[0] & 0x40)
#define DISPCNT_FORCED_BLANK (systemMemory->IORegisters[0] & 0x80)
#define DISPCNT_BG0 (systemMemory->IORegisters[1] & 0x1)
#define DISPCNT_BG1 (systemMemory->IORegisters[1] & 0x2)
#define DISPCNT_BG2 (systemMemory->IORegisters[1] & 0x4)
#define DISPCNT_BG3 (systemMemory->IORegisters[1] & 0x8)
#define DISPCNT_OBJ (systemMemory->IORegisters[1] & 0x10)
#define DISPCNT_WIN0 (systemMemory->IORegisters[1] & 0x20)
#define DISPCNT_WIN1 (systemMemory->IORegisters[1] & 0x40)

#define DISPSTAT_VBLANK_FLAG (systemMemory->IORegisters[4] & 0x1)
#define DISPSTAT_HBLANK_FLAG (systemMemory->IORegisters[4] & 0x2)
#define DISPSTAT_VCOUNTER_FLAG (systemMemory->IORegisters[4] & 0x4)
#define DISPSTAT_VBLANK_IRQ (systemMemory->IORegisters[4] & 0x8)
#define DISPSTAT_HBLANK_IRQ (systemMemory->IORegisters[4] & 0x10)
#define DISPSTAT_VCOUNT_IRQ (systemMemory->IORegisters[4] & 0x20)
#define DISPSTAT_VCOUNT_SETTING (systemMemory->IORegisters[5] & 0xFF)

#define VCOUNT (systemMemory->IORegisters[6] & 0xFF)

#define BG0CNT_BG_PRIORITY (systemMemory->IORegisters[8] & 0x3)
#define BG0CNT_CHARACTER_BASE_BLOCK ((systemMemory->IORegisters[8] & 0xC) >> 2)
#define BG0CNT_MOSAIC (systemMemory->IORegisters[8] & 0x40)
#define BG0CNT_COLOR_PALETTES (systemMemory->IORegisters[8] & 0x80)
#define BG0CNT_SCREEN_BASE_BLOCK (systemMemory->IORegisters[9] & 0x1F)
#define BG0CNT_DISPLAY_AREA_OVERFLOW (systemMemory->IORegisters[9] & 0x20)
#define BG0CNT_SCREEN_SIZE ((systemMemory->IORegisters[9] & 0xC0) >> 6)

#define BG1CNT_BG_PRIORITY (systemMemory->IORegisters[0xA] & 0x3)
#define BG1CNT_CHARACTER_BASE_BLOCK ((systemMemory->IORegisters[0xA] & 0xC) >> 2)
#define BG1CNT_MOSAIC (systemMemory->IORegisters[0xA] & 0x40)
#define BG1CNT_COLOR_PALETTES (systemMemory->IORegisters[0xA] & 0x80)
#define BG1CNT_SCREEN_BASE_BLOCK (systemMemory->IORegisters[0xB] & 0x1F)
#define BG1CNT_DISPLAY_AREA_OVERFLOW (systemMemory->IORegisters[0xB] & 0x20)
#define BG1CNT_SCREEN_SIZE ((systemMemory->IORegisters[0xB] & 0xC0) >> 6)

#define BG2CNT_BG_PRIORITY (systemMemory->IORegisters[0xC] & 0x3)
#define BG2CNT_CHARACTER_BASE_BLOCK ((systemMemory->IORegisters[0xC] & 0xC) >> 2)
#define BG2CNT_MOSAIC (systemMemory->IORegisters[0xC] & 0x40)
#define BG2CNT_COLOR_PALETTES (systemMemory->IORegisters[0xC] & 0x80)
#define BG2CNT_SCREEN_BASE_BLOCK (systemMemory->IORegisters[0xD] & 0x1F)
#define BG2CNT_DISPLAY_AREA_OVERFLOW (systemMemory->IORegisters[0xD] & 0x20)
#define BG2CNT_SCREEN_SIZE ((systemMemory->IORegisters[0xD] & 0xC0) >> 6)

#define BG3CNT_BG_PRIORITY (systemMemory->IORegisters[0xE] & 0x3)
#define BG3CNT_CHARACTER_BASE_BLOCK ((systemMemory->IORegisters[0xE] & 0xC) >> 2)
#define BG3CNT_MOSAIC (systemMemory->IORegisters[0xE] & 0x40)
#define BG3CNT_COLOR_PALETTES (systemMemory->IORegisters[0xE] & 0x80)
#define BG3CNT_SCREEN_BASE_BLOCK (systemMemory->IORegisters[0xF] & 0x1F00)
#define BG3CNT_DISPLAY_AREA_OVERFLOW (systemMemory->IORegisters[0xF] & 0x20)
#define BG3CNT_SCREEN_SIZE ((systemMemory->IORegisters[0xF] & 0xC0) >> 6)

#define BG0HOFS (systemMemory->IORegisters[0x10] & 0x1FF)
#define BG0VOFS (systemMemory->IORegisters[0x12] & 0x1FF)
#define BG1HOFS (systemMemory->IORegisters[0x14] & 0x1FF)
#define BG1VOFS (systemMemory->IORegisters[0x16] & 0x1FF)
#define BG2HOFS (systemMemory->IORegisters[0x18] & 0x1FF)
#define BG2VOFS (systemMemory->IORegisters[0x1A] & 0x1FF)
#define BG3HOFS (systemMemory->IORegisters[0x1C] & 0x1FF)
#define BG3VOFS (systemMemory->IORegisters[0x1E] & 0x1FF)

#define WIN0H_X2 (systemMemory->IORegisters[0x40] & 0xFF)
#define WIN0H_X1 (systemMemory->IORegisters[0x41] & 0xFF)
#define WIN1H_X2 (systemMemory->IORegisters[0x42] & 0xFF)
#define WIN1H_X1 (systemMemory->IORegisters[0x43] & 0xFF)
#define WIN0V_Y2 (systemMemory->IORegisters[0x44] & 0xFF)
#define WIN0V_Y1 (systemMemory->IORegisters[0x45] & 0xFF)
#define WIN1V_Y2 (systemMemory->IORegisters[0x46] & 0xFF)
#define WIN1V_Y1 (systemMemory->IORegisters[0x47] & 0xFF)

#define WININ_WIN0_BG_ENABLE_BITS (systemMemory->IORegisters[0x48] & 0xF)
#define WININ_WIN0_OBJ_ENABLE_BIT (systemMemory->IORegisters[0x48] & 0x10)
#define WININ_WIN0_COLOR_SPECIAL_EFFECT (systemMemory->IORegisters[0x48] & 0x20)
#define WININ_WIN1_BG_ENABLE_BITS ((systemMemory->IORegisters[0x49] & 0xF0) >> 4)
#define WININ_WIN1_OBJ_ENABLE_BIT (systemMemory->IORegisters[0x49] & 0x10)
#define WININ_WIN1_COLOR_SPECIAL_EFFECT (systemMemory->IORegisters[0x49] & 0x20)

#define MOSAIC_BG_HSIZE (systemMemory->IORegisters[0x4C] & 0xF)
#define MOSAIC_BG_VSIZE ((systemMemory->IORegisters[0x4C] & 0xF0) >> 4)
#define MOSAIC_OBJ_HSIZE (systemMemory->IORegisters[0x4D] & 0xF)
#define MOSAIC_OBJ_VSIZE ((systemMemory->IORegisters[0x4D] & 0xF0) >> 4)

#define BLDCNT_BG0_TP1 (systemMemory->IORegisters[0x50] & 0x1)
#define BLDCNT_BG1_TP1 (systemMemory->IORegisters[0x50] & 0x2)
#define BLDCNT_BG2_TP1 (systemMemory->IORegisters[0x50] & 0x4)
#define BLDCNT_BG3_TP1 (systemMemory->IORegisters[0x50] & 0x8)
#define BLDCNT_OBJ_TP1 (systemMemory->IORegisters[0x50] & 0x10)
#define BLDCNT_BD_TP1 (systemMemory->IORegisters[0x50] & 0x20)
#define BLDCNT_COLOR_SP ((systemMemory->IORegisters[0x50] & 0xC0) >> 6)
#define BLDCNT_BG0_TP2 (systemMemory->IORegisters[0x51] & 0x1)
#define BLDCNT_BG1_TP2 (systemMemory->IORegisters[0x51] & 0x2)
#define BLDCNT_BG2_TP2 (systemMemory->IORegisters[0x51] & 0x4)
#define BLDCNT_BG3_TP2 (systemMemory->IORegisters[0x51] & 0x8)
#define BLDCNT_OBJ_TP2 (systemMemory->IORegisters[0x51] & 0x10)
#define BLDCNT_BD_TP2 (systemMemory->IORegisters[0x51] & 0x20)

#define BLDALPHA_EVA (systemMemory->IORegisters[0x52] & 0x1F)
#define BLDALPHA_EVB (systemMemory->IORegisters[0x52] & 0x1F)

#define BLDY_EVY (systemMemory->IORegisters[0x54] & 0x1F)

/// Interrupt Control ///
#define IE_VBLANK (systemMemory->IORegisters[0x200] & 0x1)
#define IE_HBLANK (systemMemory->IORegisters[0x200] & 0x2)
#define IE_VCOUNTER (systemMemory->IORegisters[0x200] & 0x4)
#define IE_TIMER0 (systemMemory->IORegisters[0x200] & 0x8)
#define IE_TIMER1 (systemMemory->IORegisters[0x200] & 0x10)
#define IE_TIMER2 (systemMemory->IORegisters[0x200] & 0x20)
#define IE_TIMER3 (systemMemory->IORegisters[0x200] & 0x40)
#define IE_SERIAL (systemMemory->IORegisters[0x200] & 0x80)
#define IE_DMA0 (systemMemory->IORegisters[0x201] & 0x1)
#define IE_DMA1 (systemMemory->IORegisters[0x201] & 0x2)
#define IE_DMA2 (systemMemory->IORegisters[0x201] & 0x4)
#define IE_DMA3 (systemMemory->IORegisters[0x201] & 0x8)
#define IE_KEYPAD (systemMemory->IORegisters[0x201] & 0x10)
#define IE_GPAK (systemMemory->IORegisters[0x201] & 0x20)

#define IF_VBLANK (systemMemory->IORegisters[0x202] & 0x1)
#define IF_HBLANK (systemMemory->IORegisters[0x202] & 0x2)
#define IF_VCOUNTER (systemMemory->IORegisters[0x202] & 0x4)
#define IF_TIMER0 (systemMemory->IORegisters[0x202] & 0x8)
#define IF_TIMER1 (systemMemory->IORegisters[0x202] & 0x10)
#define IF_TIMER2 (systemMemory->IORegisters[0x202] & 0x20)
#define IF_TIMER3 (systemMemory->IORegisters[0x202] & 0x40)
#define IF_SERIAL (systemMemory->IORegisters[0x202] & 0x80)
#define IF_DMA0 (systemMemory->IORegisters[0x203] & 0x1)
#define IF_DMA1 (systemMemory->IORegisters[0x203] & 0x2)
#define IF_DMA2 (systemMemory->IORegisters[0x203] & 0x4)
#define IF_DMA3 (systemMemory->IORegisters[0x203] & 0x8)
#define IF_KEYPAD (systemMemory->IORegisters[0x203] & 0x10)
#define IF_GPAK (systemMemory->IORegisters[0x203] & 0x20)

#define IME (systemMemory->IORegisters[0x208] & 0x1)