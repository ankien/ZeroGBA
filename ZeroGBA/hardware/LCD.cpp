#include "LCD.hpp"

LCD::LCD() {
    // SDL + OpenGL setup
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);

    // Controller setup
    for(uint8_t i = 0; i < SDL_NumJoysticks(); i++)
        if(SDL_IsGameController(i))
            SDL_GameControllerOpen(i);

    // Window setup
    window = SDL_CreateWindow(
        "ZeroGBA", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED, 
        WIDTH*SCALE, 
        HEIGHT*SCALE, 
        SDL_WINDOW_OPENGL
    );

    SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0);
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

// Credits to NanoBoyAdvance and Crab for figuring this shit out
void LCD::renderTextBG(uint8_t bg,uint8_t vcount) {
    if((systemMemory->IORegisters[1] & (1 << bg)) == 0)
        return;
    uint8_t* currLayer = &bgLayer[bg][0];

    uint16_t bgcnt = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x8 + (0x2*bg)]);
    uint8_t* charBlockBase = &systemMemory->vram[(((bgcnt & 0xC) >> 2) * 0x4000)]; // tile data
    uint16_t* screenBlockBase = reinterpret_cast<uint16_t*>(&systemMemory->vram[(((bgcnt & 0x1F00) >> 8) * 0x800)]); // tile map
    bool eightBppEnabled = bgcnt & 0x80;
    // todo: implement mosaic

    uint16_t width, height;
    uint16_t bgcntSize = bgcnt & 0xC000;
    // we decrease the w/h by 1 pixel since we only need it as a mask for finding the tile x and y
    switch(bgcntSize) { // divide each value by 8 for tile height or width
        case 0x0000:
            width = 255; height = 255;
            break;
        case 0x4000:
            width = 511; height = 255;
            break;
        case 0x8000:
            width = 255; height = 511;
            break;
        case 0xC000:
            width = 511; height = 511;
    }

    // position of the SCREEN on the MAP, positive values correspond to moving the screen right and down respectively
    // the screen will wrap at the right at bottom edges of the BG
    uint16_t bghofs = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x10 + (0x4*bg)]) & 0x1FF;
    uint16_t bgvofs = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x12 + (0x4*bg)]) & 0x1FF;

    uint16_t bgTileRow = vcount + bgvofs & height;
    uint16_t ty = bgTileRow / 8;

    for(uint8_t i = 0; i < 240; i++) {
        uint16_t bgTileColumn = i + bghofs & width;
        uint16_t tx = bgTileColumn / 8;
        
        uint16_t screenIndex = screenEntryIndex(tx,ty,bgcntSize);
        uint16_t screenEntry = screenBlockBase[screenIndex];

        uint16_t tid = screenEntry & 0x3FF;
        bool flipX = screenEntry & 0x400;
        bool flipY = screenEntry & 0x800;
        // 1D tile map coordinates in a char block
        uint16_t x = (bgTileColumn & 7) ^ (7*flipX);
        uint16_t y = (bgTileRow & 7) ^ (7*flipY);

        if(eightBppEnabled) // 8bpp
            currLayer[i] = charBlockBase[tid * 0x40 + y*8 + x];
        else { // 4bpp
            uint8_t pBank = (screenEntry & 0xF000) >> 12;
            uint8_t pIndex = charBlockBase[tid * 0x20 + y*4 + x/2];
            pIndex = (pIndex >> ((x & 1) * 4) & 0xF); // read 4-bit pIndexes from 8-bit array
            if(pIndex != 0)
                currLayer[i] = pBank * 16 + pIndex;
        }
    }
}
void LCD::renderAffineBG(uint8_t bg) {
    if((systemMemory->IORegisters[1] & (1 << bg)) == 0)
        return;

    // I don't check whether or not a non-2 or 3 bg uses this method, but I don't need to :-)

    int32_t internalX = systemMemory->internalRef[bg - 2].x;
    int32_t internalY = systemMemory->internalRef[bg - 2].y-1;

    int16_t pa = *reinterpret_cast<int16_t*>(&systemMemory->IORegisters[0x10 * bg]);
    int16_t pc = *reinterpret_cast<int16_t*>(&systemMemory->IORegisters[(0x10 * bg) + 0x4]);;
    
    uint8_t* currLayer = bgLayer[bg];
    uint16_t bgcnt = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x8 + (0x2*bg)]);
    uint8_t* charBlockBase = &systemMemory->vram[(((bgcnt & 0xC) >> 2) * 0x4000)];
    uint8_t* screenBlockBase = reinterpret_cast<uint8_t*>(&systemMemory->vram[(((bgcnt & 0x1F00) >> 8) * 0x800)]);
    bool displayOverflow = bgcnt & 0x2000;

    uint8_t bgcntSize = (bgcnt & 0xC000) >> 14;
    uint8_t tileNum = 16 << bgcntSize; // aff BGs are always squares of factor 16x16 tiles
    uint16_t bgSize = tileNum * 8; // in pixels

    for(uint8_t i = 0; i < 240; i++) {
        uint32_t pixX = internalX >> 8;
        uint32_t pixY = internalY >> 8;

        internalX += pa;
        internalY += pc;

        if(displayOverflow) {
            pixX %= bgSize;
            pixY %= bgSize;
        } else if(pixX >= bgSize || pixY >= bgSize || pixX < 0 || pixY < 0)
            continue;

        // Affine BG screen entries are only 8 bits and only contain the TIDs
        uint8_t tid = screenBlockBase[(pixY/8)*tileNum + pixX/8];
        currLayer[i] = charBlockBase[tid * 0x40 + (pixY % 8)*8 + pixX%8];
    }
}
void LCD::renderSprites(uint32_t baseAddress, int16_t vcount) {
    uint16_t dispCnt = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0]);
    if((dispCnt & 0x1000) == 0)
        return;
    uint8_t* charBlockBase = &systemMemory->vram[baseAddress]; // todo: implement sprites for bitmapped modes

    std::fill_n(spriteLayer,240,Sprite{});

    for(uint16_t spriteByteOffset = 0; spriteByteOffset < 1024; spriteByteOffset+=8) {
        uint16_t attribute0 = *reinterpret_cast<uint16_t*>(&systemMemory->oam[spriteByteOffset]);
        uint16_t attribute1 = *reinterpret_cast<uint16_t*>(&systemMemory->oam[spriteByteOffset + 2]);

        // skip OBJ conditions from OAM attributes
        bool affine = attribute0 & 0x100;
        bool attr0Bit9 = attribute0 & 0x200;
        if(!affine && attr0Bit9)
            continue;

        uint8_t mode = (attribute0 & 0xC00) >> 10;
        if(mode == 3)
            continue;

        // the rest of the fukin variables
        int16_t y = attribute0 & 0xFF;  // marks the top of the sprite, grows downwards
        int16_t x = attribute1 & 0x1FF; // marks the left of the sprite, grows rightwards
        uint8_t shape = (attribute0 & 0xC000) >> 14;
        uint8_t size = (attribute1 & 0xC000) >> 14;

        // wrap the x and y around
        if(x >= 240) x -= 512;
        if(y >= 160) y -= 256;

        uint8_t spriteWidth = spriteOBJSize[shape][size][0];
        uint8_t spriteHeight = spriteOBJSize[shape][size][1];

        uint8_t halfWidth = spriteWidth / 2;
        uint8_t halfHeight = spriteHeight / 2;

        // accounting for width and height to represent the center
        x += halfWidth;
        y += halfHeight;

        int16_t pa,pb,pc,pd;
        if(affine) {
            uint16_t affineParam = ((attribute1 & 0x3E00) >> 9) * 32;

            pa = *reinterpret_cast<int16_t*>(&systemMemory->oam[affineParam + 0x6]);
            pb = *reinterpret_cast<int16_t*>(&systemMemory->oam[affineParam + 0xE]);
            pc = *reinterpret_cast<int16_t*>(&systemMemory->oam[affineParam + 0x16]);
            pd = *reinterpret_cast<int16_t*>(&systemMemory->oam[affineParam + 0x1E]);

            // if sprite should be treated as double sized
            if(attr0Bit9) {
                x += halfWidth;
                y += halfHeight;
                halfWidth *= 2;
                halfHeight *= 2;
            }
        } else
            pa = 0x100, pb = 0, pc = 0, pd = 0x100;

        // if sprite isn't within line, don't draw it
        if((y - halfHeight) >  vcount || (y + halfHeight) < vcount)
            continue;

        // y relative to the line being drawn
        int16_t relativeY = vcount - y;

        // bool mosaic = attribute0 & 0x1000; todo
        bool eightBitColors = attribute0 & 0x2000;
        bool horizontalFlip = affine ? false : attribute1 & 0x1000;
        bool verticalFlip = affine ? false : attribute1 & 0x2000;
        uint16_t attribute2 = *reinterpret_cast<uint16_t*>(&systemMemory->oam[0] + spriteByteOffset + 4);
        bool oneDimensionMapping = systemMemory->IORegisters[0] & 0x40;
        
        // transform x and y to texture coordinates 
        for(int8_t objX = -halfWidth; objX <= halfWidth; objX++) {
            int16_t screenX = objX + x; // left of sprite relative to screen x-coords
            
            if((screenX < 0) || (screenX >= 240))
                continue;

            // transform to tex coords and bring origin to top left
            int16_t texX = ((pa*objX + pb*relativeY) >> 8) + (spriteWidth / 2);
            int16_t texY = ((pc*objX + pd*relativeY) >> 8) + (spriteHeight / 2);

            // if the coord is outside the boundary of the original obj, skip
            if(texY >= spriteHeight || texX >= spriteWidth || texY < 0 || texX < 0)
                continue;

            if(horizontalFlip) texX =  spriteWidth - texX - 1;
            if(verticalFlip) texY = spriteHeight - texY - 1;

            uint16_t tid = attribute2 & 0x3FF;
            uint8_t tileX = texX % 8; // X within tile
            uint8_t tileY = texY % 8; // Y within tile
            
            uint16_t tileRowOffset = texY / 8; // used to get the correct tid according to 1D or 2D mapping
            if(oneDimensionMapping) // 1D VRAM mapping
                tileRowOffset *= spriteWidth / 8;
            else // 2D VRAM mapping
                if(eightBitColors)
                    tileRowOffset *= 0x10;
                else
                    tileRowOffset *= 0x20;
            tileRowOffset += texX / 8;

            uint8_t paletteIndex = 0;
            // Unlike BGs, TID for sprites are located in OAM
            if(eightBitColors) {
                tid /= 2;
                tid += tileRowOffset;
                paletteIndex = charBlockBase[tid * 0x40 + tileY * 8 + tileX];
            } else {
                tid += tileRowOffset;
                paletteIndex = charBlockBase[tid * 0x20 + tileY * 4 + tileX/2];
                paletteIndex = (paletteIndex >> ((tileX & 1) * 4) & 0xF); // read 4-bit pIndexes from 8-bit array
                if(paletteIndex != 0)
                    paletteIndex += ((attribute2 & 0xF000) >> 12) * 16;
            }

            auto& spritePixel = spriteLayer[screenX];
            bool notTransparent = paletteIndex != 0;
            uint8_t priority = (attribute2 & 0xC00) >> 10;
            uint8_t mode = (attribute0 & 0xC00) >> 10;
            if(mode == 0x2) {
                if(notTransparent) spritePixel.window = true;
            } else if(priority < spritePixel.priority || spritePixel.pindex == 0) {
                if(notTransparent) {
                    spritePixel.pindex = paletteIndex;
                    spritePixel.alpha = mode == 0x1;
                }
                spritePixel.priority = priority;
            }
        }
    }
}

template<bool brightnessIncrease>
uint16_t LCD::brightnessFade(uint8_t evy, uint16_t color) {
    evy = evy > 16 ? 16 : evy;
    uint8_t r = color & 0x1F;
    uint8_t g = (color & 0x3E0) >> 5;
    uint8_t b = (color & 0x7C00) >> 10;
    
    if(brightnessIncrease) {
        r += ((31 - r) * evy) / 16;
        g += ((31 - g) * evy) / 16;
        b += ((31 - b) * evy) / 16;
    } else {
        r -= (r * evy) / 16;
        g -= (g * evy) / 16;
        b -= (b * evy) / 16;
    }

    return (b << 10 | g << 5 | r);
}
uint16_t LCD::blend(uint8_t eva, uint16_t colorA, uint8_t evb, uint16_t colorB) {
    eva = eva > 16 ? 16 : eva;
    evb = evb > 16 ? 16 : evb;
    uint8_t rA = colorA & 0x1F;
    uint8_t gA = (colorA & 0x3E0) >> 5;
    uint8_t bA = (colorA & 0x7C00) >> 10;
    uint8_t rB = colorB & 0x1F;
    uint8_t gB = (colorB & 0x3E0) >> 5;
    uint8_t bB = (colorB & 0x7C00) >> 10;

    rA = (rA*eva + rB*evb) / 16 > 31 ? 31 : (rA*eva + rB*evb) / 16;
    bA = (bA*eva + bB*evb) / 16 > 31 ? 31 : (bA*eva + bB*evb) / 16;
    gA = (gA*eva + gB*evb) / 16 > 31 ? 31 : (gA*eva + gB*evb) / 16;
    
    return (bA << 10 | gA << 5 | rA);
}
void LCD::composeScanline(uint16_t* scanline, uint8_t vcount) {

    // todo: optimize the faq out of this, presort the backgrounds before per-pixel compositing;
    // also break off compositing loop if all BGs are enabled, but only one is shown,
    // take alpha blending into consideration (if you blend the highest layer, order up another BG, only two layer blend at most)
    uint16_t winin = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x48]);
    uint8_t win0List = winin & 0x1F;
    uint8_t win1List = (winin & 0x1F00) >> 8;
    uint16_t winout = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x4A]);
    uint8_t outList = winout & 0x1F;
    uint8_t objList = (winout & 0x1F00) >> 8;
    uint16_t dispcnt = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters);

    uint8_t bldcntFirstTargets = *reinterpret_cast<uint8_t*>(&systemMemory->IORegisters[0x50]) & 0x3F;
    uint8_t bldcntSecondTargets = *reinterpret_cast<uint8_t*>(&systemMemory->IORegisters[0x51]) & 0x3F;
    uint8_t colorSpecialEffect = (*reinterpret_cast<uint8_t*>(&systemMemory->IORegisters[0x50]) & 0xC0) >> 6;
    uint8_t eva = *reinterpret_cast<uint8_t*>(&systemMemory->IORegisters[0x52]) & 0x1F;
    uint8_t evb = *reinterpret_cast<uint8_t*>(&systemMemory->IORegisters[0x53]) & 0x1F;
    uint8_t evy = *reinterpret_cast<uint8_t*>(&systemMemory->IORegisters[0x54]) & 0x1F;


    bool win0Display, win1Display, objDisplay, win1YVisible, win0YVisible, win0Effects, win1Effects, outEffects, objEffects, specialEffects;
    win0Display = dispcnt & 0x2000;
    win1Display = dispcnt & 0x4000;
    objDisplay = dispcnt & 0x8000;
    win0YVisible = WIN0V_Y1 <= vcount && vcount < WIN0V_Y2;
    win1YVisible = WIN1V_Y1 <= vcount && vcount < WIN1V_Y2;
    win0Effects = winin & 0x20;
    win1Effects = winin & 0x2000;
    outEffects = winout & 0x20;
    objEffects = winout & 0x2000;
    uint8_t enableList = 0, win0x1, win0x2, win0y1, win0y2, win1x1, win1x2, win1y1, win1y2;
    win0x1 = systemMemory->IORegisters[0x41];
    win0x2 = systemMemory->IORegisters[0x40];
    win0y1 = systemMemory->IORegisters[0x45];
    win0y2 = systemMemory->IORegisters[0x44];
    win1x1 = systemMemory->IORegisters[0x43];
    win1x2 = systemMemory->IORegisters[0x42];
    win1y1 = systemMemory->IORegisters[0x47];
    win1y2 = systemMemory->IORegisters[0x46];

    // composite a scanline from lowest priority windows (and within those, OBJs and BGs) to highest
    for(uint8_t x = 0; x < 240; x++) {
        
        uint16_t topColor;
        uint8_t topNonTransparentBg;

        // draw each window starting from the lowest priority one
        // Order: outside, obj, win1, win0
        for(uint8_t window = 0; window < 5; window++) {

            // set the bg enable list: OBJ (4), BG0-3 (0-3)
            if(windowList[window] == WIN0) {
                if(win0Display && win0x1 <= x && x < win0x2 && win0YVisible) {
                    topColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);
                    topNonTransparentBg = BD;
                    enableList = win0List;
                    specialEffects = win0Effects;
                } else
                    continue;
            } else if(windowList[window] == WIN1) {
                if(win1Display && win1x1 <= x && x < win1x2 && win1YVisible) {
                    topColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);
                    topNonTransparentBg = BD;
                    enableList = win1List;
                    specialEffects = win1Effects;
                } else
                    continue;
            } else if(windowList[window] == OBJ_WIN) {
                if(objDisplay && spriteLayer[x].window) {
                    topColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);
                    topNonTransparentBg = BD;
                    enableList = objList;
                    specialEffects = objEffects;
                } else
                    continue;
            } else if(windowList[window] == WINOUT) { // WINOUT
                if(win0Display || win1Display || objDisplay) {
                    topColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);
                    topNonTransparentBg = BD;
                    enableList = outList;
                    specialEffects = outEffects;
                } else
                    continue;
            } else {
                topColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);
                topNonTransparentBg = BD;
                // even with no windows, BGs are shown    
                enableList = (dispcnt & 0x1F00) >> 8;
                specialEffects = true;
            }

            // iterate through bgs and sprite of this window in lowest to highest priority
            for(int8_t priority = 3; priority >= 0; priority--) {
                
                // bgs
                for(int8_t bg = 3; bg >= 0; bg--) {
                    if(enableList & (1<<bg)) {
                        // if this bg lies on this priority
                        if((*reinterpret_cast<uint8_t*>(&systemMemory->IORegisters[0x8 + (0x2*bg)]) & 0x3) == priority) {
                            if(bgLayer[bg][x] == 0)
                                continue;
                            
                            uint16_t* halfWordPram = reinterpret_cast<uint16_t*>(&systemMemory->pram);
                            uint16_t newColor = halfWordPram[bgLayer[bg][x]];
                            
                            if(specialEffects) { // if blending enabled
                                switch(colorSpecialEffect) {
                                    case 0x0: // none
                                        goto opaqueBG;
                                    case 0x1: // alpha blending
                                        if(bldcntFirstTargets & (1<<bg) && bldcntSecondTargets & (1<<topNonTransparentBg)) {
                                            topColor = blend(eva,newColor,evb,topColor);
                                            topNonTransparentBg = bg;
                                        } else 
                                            goto opaqueBG;
                                        break;
                                    case 0x2: // brightness increase
                                        if(bldcntFirstTargets & (1<<bg)) {
                                            topColor = brightnessFade<true>(evy,newColor);
                                            topNonTransparentBg = bg;
                                        } else
                                            goto opaqueBG;
                                        break;
                                    case 0x3: // brightness decrease
                                        if(bldcntFirstTargets & (1<<bg)) {
                                            topColor = brightnessFade<false>(evy,newColor);
                                            topNonTransparentBg = bg;
                                        } else
                                            goto opaqueBG;
                                        break;
                                }
                            } else {
                                opaqueBG:
                                topColor = newColor;
                                topNonTransparentBg = bg;
                            }
                        }
                    }
                }

                if(enableList & 0b10000) {
                    if(spriteLayer[x].priority == priority && spriteLayer[x].pindex != 0) {
                        uint16_t* halfWordPram = reinterpret_cast<uint16_t*>(&systemMemory->pram[0x200]);
                        uint16_t newColor = halfWordPram[spriteLayer[x].pindex];
                        
                        if(specialEffects) {
                            if(spriteLayer[x].alpha) {
                                if(bldcntFirstTargets & (1 << OBJ) && bldcntSecondTargets & (1 << topNonTransparentBg)) {
                                    topColor = blend(eva, newColor, evb, topColor);
                                    topNonTransparentBg = OBJ;
                                } else {
                                    switch(colorSpecialEffect) {
                                        case 0x2: // brightness increase
                                            if(bldcntFirstTargets & (1 << OBJ)) {
                                                topColor = brightnessFade<true>(evy, newColor);
                                                topNonTransparentBg = OBJ;
                                            } else
                                                goto opaqueSprite;
                                            break;
                                        case 0x3: // brightness decrease
                                            if(bldcntFirstTargets & (1 << OBJ)) {
                                                topColor = brightnessFade<false>(evy, newColor);
                                                topNonTransparentBg = OBJ;
                                            } else
                                                goto opaqueSprite;
                                            break;
                                        default:
                                            goto opaqueSprite;
                                    }
                                }
                            } else {
                                switch(colorSpecialEffect) {
                                    case 0x0: // none
                                        goto opaqueSprite;
                                    case 0x1: // alpha blending
                                        if(bldcntFirstTargets & (1 << OBJ) && bldcntSecondTargets & (1 << topNonTransparentBg)) {
                                            topColor = blend(eva, newColor, evb, topColor);
                                            topNonTransparentBg = OBJ;
                                        } else
                                            goto opaqueSprite;
                                        break;
                                    case 0x2: // brightness increase
                                        if(bldcntFirstTargets & (1 << OBJ)) {
                                            topColor = brightnessFade<true>(evy, newColor);
                                            topNonTransparentBg = OBJ;
                                        } else
                                            goto opaqueSprite;
                                        break;
                                    case 0x3: // brightness decrease
                                        if(bldcntFirstTargets & (1 << OBJ)) {
                                            topColor = brightnessFade<false>(evy, newColor);
                                            topNonTransparentBg = OBJ;
                                        } else
                                            goto opaqueSprite;
                                        break;
                                }
                            }
                        } else {
                            opaqueSprite:
                            topColor = newColor;
                            topNonTransparentBg = OBJ;
                        }
                    }
                }
            }
        }

        scanline[x] = topColor;
    }
}

#define RENDER_SPRITES_AND_COMPOSE(baseAddress) if(DISPCNT_OBJ) \
                                                    renderSprites(baseAddress,vcount); \
                                                composeScanline(scanLine,vcount);
void LCD::renderScanline() {

    /*
        - Quick 101 on the PPU -   See "https://www.coranac.com/tonc/text/toc.htm" for more
        First step of rendering BG/Sprites: read control registers in BGCNT(BGs) or OAM attributes(Sprites)
        PRAM contains color data for BGs and Sprites (attributes for sprites are objects in OAM)
        Tiled BGs can range from 128x128px to 1024x1024px(affine only), sprites up to 64x64px, each tiled BG + sprite is comprised of 8x8px bitmaps (tiles)
        Palettes and tiles can be 4bpp(16colors/16banked palettes) or 8bpp(256colors/1palette), all affine BGs are 8bpp
        4bpp takes up less space than 8bpp in VRAM since only color index needs to be stored, palette index is also located in VRAM for BGs, OAM for sprites
        In 8-bit color, the pixel value in the tile is the palette-index; in 4-bit color, the lower nibble is the palette index, the higher nibble is the palbank index (can be found in attributes for sprites, or upper nibble of the tiles)
        Sprites have one tile-map entry, but BGs comprised of many. Both BGs and sprites have 10 bits for tile-indices
        VRAM is basically a big bitmap 8 pixels wide, and store the tile data + maps for tile BG modes (attributes for sprites in OAM as well)
        Charblocks - 6 16kb chunks in VRAM, 4 for BGs and 2 for sprites, tile-indexing is different for sprites and BGs
        Charblocks contain tile data and may overlap with screenblocks (for tile maps), so they must be chose carefully
        You must increase the base address for sprite data (from 0x10000 to 0x14000) in VRAM for bitmap modes (since the bitmaps extend into the lower sprite range)
        
        Idea behind render order of a scanline:
        render BGs in line, render sprites in line, compose result
    */

    // todo: implement rotation + scaling (affine), and objs for bitmap modes
    uint8_t vcount = VCOUNT;

    if(vcount < 160) { // if VCOUNT < 160, load update a single scanline, 160-227 is non-visible scanline range

        uint16_t lineStart = vcount * 240;
        uint16_t* scanLine = pixelBuffer + lineStart;
        
        // clear buffers
        for(uint8_t i = 0; i < 4; i++)
            memset(bgLayer[i],0,240);

        switch(DISPCNT_MODE) {
            case 0: // BG[0-3] text/tile BG mode, no affine
                renderTextBG(0,vcount);
                renderTextBG(1,vcount);
                renderTextBG(2,vcount);
                renderTextBG(3,vcount);
                RENDER_SPRITES_AND_COMPOSE(0x10000)
                break;
            case 1: // BG[0-2] text/tile BGs like mode 0, but only BG2 has affine
                renderTextBG(0,vcount);
                renderTextBG(1,vcount);
                renderAffineBG(2);
                RENDER_SPRITES_AND_COMPOSE(0x10000)
                break;
            case 2: // BG[2-3] tiled BGs w/ affine
                renderAffineBG(2);
                renderAffineBG(3);
                RENDER_SPRITES_AND_COMPOSE(0x10000)
                break;
            case 3: // BG[2] bitmap BG mode w/o page flipping, affine
            {
                if(DISPCNT_BG2) {
                    uint16_t* halfwordChunkVram = reinterpret_cast<uint16_t*>(&systemMemory->vram);
                    for(uint8_t i = 0; i < 240; i++) {
                        //applyAffine()
                        scanLine[i] = halfwordChunkVram[lineStart + i];
                    }
                }
                break;
            }
            case 4: // BG[2] paletted bitmap BG mode, affine
            {
                if(DISPCNT_BG2) {
                    uint16_t frameBufferStart = DISPCNT_DISPLAY_FRAME_SELECT ? 0xA000 : 0;
                    uint16_t* halfwordChunkPram = reinterpret_cast<uint16_t*>(&systemMemory->pram);
                    for(uint8_t i = 0; i < 240; i++)
                        // in paletted modes, pixels in an image are represented as 8-bit or 4-bit indexes into pram -
                        // In the case of the bitmapped backgrounds in modes 3 and 4, pixels are represented as the 16-bit color values themselves.
                        scanLine[i] = halfwordChunkPram[systemMemory->vram[frameBufferStart + lineStart + i]];
                }
                break;
            }
            case 5: // BG[2] bitmap BG mode with page flipping, affine
            {
                if(DISPCNT_BG2) {
                    uint8_t row = vcount;
                    uint16_t frameBufferStart = DISPCNT_DISPLAY_FRAME_SELECT ? 0xA000 : 0;
                    uint16_t backgroundColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);// background color is the first index in pram
                    if(row < 128) {
                        for(uint8_t i = 0; i < 160; i++) {
                            uint16_t* halfwordChunkVram = reinterpret_cast<uint16_t*>(&systemMemory->vram);
                            scanLine[i] = halfwordChunkVram[(row * 160 + i) + frameBufferStart];
                        } for(uint8_t i = 160; i < 240; i++)
                            scanLine[i] = backgroundColor;
                    } else
                        for(uint8_t i = 0; i < 240; i++)
                            scanLine[i] = backgroundColor;
                }
            }
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
        std::string title = std::to_string(fps)+" fps - ZeroGBA";
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
