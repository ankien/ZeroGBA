#include "GBA.hpp"

GBA::GBA() {
    memory = new GBAMemory();
    memory->bios = (uint8_t*)calloc(0x4000,sizeof(uint8_t));
    memory->wramOnBoard = (uint8_t*)calloc(0x40000,sizeof(uint8_t));
    memory->wramOnChip = (uint8_t*)calloc(0x8000,sizeof(uint8_t));
    memory->IORegisters = (uint8_t*)calloc(0x3FF,sizeof(uint8_t));
    memory->pram = (uint8_t*)calloc(0x400,sizeof(uint8_t));
    memory->vram = (uint8_t*)calloc(0x18000,sizeof(uint8_t));
    memory->oam = (uint8_t*)calloc(0x400,sizeof(uint8_t));
    memory->gamePak = (uint8_t*)calloc(0x3000000,sizeof(uint8_t));
    memory->gPakSram = (uint8_t*)calloc(0x10000,sizeof(uint8_t));

    arm7tdmi.systemMemory = memory;
    // skip the bios, todo: implement everything in order to load it correctly
    arm7tdmi.pc = 0x8000000;
    arm7tdmi.setArrayIndex(0,0xCA5);
    arm7tdmi.setCPSR(0x1F);
    arm7tdmi.setModeArrayIndex(arm7tdmi.System,13,0x3007F00);
    arm7tdmi.setModeArrayIndex(arm7tdmi.IRQ,13,0x3007FA0);
    arm7tdmi.setModeArrayIndex(arm7tdmi.Supervisor,13,0x3007FE0);
    arm7tdmi.setModeArrayIndex(arm7tdmi.System,14,0x8000000);

    lcd.systemMemory = memory;

    keypad.systemMemory = memory;
    keypad.window = lcd.window;
    keypad.width = (lcd.WIDTH * lcd.SCALE); keypad.height = (lcd.HEIGHT * lcd.SCALE);
    memory->IORegisters[0x130] = 0xFF;
    memory->IORegisters[0x131] = 3;
    //memory->IORegisters[0x132] = ;
}

// there's a pipeline, DMA channels, audio channels, PPU, and timers too?
// i think i can fake the pipeline
void GBA::interpretARM() {
    uint32_t instruction = ((*memory)[arm7tdmi.pc + 3] << 24) |
        ((*memory)[arm7tdmi.pc + 2] << 16) |
        ((*memory)[arm7tdmi.pc + 1] << 8) |
        (*memory)[arm7tdmi.pc];

    if(arm7tdmi.checkCond(instruction & 0xF0000000)) {
        uint16_t armIndex = arm7tdmi.fetchARMIndex(instruction);
        (arm7tdmi.*(arm7tdmi.armTable[armIndex]))(instruction);
        #if defined(PRINT_INSTR)
            printf(" %X\n",instruction); // debug
        #endif
        cyclesPassed += arm7tdmi.cycleTicks;
        cyclesSinceHBlank += arm7tdmi.cycleTicks;
    } else
        arm7tdmi.pc+=4;
}
void GBA::interpretTHUMB() {
    uint16_t instruction = ((*memory)[arm7tdmi.pc+1] << 8) |
                            (*memory)[arm7tdmi.pc];

    uint8_t thumbIndex = arm7tdmi.fetchTHUMBIndex(instruction);
    (arm7tdmi.*(arm7tdmi.thumbTable[thumbIndex]))(instruction);
    #if defined(PRINT_INSTR)
        printf(" %X\n",instruction); // debug
    #endif
    cyclesPassed += arm7tdmi.cycleTicks;
    cyclesSinceHBlank += arm7tdmi.cycleTicks;
}

// todo: not really urgent, but change this to use regex
// also, could turn it into a lambda since we only need it once
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
        if(!memory->loadRom(fileName))
            return;

        if(runtimeOptions.jit) { // GBA JIT
            // no implementation yet
            return;
        } else { // GBA interpreter
            arm7tdmi.fillARM();
            arm7tdmi.fillTHUMB();
            // set runtime options here

            while(keypad.running) {

                while(cyclesPassed < 280896) {
                    // for debug breakpoints
                    //if(arm7tdmi.pc == 0x08031672)
                        //printf("Hello! I am a culprit instruction.\n");
                    //if(arm7tdmi.reg[1] == 0x6164B7AA)
                        //printf("Hello! I am a culprit register.\n");
                    uint32_t oldPC = arm7tdmi.pc; // for debugging

                    if(arm7tdmi.state)
                        interpretTHUMB();
                    else
                        interpretARM();

                    // align addresses
                    if(arm7tdmi.state)
                        arm7tdmi.pc &= ~1;
                    else
                        arm7tdmi.pc &= ~2;

                    if((cyclesSinceHBlank >= 960) && !(memory->IORegisters[4] & 0x2)) { // scan and draw line from framebuffer
                        lcd.fetchScanline(); // draw visible line
                        memory->IORegisters[4] |= 0x2; // turn on hblank
                    } else if(cyclesSinceHBlank >= 1232) {
                        memory->IORegisters[4] ^= 0x2; // turn off hblank
                        cyclesSinceHBlank -= 1232;
                    }

                    if(cyclesPassed >= 197120)
                        memory->IORegisters[4] |= 0x1; // set vblank
                    else
                        memory->IORegisters[4] ^= 0x1;; // disable vblank
                }

                if(cyclesPassed > 280896)
                    cyclesPassed -= 280896;
                else
                    cyclesPassed = 0;

                cyclesSinceHBlank = cyclesPassed; // keep other cycle counters in sync with system
                lcd.draw();

                keypad.pollInputs();

                if(keypad.notSkippingFrames)
                    SDL_Delay(16 - ((lcd.currMillseconds - lcd.millisecondsElapsed) % 16)); // roughly 1000ms / 60fps - delay since start of last frame draw
            }
        }

    } else
        std::cout << "Invalid ROM\n";
}