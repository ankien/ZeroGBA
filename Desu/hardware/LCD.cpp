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

#define BITMAP_MODE_SETUP uint16_t lineStart = VCOUNT * 240; \
                          uint16_t* scanLine = pixelBuffer + lineStart;
void LCD::fetchScanline() {

    /*
        - Quick 101 on the PPU -
        PRAM contains color data for BGs and Sprites/OBJs
        Tiled BGs can range from, 128x128px to 1024x1024px, sprites up to 64x64px, each tiled BG + sprite is comprised of 8x8px bitmaps (tiles)
        Palettes and tiles can be 4bpp(16colors/16banked palettes) or 8bpp(256colors/1palette), all affine BGs are 8bpp
        Sprites have one tile-map entries/indexes (multi-tile), but BGs comprised of many (single-tile)
        VRAM is basically a big bitmap 8 pixels wide, and store the tile data OR maps for tile BG modes (data for sprites in OAM as well)
        Charblocks - 6 16kb chunks in VRAM, 4 for BGs and 2 for sprites, tile-indexing is different for sprites and BGs
        
        Idea behind fetch order of a scanline:
        ?
    */

    // todo: implement rotation + scaling (affine), and objs for bitmap modes, check DISPCNT, no BGCNT in mode 3-5
    if(VCOUNT < 160) { // if VCOUNT < 160, load update a single scanline, 160-227 is non-visible scanline range

        switch(DISPCNT_MODE) {
            case 0: // BG[0-3] text/tile BG mode, no affine
                
                break;
            case 1: // BG[0-2] text/tile BGs like mode 0, but only BG2 has affine
                break;
            case 2: // BG[2-3] tiled BGs w/ affine
                break;
            case 3: // BG[2] bitmap BG mode w/o page flipping, affine
            {
                BITMAP_MODE_SETUP
                uint16_t* halfwordChunkVram = reinterpret_cast<uint16_t*>(&systemMemory->vram);
                for(uint8_t i = 0; i < 240; i++)
                    scanLine[i] = halfwordChunkVram[lineStart + i];
                break;
            }
            case 4: // BG[2] paletted bitmap BG mode, affine
            {
                BITMAP_MODE_SETUP
                uint16_t frameBufferStart = DISPCNT_DISPLAY_FRAME_SELECT ? 0xA000 : 0;
                uint16_t* halfwordChunkPram = reinterpret_cast<uint16_t*>(&systemMemory->pram);
                for(uint8_t i = 0; i < 240; i++)
                    // in paletted modes, pixels in an image are represented as 8-bit or 4-bit indexes into pram -
                    // In the case of the bitmapped backgrounds in modes 3 and 4, pixels are represented as the 16-bit color values themselves.
                    scanLine[i] = halfwordChunkPram[systemMemory->vram[frameBufferStart + lineStart + i]];
                break;
            }
            case 5: // BG[2] bitmap BG mode with page flipping, affine
            {
                BITMAP_MODE_SETUP
                uint8_t row = VCOUNT;
                uint16_t frameBufferStart = DISPCNT_DISPLAY_FRAME_SELECT ? 0xA000 : 0;
                uint16_t backgroundColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);// background color is the first index in pram
                if(row < 128) {
                    uint16_t* halfwordChunkVram = reinterpret_cast<uint16_t*>(&systemMemory->vram);
                    for(uint8_t i = 0; i < 160; i++)
                        scanLine[i] = halfwordChunkVram[(row * 160 + i) + frameBufferStart];
                    for(uint8_t i = 160; i < 240; i++)
                        scanLine[i] = backgroundColor;
                } else
                    for(uint8_t i = 0; i < 240; i++)
                        scanLine[i] = backgroundColor;
            }
                break;
        }
    }
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
