#include <cctype>
#include <string.h>
#include "GBA.hpp"

struct {
    uint8_t jit; // not implemented
} options;

bool parseArguments(uint64_t argc, char* argv[]) {
    if(argc < 2)
        return 0;
    
    for(uint64_t i = 0; i < argc; i++) {
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
        if(!gba.loadRom(fileName))
            return;

        if(options.jit) { // GBA JIT
            // no implementation yet
            return;
        } else { // GBA interpreter
            gba.arm7tdmi->fillARM();
            gba.arm7tdmi->fillTHUMB();

            while(true) {
                gba.interpretARM();
            }
        }

    } else if(fileExtension == ".nds") {
        // no implementation yet
        return;
    }
}

int main(int argc, char* argv[]) {
    if(parseArguments(argc,argv))
        runProgram(argv[2]);
    std::cout << "Error with arguments - format: [executable] [-options] [rom]\n";
    return 0;
}