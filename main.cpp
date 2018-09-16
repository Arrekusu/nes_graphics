#include <iostream>
#include <fstream>
#include <vector>
#include "SDL.h"

const int TILE_SIZE = 8;
const int SCREEN_WIDTH = 16 * TILE_SIZE;
const int SCREEN_HEIGHT = 16 * TILE_SIZE;
const int NAME_TABLE_SIZE = 960;

const int PRG_ROM_SIZE = 0x4000;
const int CHR_ROM_SIZE = 0x2000;

struct ines_header
{
    uint8_t magic[4];       // 0x4E, 0x45, 0x53, 0x1A
    uint8_t prg_size;       // PRG ROM in 16K
    uint8_t chr_size;       // CHR ROM in 8K, 0 -> using CHR RAM
    uint8_t flag6;
    uint8_t flag7;
    uint8_t prg_ram_size;   // PRG RAM in 8K
    uint8_t flag9;
    uint8_t flag10;         // unofficial
    uint8_t reserved[5];    // reserved
};


Uint32 makeABGR(Uint8 b, Uint8 g, Uint8 r) {
    return static_cast<Uint32>((255 << 24) | (b << 16) | (g << 8) | r);
}

int main(int argc, char* args[]) {
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("Cannot init SDL!! Error: %s\n", SDL_GetError());
    } else {
        window = SDL_CreateWindow("SDL Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * 4, SCREEN_HEIGHT * 4, SDL_WINDOW_SHOWN);
        if (window == NULL){
            printf("Cannot create window. Error: %s\n", SDL_GetError());
        } else {
            surface = SDL_GetWindowSurface(window);
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
            SDL_UpdateWindowSurface(window);
        }
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetScale(renderer, 4, 4);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
    auto* pixels = new Uint32[SCREEN_WIDTH * SCREEN_HEIGHT];

    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open("/home/alex/CLionProjects/graph/dh.nes", std::ifstream::in | std::ifstream::binary);

    // Parse header
    ines_header header;
    file.read((char *)&header, sizeof(header));

    memset(pixels, 255, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Uint32));

    auto prg_rom = std::vector<Uint8>(static_cast<unsigned long>(PRG_ROM_SIZE * header.prg_size));
    auto chr_rom = std::vector<Uint8>(static_cast<unsigned long>(CHR_ROM_SIZE * header.chr_size));
    auto nameTable1 = std::vector<Uint8>(static_cast<unsigned long>(NAME_TABLE_SIZE));
    file.read((char *)prg_rom.data(), prg_rom.size());
    file.read((char *)chr_rom.data(), chr_rom.size());
    file.read((char *)nameTable1.data(), nameTable1.size());

    file.close();

    Uint32 palette[] = {makeABGR(200, 0, 0), makeABGR(0, 200, 0), makeABGR(0, 0, 200)};

    int count = 0x1000;
    for(int row = 0; row < SCREEN_HEIGHT; row += 8){
        for(int col = 0; col < SCREEN_WIDTH; col += 8){
            for (int k = 0; k < 8; k++){
                Uint8 tileRow2 = chr_rom[count];
                Uint8 tileRow1 = chr_rom[count+8];
                for(int i = 0; i < 8; i++){
                    Uint8 value = static_cast<Uint8>((((tileRow1 & 0x80) != 0) << 1) | (((tileRow2 & 0x80) != 0)));
                    pixels[(row + k) * SCREEN_WIDTH + col + i] = palette[value];
                    tileRow1 <<= 1;
                    tileRow2 <<= 1;
                }
                count++;
            }
            count += 8;
        }
    }

    bool quit = false;
    SDL_Event event;
    while (!quit){
        SDL_WaitEvent(&event);

        switch(event.type){
            case SDL_QUIT: quit = true; break;
        }

        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
