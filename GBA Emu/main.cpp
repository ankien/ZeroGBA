#include <iostream>
#include <filesystem>
#include "GameBoy.h"

int main(int argc, char *argv[]) {
    if(argc < 3) {
        std::cerr << "Path to executable, code translation mode, and ROM must be given as arguments.\n";
        exit(1);
    }
    
    // Emulation loops
    std::string filename = std::filesystem::path(argv[3]).extension().string();
    if(filename == ".gb") {
        GameBoy gb;
        if(!gb.load(argv[1])) {
            std::cerr << "Error loading ROM.\n";
            exit(1);
        }
    } else if(filename == ".gba") {
    
    } else if(filename == ".nds") {
    
    } else {
        std::cerr << "Incompatible filetype.\n";
        exit(1);
    }
    return 0;
}