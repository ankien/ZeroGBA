#include "GameBoy.h"

GameBoy::GameBoy() { // initialize everything
    memset(eightBitReg, 0, 8);
    sp = pc = 0;
}

void GameBoy::interpretCycle() {

}

bool GameBoy::load(std::string fileName) { // need to handle different memory banks
    std::fstream fileStream(fileName, std::ios::binary | std::ios::in);
    if(!fileStream.is_open()) return false;

    char byte;
    for(uint16_t i = 0; fileStream.get(byte); i++) {
        if(i > 0x7FFF) return false;
        memory[i] = byte;
    }
    return true;
}