#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include <SDL_image.h>
#include "appfat.h"
#include "engine/surface.hpp"
#include "engine/clx_sprite.hpp"
#include "utils/surface_to_clx.hpp"
#include "headless_mode.hpp"

using namespace devilution;

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: png2clx <input.png> <output.clx> [num_frames]\n");
        fprintf(stderr, "  num_frames: vertically stacked frames (default 1)\n");
        return 1;
    }
    
    HeadlessMode = true;
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    
    SDL_Surface *img = IMG_Load(argv[1]);
    if (!img) { fprintf(stderr, "Cannot load %s: %s\n", argv[1], IMG_GetError()); return 1; }
    
    int frames = argc > 3 ? atoi(argv[3]) : 1;
    
    // Convert to indexed surface using SDL
    SDL_Surface *indexed = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_INDEX8, 0);
    SDL_FreeSurface(img);
    if (!indexed) { fprintf(stderr, "Cannot convert to indexed\n"); return 1; }
    
    printf("%dx%d, %d frames\n", indexed->w, indexed->h, frames);
    
    Surface surface(indexed);
    auto clx = SurfaceToClx(surface, frames, 0); // palette index 0 = transparent
    ClxSpriteList list(clx);
    
    const char *out = argv[2];
    FILE *f = fopen(out, "wb");
    if (!f) { fprintf(stderr, "Cannot write %s\n", out); return 1; }
    fwrite(list.data(), 1, list.dataSize(), f);
    fclose(f);
    
    printf("Saved %s (%u bytes)\n", out, list.dataSize());
    
    SDL_FreeSurface(indexed);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
