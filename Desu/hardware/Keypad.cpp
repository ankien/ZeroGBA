#include "Keypad.hpp"

void Keypad::toggleFullscreen(SDL_Window* window) {
    uint32_t fullscreenFlag = SDL_WINDOW_FULLSCREEN;
    bool isFullscreen = SDL_GetWindowFlags(window) & fullscreenFlag;
    SDL_SetWindowFullscreen(window, isFullscreen ? 0 : fullscreenFlag);
    isFullscreen ? (displayMode.w = width, displayMode.h = height) : SDL_GetCurrentDisplayMode(0,&displayMode);
    glViewport(0,0,displayMode.w,displayMode.h);
    SDL_ShowCursor(isFullscreen);
}

void Keypad::pollInputs() {
    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_WINDOWEVENT)
            if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                running = false;

        if(event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_UP:
                    systemMemory->IORegisters[0x130] ^= 0x40;
                    break;
                case SDL_SCANCODE_DOWN:
                    systemMemory->IORegisters[0x130] ^= 0x80;
                    break;
                case SDL_SCANCODE_LEFT:
                    systemMemory->IORegisters[0x130] ^= 0x20;
                    break;
                case SDL_SCANCODE_RIGHT:
                    systemMemory->IORegisters[0x130] ^= 0x10;
                    break;
                case SDL_SCANCODE_SPACE:
                    systemMemory->IORegisters[0x130] ^= 0x1;
                    break;
                case SDL_SCANCODE_S:
                    systemMemory->IORegisters[0x130] ^= 0x2;
                    break;
                case SDL_SCANCODE_A:
                    systemMemory->IORegisters[0x131] ^= 0x2;
                    break;
                case SDL_SCANCODE_D:
                    systemMemory->IORegisters[0x131] ^= 0x1;
                    break;
                case SDL_SCANCODE_RETURN:
                    enterDown = true;
                    systemMemory->IORegisters[0x130] ^= 0x8;
                    break;
                case SDL_SCANCODE_RSHIFT:
                    systemMemory->IORegisters[0x130] ^= 0x4;
                    break;
                case SDL_SCANCODE_LALT:
                    altDown = true;
                    break;
                case SDL_SCANCODE_RALT:
                    altDown = true;
                    break;
            }

            if(altDown && enterDown)
                toggleFullscreen(window);
        }

        if(event.type == SDL_KEYUP) {
            switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_UP:
                    systemMemory->IORegisters[0x130] |= 0x40;
                    break;
                case SDL_SCANCODE_DOWN:
                    systemMemory->IORegisters[0x130] |= 0x80;
                    break;
                case SDL_SCANCODE_LEFT:
                    systemMemory->IORegisters[0x130] |= 0x20;
                    break;
                case SDL_SCANCODE_RIGHT:
                    systemMemory->IORegisters[0x130] |= 0x10;
                    break;
                case SDL_SCANCODE_SPACE:
                    systemMemory->IORegisters[0x130] |= 0x1;
                    break;
                case SDL_SCANCODE_S:
                    systemMemory->IORegisters[0x130] |= 0x2;
                    break;
                case SDL_SCANCODE_A:
                    systemMemory->IORegisters[0x131] |= 0x2;
                    break;
                case SDL_SCANCODE_D:
                    systemMemory->IORegisters[0x131] |= 0x1;
                    break;
                case SDL_SCANCODE_RETURN:
                    enterDown = false;
                    systemMemory->IORegisters[0x130] |= 0x8;
                    break;
                case SDL_SCANCODE_RSHIFT:
                    systemMemory->IORegisters[0x130] |= 0x4;
                    break;
                case SDL_SCANCODE_LALT:
                    altDown = false;
                    break;
                case SDL_SCANCODE_RALT:
                    altDown = false;
                    break;
            }
        }
    }
}
