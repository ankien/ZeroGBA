#include <cctype>
#include <string.h>
#include "GBA.hpp"

int main(int argc, char *argv[]) {
    
    if(argc < 3) {
        std::cerr << "Executable, code translation mode [-i,-j], and ROM must be given as arguments.\n";
        exit(1);
    }
    
    std::string fileExtension = std::filesystem::path(argv[2]).extension().string();
    std::transform(fileExtension.begin(),fileExtension.end(),fileExtension.begin(),[](char c){return std::tolower(c);});
    
    // Emulation loops
    // todo: implement argument parsing
    if(fileExtension == ".gba") {
        // load GBA game
        GBA gba;
        if(!gba.loadRom(argv[2])) {
            std::cout << "Error loading file\n";
            exit(1);
        }

        if(strcmp(argv[1],"-i") == 0) {
            gba.arm7tdmi->fillARM();
            gba.arm7tdmi->fillTHUMB();

            while(true) {
                gba.interpretARM();
            }

        } else if(strcmp(argv[1],"-j") == 0) {
            // no implementation yet
            exit(1);
        } else {
            std::cout << "Invalid arguments\n";
            exit(1);
        }

    } else if(fileExtension == ".nds") {
        // no implementation yet
        exit(1);
    } else {
        std::cerr << "Incompatible filetype\n";
        exit(1);
    }
    return 0;
}