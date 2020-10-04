#include <iostream>
#include <filesystem>
#include <cctype>
#include "GBA.hpp"

int main(int argc, char *argv[]) {
    if(argc < 3) {
        std::cerr << "Path to executable, code translation mode, and ROM must be given as arguments.\n";
        exit(1);
    }
    
    // Emulation loops
    std::string fileExtension = std::filesystem::path(argv[2]).extension().string();
    std::transform(fileExtension.begin(),fileExtension.end(),fileExtension.begin(),[](char c){return std::tolower(c);});
    if(fileExtension == ".gba") {
        // load game
        GBA gba(argv[2]);

        if(argv[1] == "-i") {

        } else if(argv[1] == "-j") {

        } else {
            std::cout << "Invalid arguments\n";
        }

    } else if(fileExtension == ".nds") {
    
    } else {
        std::cerr << "Incompatible filetype\n";
        exit(1);
    }
    return 0;
}