#pragma once

inline uint32_t CPUState::getCPSR() {
    return mode |
           (state << 5) |
           (fiqDisable << 6) |
           (irqDisable << 7) |
           (reserved << 8) |
           (stickyOverflow << 27) |
           (overflowFlag << 28) |
           (carryFlag << 29) |
           (zeroFlag << 30) |
           (signFlag << 31);
}
inline void CPUState::setCPSR(uint32_t num) {
    mode = (num & 0x1F);
    state = (num & 0x20) >> 5;
    fiqDisable = (num & 0x40) >> 6;
    irqDisable = (num & 0x80) >> 7;
    reserved = (num & 0x7FFFF00) >> 8;
    stickyOverflow = (num & 0x8000000) >> 27;
    overflowFlag = (num & 0x10000000) >> 28;
    carryFlag = (num & 0x20000000) >> 29;
    zeroFlag = (num & 0x40000000) >> 30;
    signFlag = (num & 0x80000000) >> 31;
}
inline void CPUState::switchMode(uint8_t newMode) {

    // because this can happen in an msr apparently
    if(newMode == mode)
        return;

    // bank current regs
    switch(mode) {
        case User:
        case System:
            for(uint8_t i = 8; i < 15; i++)
                sysUserReg[i-8] = r[i];
            break;
        case FIQ:
            for(uint8_t i = 8; i < 15; i++)
                fiqReg[i-8] = r[i];
            fiqReg[7] = getCPSR();
            break;
        case Supervisor:
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                svcReg[i-13] = r[i];
            svcReg[2] = getCPSR();
            break;
        case Abort:
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                abtReg[i-13] = r[i];
            abtReg[2] = getCPSR();
            break;
        case IRQ:
            *stateRelativeToBios = 2;
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                irqReg[i-13] = r[i];
            irqReg[2] = getCPSR();
            break;
        case Undefined:
            for(uint8_t i = 8; i < 13; i++)
                sysUserReg[i-8] = r[i];
            for(uint8_t i = 13; i < 15; i++)
                undReg[i-13] = r[i];
            undReg[2] = getCPSR();
    }

    mode = newMode;

    // get new regs
    switch(newMode) {
        case User:
        case System:
            for(uint8_t i = 8; i < 15; i++)
                r[i] = sysUserReg[i-8];
            break;
        case FIQ:
            for(uint8_t i = 8; i < 15; i++)
                r[i] = fiqReg[i-8];
            setCPSR(fiqReg[7]);
            break;
        case Supervisor:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = svcReg[i-13];
            setCPSR(svcReg[2]);
            break;
        case Abort:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = abtReg[i-13];
            setCPSR(abtReg[2]);
            break;
        case IRQ:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = irqReg[i-13];
            setCPSR(irqReg[2]);
            break;
        case Undefined:
            for(uint8_t i = 8; i < 13; i++)
                r[i] = sysUserReg[i-8];
            for(uint8_t i = 13; i < 15; i++)
                r[i] = undReg[i-13];
            setCPSR(undReg[2]);
            break;
    }
}
inline uint32_t CPUState::getBankedReg(uint8_t mode, uint8_t reg) {
    if(reg == 'S')
        mode == FIQ ? reg = 7 : reg = 2;
    switch(mode) {
        case User:
        case System:
            return sysUserReg[reg];
            break;
        case FIQ:
            return fiqReg[reg];
            break;
        case Supervisor:
            return svcReg[reg];
            break;
        case Abort:
            return abtReg[reg];
            break;
        case IRQ:
            return irqReg[reg];
            break;
        case Undefined:
            return undReg[reg];
    }
}
inline void CPUState::setBankedReg(uint8_t mode, uint8_t reg, uint32_t arg) {
    if(reg == 'S')
        mode == FIQ ? reg = 7 : reg = 2;
    switch(mode) {
        case User:
        case System:
            sysUserReg[reg] = arg;
            break;
        case FIQ:
            fiqReg[reg] = arg;
            break;
        case Supervisor:
            svcReg[reg] = arg;
            break;
        case Abort:
            abtReg[reg] = arg;
            break;
        case IRQ:
            irqReg[reg] = arg;
            break;
        case Undefined:
            undReg[reg] = arg;
    }
}
inline uint32_t CPUState::getReg(uint8_t reg) {
    return r[reg];
}
inline void CPUState::setReg(uint8_t reg, uint32_t arg) {
    r[reg] = arg;
}
inline uint32_t CPUState::getSPSR(uint8_t mode) {
    switch(mode) {
        case FIQ:
            return fiqReg[7];
        case Supervisor:
            return svcReg[2];
        case Abort:
            return abtReg[2];
        case IRQ:
            return irqReg[2];
        case Undefined:
            return undReg[2];
    }
}
inline void CPUState::setSPSR(uint8_t mode,uint32_t arg) {
    switch(mode) {
        case FIQ:
            fiqReg[7] = arg;
            break;
        case Supervisor:
            svcReg[2] = arg;
            break;
        case Abort:
            abtReg[2] = arg;
            break;
        case IRQ:
            irqReg[2] = arg;
            break;
        case Undefined:
            undReg[2] = arg;
            break;
    }
}

inline void CPUState::handleException(uint8_t exception, int8_t nn, uint8_t newMode) {
    if(newMode != mode) {
        setBankedReg(newMode,1,r[15]+nn); // save old PC
        setBankedReg(newMode,'S',getCPSR()); // save old CPSR
    } else {
        // just set the current registers
        r[14] = r[15]+nn; // don't need to set banked LR since nothing can read that anyways
        setBankedReg(newMode,'S',getCPSR()); // don't need to set current CPSR, duh
    }
    switchMode(newMode); // switch mode
    // new bits!
    mode = newMode;
    state = 0;
    irqDisable = 1;
    
    if((newMode == Reset) || (newMode == FIQ))
        fiqDisable = 1;

    switch(newMode) {

        case Supervisor:

            switch(exception) {

                case Reset:
                    r[15] = 0x0;
                    break;
                case AddressExceeds26Bit:
                    r[15] = 0x14;
                    break;
                case SoftwareInterrupt:
                    *stateRelativeToBios = 3;
                    r[15] = 0x8;
                    break;
            }
            break;

        case Undefined:
            
            switch(exception) {
                
                case UndefinedInstruction:
                    r[15] = 0x4;
                    break;
            }
            break;

        case Abort:

            switch(exception) {

                case DataAbort:
                    r[15] = 0x10;
                    break;
                case PrefetchAbort:
                    r[15] = 0xC;
                    break;
            }
            break;

        case IRQ:
            
            switch(exception) {

                case NormalInterrupt:
                    *stateRelativeToBios = 1; // dumb open bus shit, see GBAMemory.hpp
                    r[15] = 0x18;
                    break;
            }
            break;

        case FIQ:
            
            switch(exception) {

                case FastInterrupt:
                    r[15] = 0x1C;
                    break;
            }
    }
}