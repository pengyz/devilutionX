#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include "engine/palette.h"
#include "engine/assets.hpp"
#include "utils/cel_to_clx.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/surface.hpp"
#include "game_mode.hpp"
#include "utils/paths.h"

using namespace devilution;

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Usage: cel2png <cel> <width> [out.png] [mpqdir]\n"); return 1; }
    SDL_Init(SDL_INIT_VIDEO);
    paths::SetBasePath(""); paths::SetPrefPath("/tmp");
    paths::SetAssetsPath(argc > 4 ? argv[4] : ".");
    HeadlessMode = false;
    LoadCoreArchives();
    LoadPalette("ui_art\\diablo.pal");
    HeadlessMode = true;
    
    FILE *f = fopen(argv[1], "rb");
    fseek(f, 0, SEEK_END); size_t size = ftell(f); fseek(f, 0, SEEK_SET);
    auto data = new uint8_t[size]; fread(data, 1, size, f); fclose(f);
    auto result = CelToClx(data, size, PointerOrValue<uint16_t>((uint16_t)atoi(argv[2])));
    delete[] data;
    ClxSpriteList list = result.list();
    const ClxSprite sprite = *list.begin();
    
    int w = sprite.width(), h = sprite.height();
    printf("%dx%d\n", w, h);
    
    // Dump raw pixels and palette
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 8, SDL_PIXELFORMAT_INDEX8);
    SDL_memset(surf->pixels, 0, surf->pitch * h);
    SDL_Palette *pal = SDL_AllocPalette(256);
    SDL_SetPaletteColors(pal, logical_palette.data(), 0, 256);
    SDL_SetSurfacePalette(surf, pal);
    Surface s(surf);
    RenderClxSprite(s, sprite, {0,0});
    
    // Save raw pixels (1 byte per pixel, palette index)
    const char *out = argc > 3 ? argv[3] : "output";
    FILE *raw = fopen(out, "wb");
    for (int y = 0; y < h; y++)
        fwrite((uint8_t*)surf->pixels + y*surf->pitch, 1, w, raw);
    fclose(raw);
    
    // Save palette
    std::string palfile = std::string(out) + ".pal";
    FILE *pf = fopen(palfile.c_str(), "wb");
    for (int i = 0; i < 256; i++) {
        fputc(logical_palette[i].r, pf);
        fputc(logical_palette[i].g, pf);
        fputc(logical_palette[i].b, pf);
    }
    fclose(pf);
    
    printf("Saved raw: %s (%dx%d) + palette: %s\n", out, w, h, palfile.c_str());
    SDL_FreePalette(pal); SDL_FreeSurface(surf); SDL_Quit();
    return 0;
}
