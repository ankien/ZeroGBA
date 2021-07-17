#include "Keypad.hpp"

void Keypad::toggleFullscreen(SDL_Window* window) {
    bool isFullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
    
    if(isFullscreen) {
        SDL_SetWindowFullscreen(window,false);
        SDL_SetWindowSize(window,initialWidth,initialHeight);
        glViewport(0, 0, initialWidth, initialHeight);
        SDL_ShowCursor(true);
    } else {
        
        SDL_GetCurrentDisplayMode(0,&displayMode);
        
        uint32_t oldWidth = displayMode.w;
        double possibleWidthScale = (double)displayMode.w / initialWidth;
        double possibleHeightScale = (double)displayMode.h / initialHeight;
        double scale;
        
        if(possibleHeightScale > possibleWidthScale)
            scale = possibleWidthScale;
        else
            scale = possibleHeightScale;
        
        uint32_t finalWidth = scale * initialWidth;
        uint32_t finalHeight = scale * initialHeight;
        
        SDL_SetWindowSize(window,finalWidth,finalHeight);
        SDL_SetWindowFullscreen(window, isFullscreen ? 0 : SDL_WINDOW_FULLSCREEN);
        SDL_GetCurrentDisplayMode(0,&displayMode);
        
        int32_t xOffset = (oldWidth-finalWidth)/2;
        if(xOffset > 0)
            xOffset -= 1;
        
        glViewport(xOffset, 0, finalWidth, finalHeight);
        SDL_ShowCursor(false);
    }
}

void Keypad::pollInputs() {
    while(SDL_PollEvent(&event)) {
        if(event.type == SDL_WINDOWEVENT)
            if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                running = false;

        if(event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.scancode) {
                case SDL_SCANCODE_UP:
                    systemMemory->IORegisters[0x130] &= 0xBF;
                    break;
                case SDL_SCANCODE_DOWN:
                    systemMemory->IORegisters[0x130] &= 0x7F;
                    break;
                case SDL_SCANCODE_LEFT:
                    systemMemory->IORegisters[0x130] &= 0xDF;
                    break;
                case SDL_SCANCODE_RIGHT:
                    systemMemory->IORegisters[0x130] &= 0xEF;
                    break;
                case SDL_SCANCODE_SPACE:
                    systemMemory->IORegisters[0x130] &= 0xFE;
                    break;
                case SDL_SCANCODE_S:
                    systemMemory->IORegisters[0x130] &= 0xFD;
                    break;
                case SDL_SCANCODE_A:
                    systemMemory->IORegisters[0x131] &= 0xFD;
                    break;
                case SDL_SCANCODE_D:
                    systemMemory->IORegisters[0x131] &= 0xFE;
                    break;
                case SDL_SCANCODE_RETURN:
                    enterDown = true;
                    systemMemory->IORegisters[0x130] &= 0xF7;
                    break;
                case SDL_SCANCODE_RSHIFT:
                    systemMemory->IORegisters[0x130] &= 0xFB;
                    break;
                #ifdef TRACE
                case SDL_SCANCODE_T:
                    systemMemory->tracing ^= true;
                    break;
                #endif
                case SDL_SCANCODE_LALT:
                    altDown = true;
                    break;
                case SDL_SCANCODE_RALT:
                    altDown = true;
                    break;
                case SDL_SCANCODE_ESCAPE:
                    running = false;
                    break;
                case SDL_SCANCODE_TAB:
                    noAudioSync = true;
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
                case SDL_SCANCODE_TAB:
                    noAudioSync = false;
            }
        }

        if(event.type == SDL_CONTROLLERBUTTONDOWN) {
            switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    systemMemory->IORegisters[0x130] &= 0xBF;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    systemMemory->IORegisters[0x130] &= 0x7F;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    systemMemory->IORegisters[0x130] &= 0xDF;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    systemMemory->IORegisters[0x130] &= 0xEF;
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    systemMemory->IORegisters[0x130] &= 0xFE;
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    systemMemory->IORegisters[0x130] &= 0xFD;
                    break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    systemMemory->IORegisters[0x131] &= 0xFD;
                    break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    systemMemory->IORegisters[0x131] &= 0xFE;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    systemMemory->IORegisters[0x130] &= 0xF7;
                    break;
                case SDL_CONTROLLER_BUTTON_BACK:
                    systemMemory->IORegisters[0x130] &= 0xFB;
                    break;
                case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                    noAudioSync ^= true;
            }
        }

        if(event.type == SDL_CONTROLLERBUTTONUP) {
            switch(event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    systemMemory->IORegisters[0x130] |= 0x40;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    systemMemory->IORegisters[0x130] |= 0x80;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    systemMemory->IORegisters[0x130] |= 0x20;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    systemMemory->IORegisters[0x130] |= 0x10;
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    systemMemory->IORegisters[0x130] |= 0x1;
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    systemMemory->IORegisters[0x130] |= 0x2;
                    break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    systemMemory->IORegisters[0x131] |= 0x2;
                    break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    systemMemory->IORegisters[0x131] |= 0x1;
                    break;
                case SDL_CONTROLLER_BUTTON_START:
                    systemMemory->IORegisters[0x130] |= 0x8;
                    break;
                case SDL_CONTROLLER_BUTTON_BACK:
                    systemMemory->IORegisters[0x130] |= 0x4;
            }
        }

        if(event.type == SDL_CONTROLLERAXISMOTION) {
            switch(event.caxis.axis) {
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    event.caxis.value != 0 ? noAudioSync = true : noAudioSync = false;
            }
        }
    }
}
