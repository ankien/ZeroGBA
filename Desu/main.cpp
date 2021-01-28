#include <iostream>
#include <SDL.h>
#include "GBA.hpp"

// because SDL defines main for some reason
// enable for console subsystem on VS
//#undef main 

// For windows subsystem compatibility
// https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-160
int _stdcall WinMain (struct HINSTANCE__*,struct HINSTANCE__*,char*,int) { return main (__argc, __argv); }

int main(int argc, char* argv[]) {
    GBA gba;
    if(gba.parseArguments(argc,argv))
        gba.run(argv[argc - 1]);
    else
        std::cout << "Error with arguments - format: [executable] [-options] [rom]\n";
    return 0;
}