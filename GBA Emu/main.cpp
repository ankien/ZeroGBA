#include <cctype>
#include "GBA.hpp"

int main(int argc, char *argv[]) {
    if(argc < 3) {
        std::cerr << "Executable, code translation mode [-i,-j], and ROM must be given as arguments.\n";
        exit(1);
    }
    
    // Emulation loops
    std::string fileExtension = std::filesystem::path(argv[2]).extension().string();
    std::transform(fileExtension.begin(),fileExtension.end(),fileExtension.begin(),[](char c){return std::tolower(c);});
    if(fileExtension == ".gba") {
        GBA gba(argv[2]);
        if(gba.romMemory == NULL) {
            std::cout << "Error loading file\n";
            exit(1);
        }

        if(argv[1] == "-i") {
            while(true) {
                //gba.arm7.interpretARMCycle(gba.romMemory);
            }
        } else if(argv[1] == "-j") {
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