#include <cctype>
#include <string.h>
#include <iostream>
#include "GBA.hpp"

#undef main // because SDL defines main for some reason

struct {
    uint8_t jit : 1; // not implemented
} options;

bool parseArguments(uint64_t argc, char* argv[]) {
    if(argc < 2)
        return 0;
    
    if(argc > 2)
        for(uint64_t i = 1; i < (argc - 1); i++) {
            if(strcmp(argv[i], "-j") == 0)
                options.jit = 1;
            else
                return 0;
        }

    return 1;
}

void runProgram(char* fileName) {
    std::string fileExtension = std::filesystem::path(fileName).extension().string();
    std::transform(fileExtension.begin(),fileExtension.end(),fileExtension.begin(),[](char c){return std::tolower(c);});
    
    if(fileExtension == ".gba") {
       // load GBA game
        GBA gba;
        if(!gba.memory->loadRom(fileName))
            return;

        if(options.jit) { // GBA JIT
            // no implementation yet
            return;
        } else { // GBA interpreter
            gba.arm7tdmi->fillARM();
            gba.arm7tdmi->fillTHUMB();

            while(true) {
                while(gba.cyclesPassed < 280896) {
                    if(gba.arm7tdmi->state)
                        gba.interpretTHUMB();
                    else
                        gba.interpretARM();
                }

                if(gba.cyclesPassed > 280896)
                    gba.cyclesPassed -= 280896;
                else
                    gba.cyclesPassed = 0;

                // update framebuffer here? or parts of it in doProcesses?
                gba.lcd->draw();

                SDL_Delay(16 - (SDL_GetTicks() / 1000 - gba.lcd->secondsElapsed)); // roughly 1000ms / 60fps - delay since start of last frame draw
            }
        }

    } else if(fileExtension == ".nds") {
        // no implementation yet
        return;
    } else
        std::cout << "Invalid ROM\n";
}

int main(int argc, char* argv[]) {
    if(parseArguments(argc,argv))
        runProgram(argv[argc - 1]);
    else
        std::cout << "Error with arguments - format: [executable] [-options] [rom]\n";
    return 0;
}