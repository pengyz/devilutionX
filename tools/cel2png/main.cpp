#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include "appfat.h"
#include "utils/cel_to_clx.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/palette.h"
#include "engine/surface.hpp"
#include "headless_mode.hpp"
#include "utils/surface_to_png.hpp"

using namespace devilution;

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: cel2png <cel_file> <width> [output.png]\n");
        return 1;
    }
    HeadlessMode = true;
    SDL_Init(SDL_INIT_VIDEO);
    LoadPalette("ui_art\\diablo.pal");
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    auto data = new uint8_t[size];
    fread(data, 1, size, f);
    fclose(f);
    
    int width = atoi(argv[2]);
    const char *output = argc > 3 ? argv[3] : "output.png";
    auto result = CelToClx(data, size, PointerOrValue<uint16_t>(static_cast<uint16_t>(width)));
    delete[] data;
    
    ClxSpriteList list = result.list();
    const ClxSprite sprite = *list.begin();
    printf("%dx%d\n", sprite.width(), sprite.height());
    
    OwnedSurface surface(sprite.width(), sprite.height());
    SDL_memset(surface.at(0,0), 0, surface.pitch() * surface.h());
    RenderClxSprite(surface, sprite, {0,0});
    
    SDL_RWops *rw = SDL_RWFromFile(output, "wb");
    if (!rw) { fprintf(stderr, "Cannot write %s\n", output); return 1; }
    WriteSurfaceToFilePng(surface, rw);
    printf("Saved %s\n", output);
    SDL_Quit();
    return 0;
}
