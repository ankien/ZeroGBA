#include "GBA.hpp"

GBA::GBA() {
    memory = new GBAMemory();

    arm7tdmi.systemMemory = memory;
    arm7tdmi.setCPSR(0x1F);

    // Skip BIOS (for debugging)
    arm7tdmi.r[15] = 0x8000000;
    arm7tdmi.setReg(13,0x3007F00);
    arm7tdmi.setBankedReg(arm7tdmi.IRQ,0,0x3007FA0);
    arm7tdmi.setBankedReg(arm7tdmi.Supervisor,0,0x3007FE0);

    lcd.systemMemory = memory;

    keypad.systemMemory = memory;
    keypad.window = lcd.window;
    keypad.width = (lcd.WIDTH * lcd.SCALE); keypad.height = (lcd.HEIGHT * lcd.SCALE);
    memory->IORegisters[0x130] = 0xFF;
    memory->IORegisters[0x131] = 3;
}

// there's a pipeline, DMA channels, audio channels, PPU, and timers too?
// i think i can fake the pipeline
void GBA::interpretARM() {
    uint32_t instruction = arm7tdmi.readWord(arm7tdmi.r[15]);

    if(arm7tdmi.checkCond(instruction & 0xF0000000)) {
        uint16_t armIndex = arm7tdmi.fetchARMIndex(instruction);
        (arm7tdmi.*(arm7tdmi.armTable[armIndex]))(instruction);
        #if defined(PRINT_INSTR)
            printf(" %X\n",instruction); // debug
        #endif
        cyclesPassed += arm7tdmi.cycleTicks;
        cyclesSinceHBlank += arm7tdmi.cycleTicks;
    } else
        arm7tdmi.r[15]+=4;
}
void GBA::interpretTHUMB() {
    uint16_t instruction = arm7tdmi.readHalfWord(arm7tdmi.r[15]);

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
            #if defined(TRACE)
                static uint64_t traceAmount = 0;
                FILE* traceFile = fopen("log.txt","wb");
            #endif


            while(keypad.running) {

                while(cyclesPassed < 280896) {
                    // for debug breakpoints, mgba is 4/2 ahead depending on ARM/THUMB state
                    // arm: t225, mgba: t225
                    // thumb: t118, mgba: t230
                    if(arm7tdmi.r[15] == 0x08000724)
                        printf("Hello! I am a culprit instruction.\n");
                    //for(int i = 0; i < 16; i++)
                    //if(arm7tdmi.r[i] == 0x1e06067e)
                        //printf("Hello! I am a culprit register.\n");
                    uint32_t oldPC = arm7tdmi.r[15]; // for debugging

                    #if defined(TRACE)
                        if(traceAmount < TRACE) {
                            for(uint8_t j = 0; j < 16; j++) {
                                if(j == 15)
                                    arm7tdmi.state ? fprintf(traceFile,"%08X ",arm7tdmi.r[j]+2) : fprintf(traceFile,"%08X ",arm7tdmi.r[j]+4);
                                else
                                    fprintf(traceFile,"%08X ",arm7tdmi.r[j]);
                            }
                            fprintf(traceFile,"cpsr: %08X\n",arm7tdmi.getCPSR());
                            traceAmount++;
                        } else {
                            fclose(traceFile);
                            exit(0);
                        }
                    #endif

                    if(arm7tdmi.state)
                        interpretTHUMB();
                    else
                        interpretARM();

                    // align addresses
                    if(arm7tdmi.state)
                        arm7tdmi.r[15] &= ~1;
                    else
                        arm7tdmi.r[15] &= ~2;

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

                // todo: implement JIT polling and run ahead - https://byuu.net/input
                keypad.pollInputs();

                // todo: make the delay (16 ms) dynamically fluctuate for 60 fps target
                if(keypad.notSkippingFrames)
                    SDL_Delay(16 - ((lcd.currMillseconds - lcd.millisecondsElapsed) % 16)); // roughly 1000ms / 60fps - delay since start of last frame draw
            }
        }

    } else
        std::cout << "Invalid ROM\n";
}