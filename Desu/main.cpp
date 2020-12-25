#include <cctype>
#include <cstring>
#include <iostream>
#include "GBA.hpp"

// because SDL defines main for some reason
// enable for console subsystem on VS
#undef main 

/* // Black magic code for windows subsystem compatibility
#ifdef __GNUC__
#define _stdcall  __attribute__((stdcall))
#endif
int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
         struct HINSTANCE__ *hPrevInstance,
         char               *lpszCmdLine,
         int                 nCmdShow)
{
  return main (__argc, __argv);
}
*/

struct {
    uint8_t jit : 1; // not implemented
} runtimeOptions;

bool parseArguments(uint64_t argc, char* argv[]) {
    if(argc < 2)
        return 0;
    
    if(argc > 2)
        for(uint64_t i = 1; i < (argc - 1); i++) {
            if(strcmp(argv[i], "-j") == 0)
                runtimeOptions.jit = 1;
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

        if(runtimeOptions.jit) { // GBA JIT
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

                    if((gba.cyclesSinceHBlank >= 960) && (gba.cyclesPassed <= 197120)) { // scan and draw line from framebuffer
                        gba.lcd->fetchScanline();
                        gba.lcd->draw();
                        gba.cyclesSinceHBlank -= 1232;
                    }
                }

                if(gba.cyclesPassed > 280896)
                    gba.cyclesPassed -= 280896;
                else
                    gba.cyclesPassed = 0;

                gba.cyclesSinceHBlank = gba.cyclesPassed; // keep other cycle counters in sync with system

                SDL_Delay(16 - (SDL_GetTicks() / 1000 - gba.lcd->secondsElapsed)); // roughly 1000ms / 60fps - delay since start of last frame draw
            }
        }

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