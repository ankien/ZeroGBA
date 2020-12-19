#include "LCD.hpp"

LCD::LCD(GBAMemory* systemMemory) {
    this->systemMemory = systemMemory;

    // SDL + OpenGL setup
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,1);

    SDL_Window* window = SDL_CreateWindow(
        "Placeholder Title", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        WIDTH*SCALE, 
        HEIGHT*SCALE, 
        SDL_WINDOW_OPENGL
    );
    
    this->window = window;

    SDL_GL_CreateContext(window);
    glewInit();

    // load shaders
    
}

void LCD::draw(uint8_t* pixelBuffer) {
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5,WIDTH,HEIGHT,0,GL_RGBA,GL_UNSIGNED_SHORT_1_5_5_5_REV,pixelBuffer);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    SDL_GL_SwapWindow(window);

    fps+=1;

    // If a second has passed
    uint8_t currSecsPassed = SDL_GetTicks() / 1000;
    if(currSecsPassed != secondsElapsed) {
        std::string title = std::to_string(fps)+" FPS desu!";
        SDL_SetWindowTitle(window,title.c_str());
        fps = 0;
        secondsElapsed = currSecsPassed;
    }
}
