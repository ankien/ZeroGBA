#include "GBA.hpp"

GBA::GBA() {
    // Memory init
    systemMemory = new GBAMemory();
    systemMemory->memoryArray<uint16_t>(0x4000204) = 0x4317;
    systemMemory->interrupts = &interrupts;
    systemMemory->cpuState = &arm7tdmi.cpuState;

    // CPU init
    arm7tdmi.interrupts = &interrupts;
    arm7tdmi.systemMemory = systemMemory;
    arm7tdmi.scheduler = &scheduler;
    arm7tdmi.cpuState.mode = 0x1F;
    arm7tdmi.cpuState.stateRelativeToBios = &systemMemory->stateRelativeToBios;
    
    // startup register values
    arm7tdmi.cpuState.r[13] = 0x3007F00;
    arm7tdmi.cpuState.setBankedReg(IRQ,0,0x3007FA0);
    arm7tdmi.cpuState.setBankedReg(Supervisor,0,0x3007FE0);

    // Skip BIOS
    arm7tdmi.cpuState.r[15] = 0x8000000;

    // Interrupts init
    interrupts.scheduler = &scheduler;
    interrupts.cpuState = &arm7tdmi.cpuState;
    interrupts.IORegisters = systemMemory->IORegisters;
    interrupts.internalTimer = systemMemory->internalTimer;

    // LCD init
    lcd.systemMemory = systemMemory;

    // Keypad init
    keypad.systemMemory = systemMemory;
    keypad.window = lcd.window;
    keypad.initialWidth = (lcd.WIDTH * lcd.SCALE); keypad.initialHeight = (lcd.HEIGHT * lcd.SCALE);
    systemMemory->IORegisters[0x130] = 0xFF;
    systemMemory->IORegisters[0x131] = 3;

    // Initialize scheduler events
    scheduler.eventList = {
        {startHBlank,Scheduler::GenericRescheduable,960,1},
        {endHBlank,Scheduler::GenericRescheduable,1232,1},
        {startVBlank,Scheduler::GenericRescheduable,197120,1},
        {postFrame,Scheduler::GenericRescheduable,280896,1}
    };
}

void GBA::interpretARM() {
    uint32_t instruction = systemMemory->memoryArray<uint32_t>(arm7tdmi.cpuState.r[15]);

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
    uint16_t instruction = systemMemory->memoryArray<uint16_t>(arm7tdmi.cpuState.r[15]);

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
    std::string fileNameString(fileName);
    std::string fileExtension = std::filesystem::path(fileName).extension().string();
    std::transform(fileExtension.begin(),fileExtension.end(),fileExtension.begin(),[](char c){return std::tolower(c);});
    
    if(fileExtension == ".gba") {
       // load GBA game
        if(!systemMemory->loadRom(fileNameString))
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

                #ifdef DEBUG_VARS
                uint32_t oldPC = arm7tdmi.cpuState.r[15];
                if(arm7tdmi.cpuState.r[15] == 0x080039fe)
                    printf("fug");
                #endif

                // todo: buffer this output or use MIO so we don't destroy our hdd with a billion calls
                #if defined(TRACE)
                if(systemMemory->tracing && traceAmount < TRACE) {
                    for(uint8_t j = 0; j < 16; j++) {
                        if(j == 15)
                            arm7tdmi.cpuState.state ? fprintf(traceFile, "%08X ", arm7tdmi.cpuState.r[j] + 2) : fprintf(traceFile, "%08X ", arm7tdmi.cpuState.r[j] + 4);
                        else
                            fprintf(traceFile, "%08X ", arm7tdmi.cpuState.r[j]);
                    }
                    fprintf(traceFile, "cpsr: %08X\n", arm7tdmi.cpuState.getCPSR());
                    traceAmount++;
                } else if(traceAmount >= TRACE) {
                    fclose(traceFile);
                    exit(0);
                }
                #endif

                // todo: make it so that we don't have to check what state we're in every instruction and alignment
                if(arm7tdmi.cpuState.state)
                    interpretTHUMB();
                else
                    interpretARM();

                #ifdef DEBUG_VARS
                instrCount++;
                #endif


                if(arm7tdmi.cpuState.state)
                    arm7tdmi.cpuState.r[15] &= ~1;
                else
                    arm7tdmi.cpuState.r[15] &= ~3;

                scheduler.step();
            }
        }

    } else
        std::cout << "Invalid ROM\n";
}