#pragma once
#include <cstdint>

struct DMA {
    
    const uint32_t sourceAddressMasks[4] = {0x07FFFFFF,0x0FFFFFFF,0x0FFFFFFF,0x0FFFFFFF}; 
    const uint32_t destAddressMasks[4] = {0x07FFFFFF,0x07FFFFFF,0x07FFFFFF,0x0FFFFFFF};
    const uint16_t lengthMasks[4] = {0x3FFF,0x3FFF,0x3FFF,0xFFFF};


};