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

    // initialize framebuffer
    frameBuffer = new uint32_t[38400];
    memset(frameBuffer,0,38400);

    vertexArrayObject = new uint32_t[1];
    vertexArrayObject[0] = 0;
    compileShaders();
}

void LCD::fetchScanline() {
    if((*systemMemory).vcount < 160) {
        switch((*systemMemory).getBits((*systemMemory).dispcnt,0,2)) {
            case 0:
                break;
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
            case 5:
                break;
        }
    }
}

void LCD::draw() {
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5,WIDTH,HEIGHT,0,GL_RGBA,GL_UNSIGNED_SHORT_1_5_5_5_REV,frameBuffer);
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

std::string LCD::loadShader(const char* fileName) {
    std::ifstream fileStream(fileName, std::ios::binary | std::ios::in);

    std::string output;
    std::string line;

    while(fileStream.good()) {
        std::getline(fileStream, line);
        output.append(line + "\n");
    }
    return output;
}
uint32_t LCD::createShader(std::string source, uint32_t shaderType) {
    uint32_t shader = glCreateShader(shaderType);

    const char* shaderSourceStrings[1] = {source.c_str()};

    glShaderSource(shader,1,shaderSourceStrings,NULL);
    glCompileShader(shader);

    return shader;
}
void LCD::compileShaders() {
    program = glCreateProgram();

    uint32_t vertexShader = createShader(loadShader("./shaders/GBA.vert"),GL_VERTEX_SHADER);
    uint32_t fragmentShader = createShader(loadShader("./shaders/GBA.frag"),GL_FRAGMENT_SHADER);

    glGenTextures(1,frameBuffer);

    // bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,*frameBuffer);

    // attach shaders to program
    glAttachShader(program,vertexShader);
    glAttachShader(program,fragmentShader);
    glLinkProgram(program);

    // set texture parameters of program
    const int32_t abgr[4] = {1,GL_BLUE,GL_GREEN,GL_RED};
    glTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_RGBA,abgr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUseProgram(program);

    // generate mesh/vertices
    glGenVertexArrays(1,vertexArrayObject);
    glBindVertexArray(*vertexArrayObject);
}
