#include "LCD.hpp"

LCD::LCD() {
    // SDL + OpenGL setup
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER);

    window = SDL_CreateWindow(
        "Desu", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        WIDTH*SCALE, 
        HEIGHT*SCALE, 
        SDL_WINDOW_OPENGL
    );

    SDL_GL_CreateContext(window);
    glewInit();
    
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,1);

    // initialize pixel buffer
    pixelBuffer = new uint16_t[38400];

    compileShaders();
}

void LCD::fetchScanline() {

    if(VCOUNT < 160) { // if VCOUNT < 160, load update a single scanline, 160-227 is non-visible scanline range
        switch(DISPCNT_MODE) {
            case 0: // tile/map + text mode
                
                break;
            case 1: // mode 0/2 mixed
                break;
            case 2: // tile/map + scale/rotation mode
                break;
            case 3: // bitmap mode for still images
            {
                uint16_t lineStart = VCOUNT * 240;
                for(uint8_t i = 0; i < 240; i++) {
                    uint32_t vramIndex = (lineStart + i) * 2;
                    pixelBuffer[lineStart + i] = systemMemory->vram[vramIndex] | (systemMemory->vram[vramIndex + 1] << 8);
                }
                break;
            }
            case 4: // bitmap mode
            {
                uint16_t frameBufferStart = DISPCNT_DISPLAY_FRAME_SELECT ? 0x5000 : 0;
                uint16_t lineStart = VCOUNT * 240;
                for(uint8_t i = 0; i < 240; i++) {
                    uint8_t paletteEntry = systemMemory->vram[frameBufferStart + lineStart + i] * 2;
                    pixelBuffer[lineStart + i] = systemMemory->pram[paletteEntry] | (systemMemory->pram[paletteEntry + 1] << 8);
                }
                break;
            }
            case 5: // bitmap mode
                break;
        }
    }
    
    if(VCOUNT == 228)
        systemMemory->IORegisters[6] = 0; // reset vcount
    else
        systemMemory->IORegisters[6]++; // increment vcount
    systemMemory->IORegisters[4] |= ((VCOUNT == DISPSTAT_VCOUNT_SETTING) << 2); // set v-counter flag
}

void LCD::draw() {
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB5_A1,WIDTH,HEIGHT,0,GL_BGRA,GL_UNSIGNED_SHORT_1_5_5_5_REV,pixelBuffer);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);

    SDL_GL_SwapWindow(window);

    fps++;

    // If a second has passed
    currMillseconds = SDL_GetTicks();
    if(currMillseconds / 1000 != millisecondsElapsedAtLastSecond / 1000) {
        std::string title = std::to_string(fps)+" fps - Desu";
        SDL_SetWindowTitle(window,title.c_str());
        fps = 0;
        millisecondsElapsedAtLastSecond = currMillseconds;
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

    uint32_t textureName;
    // bind textures
    glGenTextures(1,&textureName);
    glBindTexture(GL_TEXTURE_2D,textureName);

    // attach shaders to program
    glAttachShader(program,vertexShader);
    glAttachShader(program,fragmentShader);
    glLinkProgram(program);

    // set texture parameters of program
    const int32_t bgrx[4] = {GL_BLUE,GL_GREEN,GL_RED,GL_ONE};
    // r is b when fetched, g is g, b is r, alpha is always on
    glTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_RGBA,bgrx);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glUseProgram(program);
}
