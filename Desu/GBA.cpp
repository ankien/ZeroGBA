#include "GBA.hpp"

GBA::GBA() {
    systemMemory = new GBAMemory();
    systemMemory->interrupts = &interrupts;

    // CPU init
    arm7tdmi.interrupts = &interrupts;
    arm7tdmi.systemMemory = systemMemory;
    arm7tdmi.scheduler = &scheduler;
    arm7tdmi.cpuState.mode = 0x1F;

    // Skip BIOS (for debugging)
    arm7tdmi.cpuState.r[15] = 0x8000000;
    arm7tdmi.cpuState.setReg(13,0x3007F00);
    arm7tdmi.cpuState.setBankedReg(IRQ,0,0x3007FA0);
    arm7tdmi.cpuState.setBankedReg(Supervisor,0,0x3007FE0);

    // Interrupts init
    interrupts.scheduler = &scheduler;
    interrupts.cpuState = &arm7tdmi.cpuState;
    interrupts.IORegisters = systemMemory->IORegisters;

    // LCD init
    lcd.systemMemory = systemMemory;

    // Keypad init
    keypad.systemMemory = systemMemory;
    keypad.window = lcd.window;
    keypad.width = (lcd.WIDTH * lcd.SCALE); keypad.height = (lcd.HEIGHT * lcd.SCALE);
    systemMemory->IORegisters[0x130] = 0xFF;
    systemMemory->IORegisters[0x131] = 3;

    // Initialize scheduler events
    scheduler.addEventToBack(startHBlank,960,1);
    scheduler.addEventToBack(endHBlank,1232,1);
    scheduler.addEventToBack(startVBlank,197120,1);
    scheduler.addEventToBack(postFrame,280896,1);
    scheduler.getInitialEventList();
}

// todo: DMA channels, audio channels, PPU, and timers
void GBA::interpretARM() {
    uint32_t instruction = arm7tdmi.systemMemory->readWord(arm7tdmi.cpuState.r[15]);

    if(arm7tdmi.checkCond(instruction & 0xF0000000)) {
        uint16_t armIndex = arm7tdmi.fetchARMIndex(instruction);
        (arm7tdmi.*(arm7tdmi.armTable[armIndex]))(instruction);
        #if defined(PRINT_INSTR)
            printf(" %X\n",instruction); // debug
        #endif
    } else
        arm7tdmi.cpuState.r[15]+=4;
    scheduler.cyclesPassedSinceLastFrame += arm7tdmi.cycleTicks;
}
void GBA::interpretTHUMB() {
    uint16_t instruction = arm7tdmi.systemMemory->readHalfWord(arm7tdmi.cpuState.r[15]);

    uint8_t thumbIndex = arm7tdmi.fetchTHUMBIndex(instruction);
    (arm7tdmi.*(arm7tdmi.thumbTable[thumbIndex]))(instruction);
    #if defined(PRINT_INSTR)
        printf(" %X\n",instruction); // debug
    #endif
    scheduler.cyclesPassedSinceLastFrame += arm7tdmi.cycleTicks;
}

// todo: not really urgent, but change this to use regex
bool GBA::parseArguments(uint64_t argc, char* argv[]) {
    if(argc < 2)
        return 0;
    
    if(argc > 2)
        for(uint64_t i = 1; i < (argc - 1); i++) {
            if(strcmp(argv[i], "-j") == 0);
                // do shit
            else
                return 0;
        }

    return 1;
}
void GBA::run(char* fileName) {
    std::string fileExtension = std::filesystem::path(fileName).extension().string();
    std::transform(fileExtension.begin(),fileExtension.end(),fileExtension.begin(),[](char c){return std::tolower(c);});
    
    if(fileExtension == ".gba") {
       // load GBA game
        if(!systemMemory->loadRom(fileName))
            return;

        if(runtimeOptions.jit) { // GBA JIT
            // no implementation yet
            return;
        } else { // GBA interpreter
            arm7tdmi.fillARM();
            arm7tdmi.fillTHUMB();
            #if defined(TRACE)
                static uint64_t traceAmount = 0;
                FILE* traceFile = fopen("log.txt","wb");
            #endif


            while(keypad.running) {

                // check x3 for strange interrupt
                if(arm7tdmi.cpuState.r[15] == 0x03000140)
                    printf("Hello! I am a culprit instruction.");
                //for(int i = 0; i < 16; i++)
                //if(arm7tdmi.r[i] == 0x1e06067e)
                    //printf("Hello! I am a culprit register.\n");
                //uint32_t oldPC = arm7tdmi.r[15]; // for debugging

                #if defined(TRACE)
                if(traceAmount < TRACE) {
                    for(uint8_t j = 0; j < 16; j++) {
                        if(j == 15)
                            arm7tdmi.cpuState.state ? fprintf(traceFile, "%08X ", arm7tdmi.cpuState.r[j] + 2) : fprintf(traceFile, "%08X ", arm7tdmi.cpuState.r[j] + 4);
                        else
                            fprintf(traceFile, "%08X ", arm7tdmi.cpuState.r[j]);
                    }
                    fprintf(traceFile, "cpsr: %08X\n", arm7tdmi.cpuState.getCPSR());
                    traceAmount++;
                } else {
                    fclose(traceFile);
                    exit(0);
                }
                #endif

                // todo: make it so that we don't have to check what state we're in every instruction and alignment
                // also todo: implement cpu halt mode (swi 0x20) for performance
                if(arm7tdmi.cpuState.state)
                    interpretTHUMB();
                else
                    interpretARM();

                if(arm7tdmi.cpuState.state)
                    arm7tdmi.cpuState.r[15] &= ~1;
                else
                    arm7tdmi.cpuState.r[15] &= ~2;

                scheduler.step();
            }
        }

    } else
        std::cout << "Invalid ROM\n";
}