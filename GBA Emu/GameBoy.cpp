#include "GameBoy.h"

GameBoy::GameBoy() { // initialize everything
    memset(eightBitReg, 0, 8);
    sp = pc = 0;
}

void GameBoy::interpretCycle() {

}

bool GameBoy::load(std::string fileName) {
    std::fstream fileStream(fileName, std::ios::binary | std::ios::in);
    if(!fileStream.is_open()) return false;

    char byte;
    for(uint16_t i = ; fileStream.get(byte); i++) {

    }
    return true;
}