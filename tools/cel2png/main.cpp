#include <cstdio>
#include <cstdlib>
#include <string>
#include <SDL.h>
#include "appfat.h"
#include "utils/cel_to_clx.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/palette.h"
#include "engine/surface.hpp"
#include "engine/assets.hpp"
#include "headless_mode.hpp"
#include "utils/paths.h"
#include "utils/surface_to_png.hpp"

using namespace devilution;

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: cel2png <cel_file> <width> [output.png] [-p palette.pal]\n");
        fprintf(stderr, "If -p is not given, tries diablo.pal in current dir.\n");
        return 1;
    }
    HeadlessMode = true;
    SDL_Init(SDL_INIT_VIDEO);
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    auto data = new uint8_t[size];
    fread(data, 1, size, f);
    fclose(f);
    
    int width = atoi(argv[2]);
    const char *output = "output.png";
    const char *palFile = "diablo.pal";
    
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i+1 < argc)
            palFile = argv[++i];
        else if (argv[i][0] != '-')
            output = argv[i];
    }
    
    // Load palette from file
    std::array<SDL_Color, 256> pal = {};
    FILE *pf = fopen(palFile, "rb");
    if (pf) {
        for (int i = 0; i < 256; i++) {
            pal[i].r = fgetc(pf);
            pal[i].g = fgetc(pf);
            pal[i].b = fgetc(pf);
        }
        fclose(pf);
        printf("Loaded palette: %s\n", palFile);
    } else {
        fprintf(stderr, "No palette file (%s), using grayscale\n", palFile);
        for (int i = 0; i < 256; i++)
            pal[i].r = pal[i].g = pal[i].b = i;
    }
    
    auto result = CelToClx(data, size, PointerOrValue<uint16_t>(static_cast<uint16_t>(width)));
    delete[] data;
    
    ClxSpriteList list = result.list();
    const ClxSprite sprite = *list.begin();
    printf("%dx%d\n", sprite.width(), sprite.height());
    
    OwnedSurface surface(sprite.width(), sprite.height());
    if (surface.surface->format->palette)
        SDL_SetPaletteColors(surface.surface->format->palette, pal.data(), 0, 256);
    SDL_memset(surface.at(0,0), 0, surface.pitch() * surface.h());
    RenderClxSprite(surface, sprite, {0,0});
    
    SDL_Surface *rgb = SDL_ConvertSurfaceFormat(surface.surface, SDL_PIXELFORMAT_RGB24, 0);
    Surface rgbSurface(rgb);
    SDL_RWops *rw = SDL_RWFromFile(output, "wb");
    if (!rw) { fprintf(stderr, "Cannot write %s\n", output); return 1; }
    WriteSurfaceToFilePng(rgbSurface, rw);
    SDL_FreeSurface(rgb);
    printf("Saved %s\n", output);
    SDL_Quit();
    return 0;
}
