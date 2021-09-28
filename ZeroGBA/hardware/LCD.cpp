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
    std::fill_n(pixelBuffer,38400,0x0000);

    compileShaders();
}

void LCD::renderTextBG(uint8_t bg,uint8_t vcount) {
    if((systemMemory->IORegisters[1] & (1 << bg)) == 0)
        return;
    uint32_t* currLayer = &bgLayer[bg][0];

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

    uint16_t seCalc2Num = 0;
    if(ty >= 32 && bgcntSize == 0xC000)
        seCalc2Num += 0x0400;

    #ifdef AVX2_RENDERER
    
    // i'm so sorry
    const __m256i bghofsVec = _mm256_set1_epi32(bghofs);
    const __m256i widthVec = _mm256_set1_epi32(width);
    const __m256i tyVec = _mm256_set1_epi32(ty);
    const __m256i thirtyTwoVec = _mm256_set1_epi32(32);
    const __m256i oneVec = _mm256_set1_epi32(1);
    const __m256i sevenVec = _mm256_set1_epi32(7);
    const __m256i halfWordMask = _mm256_set1_epi32(0xFFFF);
    const __m256i tidAndVec = _mm256_set1_epi32(0x3FF);
    const __m256i flipXAndVec = _mm256_set1_epi32(0x400);
    const __m256i flipYAndVec = _mm256_set1_epi32(0x800);
    const __m256i seAdjust1 = _mm256_set1_epi32(0x03E0);
    const __m256i initial = _mm256_set_epi32(7,6,5,4,3,2,1,0); // least significant == rightmost
    const __m256i bgTileRowVec = _mm256_set1_epi32(bgTileRow);
    
    const __m256i seCalc2 = _mm256_set1_epi32(seCalc2Num);

    // 30 * 240 is perfectly divisible by 256 (AVX)
    for(uint8_t i = 0; i < 30; i++) {
        // fill vector with 'i'
        __m256i indexVec = _mm256_add_epi32(initial,_mm256_set1_epi32(i*8));

        // calculate bgTileColumns and tx
        __m256i bgTileColumnVec = _mm256_add_epi32(indexVec,bghofsVec);
        bgTileColumnVec = _mm256_and_si256(bgTileColumnVec,widthVec);
        __m256i txVec = _mm256_srli_epi32(bgTileColumnVec,3);

        // screen entries
        __m256i n = _mm256_mullo_epi32(tyVec,thirtyTwoVec);
        n = _mm256_add_epi32(n,txVec);
        __m256i addMask1 = _mm256_cmpgt_epi32(txVec,thirtyTwoVec);
        addMask1 = _mm256_or_si256(addMask1,_mm256_cmpeq_epi32(txVec,thirtyTwoVec));
        n = _mm256_add_epi32(n,_mm256_and_si256(addMask1,seAdjust1));

        n = _mm256_add_epi32(n,seCalc2);
        n = _mm256_and_si256(n,halfWordMask);

        // screenIndex = n
        __m256i screenEntryVec = _mm256_i32gather_epi32(screenBlockBase,_mm256_srli_epi32(n,1),4); // load data from idx / 2
        
        screenEntryVec = _mm256_srlv_epi32(screenEntryVec,_mm256_slli_epi32(_mm256_and_si256(n,oneVec),4)); // shift hi 16-bit data right 16 bits
        screenEntryVec = _mm256_and_si256(screenEntryVec,halfWordMask);

        // tid and tile coordinates in char block
        __m256i tidVec = _mm256_and_si256(screenEntryVec,tidAndVec);
        __m256i flipXVec = _mm256_and_si256(screenEntryVec,flipXAndVec);
        __m256i flipYVec = _mm256_and_si256(screenEntryVec,flipYAndVec);
        __m256i xVec = _mm256_xor_si256(_mm256_and_si256(bgTileColumnVec,sevenVec),_mm256_srli_epi32(_mm256_mullo_epi32(sevenVec,flipXVec),10));
        __m256i yVec = _mm256_xor_si256(_mm256_and_si256(bgTileRowVec,sevenVec),_mm256_srli_epi32(_mm256_mullo_epi32(sevenVec,flipYVec),11));
        xVec = _mm256_and_si256(xVec,halfWordMask);
        yVec = _mm256_and_si256(yVec,halfWordMask);
        
        const __m256i threeVec = _mm256_set1_epi32(3);
        const __m256i byteMask = _mm256_set1_epi32(0xFF);
        if(eightBppEnabled) {
            const __m256i tidOffsetVec = _mm256_set1_epi32(0x40);
            const __m256i eightVec = _mm256_set1_epi32(8);
            __m256i pindexVec = _mm256_add_epi32(_mm256_add_epi32(_mm256_mullo_epi32(tidVec,tidOffsetVec),_mm256_mullo_epi32(yVec,eightVec)),xVec);
            __m256i finalPIndexVec = _mm256_i32gather_epi32(charBlockBase,_mm256_srli_epi32(pindexVec,2),4); // load data from idx / 4
            finalPIndexVec = _mm256_srlv_epi32(finalPIndexVec,_mm256_slli_epi32(_mm256_and_si256(pindexVec,threeVec),3)); // shift data right by (idx & 3) *8
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&currLayer[i*8]),_mm256_and_si256(finalPIndexVec,byteMask));
        } else {
            const __m256i tidOffsetVec = _mm256_set1_epi32(0x20);
            const __m256i fourVec = _mm256_set1_epi32(4);
            const __m256i twelveVec = _mm256_set1_epi32(12);
            const __m256i pBankHashVec = _mm256_set1_epi32(0xF000);
            const __m256i pindexHashVec = _mm256_set1_epi32(0xF);
            const __m256i zeroVec = _mm256_set1_epi32(0);
            __m256i pBankVec = _mm256_srlv_epi32(_mm256_and_si256(screenEntryVec,pBankHashVec),twelveVec);
            __m256i pindexVec = _mm256_add_epi32(_mm256_add_epi32(_mm256_mullo_epi32(tidVec,tidOffsetVec),_mm256_mullo_epi32(yVec,fourVec)),_mm256_srli_epi32(xVec,1));
            __m256i finalPIndexVec = _mm256_i32gather_epi32(charBlockBase,_mm256_srli_epi32(pindexVec,2),4); // load data from idx / 4
            finalPIndexVec = _mm256_srlv_epi32(finalPIndexVec,_mm256_slli_epi32(_mm256_and_si256(pindexVec,threeVec),3)); // shift data right by (idx & 3) * 8
            finalPIndexVec = _mm256_and_si256(finalPIndexVec,byteMask);
            finalPIndexVec = _mm256_and_si256(_mm256_srlv_epi32(finalPIndexVec,_mm256_slli_epi32(_mm256_and_si256(xVec,oneVec),2)),pindexHashVec); // read 4-bit indexes
            finalPIndexVec = _mm256_add_epi32(_mm256_slli_epi32(_mm256_and_si256(_mm256_cmpgt_epi32(finalPIndexVec,zeroVec),pBankVec),4),finalPIndexVec);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&currLayer[i*8]),_mm256_and_si256(finalPIndexVec,byteMask));
        }
    }
    #else
    for(uint8_t i = 0; i < 240; i++) {
        uint16_t bgTileColumn = i + bghofs & width;
        uint16_t tx = bgTileColumn / 8;
        
        uint16_t screenIndex = screenEntryIndexNum1(tx,ty) + seCalc2Num;
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
    #endif
}
void LCD::renderAffineBG(uint8_t bg) {
    if((systemMemory->IORegisters[1] & (1 << bg)) == 0)
        return;

    // I don't check whether or not a non-2 or 3 bg uses this method, but I don't need to :-)

    int32_t internalX = systemMemory->internalRef[bg - 2].x;
    int32_t internalY = systemMemory->internalRef[bg - 2].y;

    int16_t pa = *reinterpret_cast<int16_t*>(&systemMemory->IORegisters[0x10 * bg]);
    int16_t pc = *reinterpret_cast<int16_t*>(&systemMemory->IORegisters[(0x10 * bg) + 0x4]);;
    
    uint32_t* currLayer = bgLayer[bg];
    uint16_t bgcnt = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x8 + (0x2*bg)]);
    uint8_t* charBlockBase = &systemMemory->vram[(((bgcnt & 0xC) >> 2) * 0x4000)];
    uint8_t* screenBlockBase = reinterpret_cast<uint8_t*>(&systemMemory->vram[(((bgcnt & 0x1F00) >> 8) * 0x800)]);
    bool displayOverflow = bgcnt & 0x2000;

    uint8_t bgcntSize = (bgcnt & 0xC000) >> 14;
    uint8_t tileNum = 16 << bgcntSize; // aff BGs are always squares of factor 16x16 tiles
    uint16_t bgSize = tileNum * 8; // in pixels

    for(uint8_t i = 0; i < 240; i++) {
        int32_t pixX = internalX >> 8;
        int32_t pixY = internalY >> 8;

        internalX += pa;
        internalY += pc;

        if(displayOverflow) {
            // this is the expected behavior in sbb_aff.gba
            if (pixX >= bgSize)
                pixX %= bgSize;
            else if (pixX < 0)
                pixX = bgSize + (pixX % bgSize);
      
            if (pixY >= bgSize)
                pixY %= bgSize;
            else if (pixY < 0)
                pixY = bgSize + (pixY % bgSize);
        } else if(pixX >= bgSize || pixY >= bgSize || pixX < 0 || pixY < 0)
            continue;

        // Affine BG screen entries are only 8 bits and only contain the TIDs
        uint8_t tid = screenBlockBase[(pixY/8)*tileNum + pixX/8];
        currLayer[i] = charBlockBase[tid * 0x40 + (pixY % 8)*8 + pixX%8];
    }
}
template<bool bitmappedMode>
void LCD::renderSprites(int16_t vcount) {

    std::fill_n(spriteLayer,240,Sprite{});
    
    uint16_t dispCnt = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0]);
    if((dispCnt & 0x1000) == 0)
        return;

    uint8_t* charBlockBase = &systemMemory->vram[0x10000];

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
                if(eightBitColors)
                    tileRowOffset *= spriteWidth / 4;
                else
                    tileRowOffset *= spriteWidth / 8;
            else // 2D VRAM mapping
                tileRowOffset *= 0x20;
            tileRowOffset += (texX / 8) * (1+eightBitColors);

            uint8_t paletteIndex = 0;

            if(bitmappedMode && tid < 512)
                continue;

            tid = (tid + tileRowOffset) & 0x3FF;
            
            // Unlike BGs, TID for sprites are located in OAM
            if(eightBitColors)
                paletteIndex = charBlockBase[tid * 0x20 + tileY * 8 + tileX];
            else {
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
uint16_t LCD::blendColor(uint8_t eva, uint16_t colorA, uint8_t evb, uint16_t colorB) {
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
uint16_t LCD::blend(uint16_t colorA, uint16_t colorB, uint8_t colorSpecialEffect) {
    switch(colorSpecialEffect) {
        case 0:
            return colorA;
        case 1:
        {
            uint8_t eva = systemMemory->IORegisters[0x52] & 0x1F;
            uint8_t evb = systemMemory->IORegisters[0x53] & 0x1F;
            return blendColor(eva,colorA,evb,colorB);
        }
        case 2:
        {
            uint8_t evy = systemMemory->IORegisters[0x54] & 0x1F;
            return brightnessFade<true>(evy,colorA);
        }
        case 3:
        {
            uint8_t evy = systemMemory->IORegisters[0x54] & 0x1F;
            return brightnessFade<false>(evy,colorA);
        }
    }
}
void LCD::composeScanline(uint16_t* scanline, uint8_t vcount, uint8_t bgMax, uint8_t bgMin) {

    uint16_t winin = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x48]);
    uint8_t win0List = winin & 0x1F;
    uint8_t win1List = (winin & 0x1F00) >> 8;
    uint16_t winout = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters[0x4A]);
    uint8_t outList = winout & 0x1F;
    uint8_t objList = (winout & 0x1F00) >> 8;
    uint16_t dispcnt = *reinterpret_cast<uint16_t*>(&systemMemory->IORegisters);

    uint8_t bldcntFirstTargets = systemMemory->IORegisters[0x50] & 0x3F;
    uint8_t bldcntSecondTargets = systemMemory->IORegisters[0x51] & 0x3F;
    uint8_t colorSpecialEffect = (systemMemory->IORegisters[0x50] & 0xC0) >> 6;

    uint16_t* halfWordPram = reinterpret_cast<uint16_t*>(&systemMemory->pram);
    uint16_t* spriteHalfWordPram = reinterpret_cast<uint16_t*>(&systemMemory->pram[0x200]);
    uint16_t bdColor = *reinterpret_cast<uint16_t*>(&systemMemory->pram[0]);

    bool win0Display, win1Display, objDisplay, win1YVisible, win0YVisible, win0Effects, win1Effects, outEffects, objEffects, specialEffects, spritesEnabled;
    win0Display = dispcnt & 0x2000;
    win1Display = dispcnt & 0x4000;
    objDisplay = dispcnt & 0x8000;
    win0YVisible = WIN0V_Y1 <= vcount && vcount < WIN0V_Y2;
    win1YVisible = WIN1V_Y1 <= vcount && vcount < WIN1V_Y2;
    win0Effects = winin & 0x20;
    win1Effects = winin & 0x2000;
    outEffects = winout & 0x20;
    objEffects = winout & 0x2000;
    spritesEnabled = dispcnt & 0x1000;
    uint8_t enableList = 0, win0x1, win0x2, win1x1, win1x2;
    win0x1 = systemMemory->IORegisters[0x41];
    win0x2 = systemMemory->IORegisters[0x40];
    win1x1 = systemMemory->IORegisters[0x43];
    win1x2 = systemMemory->IORegisters[0x42];

    // presort the enabled BGs by priority from lowest to highest
    uint8_t bgList[4];
    int8_t bgCount = 0;
    for(int8_t priority = 3; priority >= 0; priority--)
        for(int8_t bg = bgMax; bg >= bgMin; bg--)
            if((systemMemory->IORegisters[1] & (1 << bg)) && ((systemMemory->IORegisters[0x8 + (0x2 * bg)] & 0x3) == priority))
                bgList[bgCount++] = bg;

    // variables for top-most visible bg-related things, hi-prio = 0, lower-prio = 1
    uint16_t color[2];
    uint8_t priority[2];
    uint8_t layer[2];

    // composite a scanline from lowest priority window contents (OBJs and BGs) to highest, from highest prio window
    for(uint8_t x = 0; x < 240; x++) {

        if(win0Display && win0x1 <= x && x < win0x2 && win0YVisible) { // WIN0
            enableList = win0List;
            specialEffects = win0Effects;
        } else if(win1Display && win1x1 <= x && x < win1x2 && win1YVisible) { // WIN1
            enableList = win1List;
            specialEffects = win1Effects;
        } else if(objDisplay && spriteLayer[x].window) { // OBJ
            enableList = objList;
            specialEffects = objEffects;
        } else if(win0Display || win1Display || objDisplay) { // WINOUT
            enableList = outList;
            specialEffects = outEffects;
        } else { // no windows
            // even with no windows, BGs are shown    
            enableList = (dispcnt & 0x1F00) >> 8;
            specialEffects = true;
        }

        bool objBlends = false;
        priority[0] = 4;
        layer[0] = BD;

        // find the top-most visible bg pixel
        if(bgCount != 0) {
            int8_t i = bgCount - 1;
            for(;i >= 0;i--) {
                uint8_t bg = bgList[i];
                if(enableList & (1 << bg)) {
                    if(bgLayer[bg][x] != 0) {
                        priority[0] = systemMemory->IORegisters[0x8 + (0x2 * bg)] & 0x3;
                        layer[0] = bg;
                        i--;
                        break;
                    }
                }
            }

            // if it blends
            if(specialEffects) {
                if(colorSpecialEffect != 0) {
                    priority[1] = 4;
                    layer[1] = BD;
                    // find the second top-most visible bg pixel
                    for(;i >= 0;i--) {
                        uint8_t bg = bgList[i];
                        if(enableList & (1 << bg)) {
                            if(bgLayer[bg][x] != 0) {
                                priority[1] = systemMemory->IORegisters[0x8 + (0x2 * bg)] & 0x3;
                                layer[1] = bg;
                                break;
                            }
                        }
                    }
                }
            }

        }

        // Check if OBJ pixel takes priority over one of the two top-most visible bg pixels
        if( (enableList & 0b10000) &&
            spriteLayer[x].pindex != 0 ) {
            if(spriteLayer[x].priority <= priority[0]) {
                layer[1] = layer[0];
                layer[0] = OBJ;
                objBlends = spriteLayer[x].alpha;
            } else if(spriteLayer[x].priority <= priority[1]) {
                layer[1] = OBJ;
            }
        }

        // find color from layer pindexes
        for(int8_t i : {0,1}) {
            switch(layer[i]) {
                case BG0 ... BG3:
                    color[i] = halfWordPram[bgLayer[layer[i]][x]];
                    break;
                case OBJ:
                    color[i] = spriteHalfWordPram[spriteLayer[x].pindex];
                    break;
                case BD:
                    color[i] = bdColor;
                    break;
            }
        }

        // blend that shit baby (if we doin it)
        if(specialEffects || objBlends) {
            bool has1stTarget = bldcntFirstTargets & (1 << layer[0]); // must be on top for blending
            bool has2ndTarget = bldcntSecondTargets & (1 << layer[1]); // must be on bottom
            if(objBlends && has2ndTarget)
                color[0] = blend(color[0],color[1],1);
            else if(colorSpecialEffect != 0 && has1stTarget && (has2ndTarget || colorSpecialEffect != 1))
                color[0] = blend(color[0],color[1],colorSpecialEffect);
        }

        scanline[x] = color[0];
    }
}
void LCD::composeScanlineOffThread() {
    
}

#define RENDER_SPRITES_AND_COMPOSE(bitmappedMode,bgMax,bgMin) \
                        renderSprites<bitmappedMode>(vcount); \
                composeScanline(scanLine,vcount,bgMax,bgMin);
void LCD::renderScanline() {

    // todo: implement rotation + scaling (affine) for bitmap modes
    uint8_t vcount = VCOUNT;

    if(vcount < 160) { // if VCOUNT < 160, load update a single scanline, 160-227 is non-visible scanline range

        uint16_t lineStart = vcount * 240;
        uint16_t* scanLine = pixelBuffer + lineStart;
        
        // clear buffers, 4 * 4 * 240
        if(systemMemory->IORegisters[0] & 0x80) {
            std::fill_n(scanLine,240,0x7FFF);
            return;
        } else
            memset(bgLayer,0,3840);

        switch(DISPCNT_MODE) {
            case 0: // BG[0-3] text/tile BG mode, no affine
                renderTextBG(0,vcount);
                renderTextBG(1,vcount);
                renderTextBG(2,vcount);
                renderTextBG(3,vcount);
                RENDER_SPRITES_AND_COMPOSE(false,3,0)
                break;
            case 1: // BG[0-2] text/tile BGs like mode 0, but only BG2 has affine
                renderTextBG(0,vcount);
                renderTextBG(1,vcount);
                renderAffineBG(2);
                RENDER_SPRITES_AND_COMPOSE(false,2,0)
                break;
            case 2: // BG[2-3] tiled BGs w/ affine
                renderAffineBG(2);
                renderAffineBG(3);
                RENDER_SPRITES_AND_COMPOSE(false,3,2)
                break;
            case 3: // BG[2] bitmap BG mode w/o page flipping, affine
            {
                if(DISPCNT_BG2) {
                    for(uint8_t i = 0; i < 240; i++) {
                        //applyAffine()
                        bgLayer[2][i] = lineStart + i;
                    }
                }
                RENDER_SPRITES_AND_COMPOSE(true,2,2)
                break;
            }
            case 4: // BG[2] paletted bitmap BG mode, affine
            {
                if(DISPCNT_BG2) {
                    uint16_t frameBufferStart = DISPCNT_DISPLAY_FRAME_SELECT ? 0xA000 : 0;
                    for(uint8_t i = 0; i < 240; i++)
                        // in paletted modes, pixels in an image are represented as 8-bit or 4-bit indexes into pram -
                        // In the case of the bitmapped backgrounds in modes 3 and 4, pixels are represented as the 16-bit color values themselves.
                        bgLayer[2][i] = systemMemory->vram[frameBufferStart + lineStart + i];
                }
                RENDER_SPRITES_AND_COMPOSE(true,2,2)
                break;
            }
            case 5: // BG[2] bitmap BG mode with page flipping, affine
            {
                if(DISPCNT_BG2) {
                    uint16_t frameBufferStart = DISPCNT_DISPLAY_FRAME_SELECT ? 0xA000 : 0;
                    if(vcount < 128)
                        for(uint8_t i = 0; i < 160; i++)
                            bgLayer[2][i] = (vcount * 160 + i) + frameBufferStart;
                }
                RENDER_SPRITES_AND_COMPOSE(true,2,2)
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
