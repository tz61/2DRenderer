#include <cstddef>
#include <header.hpp>
#include <iostream>
#include <string>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

// void trinity_renderer(fb_id_t fb_id, ap_uint<128> *vram, ap_uint<9> angle);
// 0x12c000 for 1 single frame buffer
uint64_t vram[0x4B0000 / 8];
int main() {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("2DRenderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FB_WIDTH, FB_HEIGHT, 0);
    // load RGB555 bullet sprite called bullet.bmp
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, FB_WIDTH, FB_HEIGHT);
    for (int i = 0; i < 640 * 240 / 2; i++) {
        // populate with red
        vram[i] = 0xFF0000FFFF000000;
    }
    bool quit = false;
    int fb_id = 0;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);
        //         SDL_RenderClear(renderer);
        //
        Uint64 start = SDL_GetPerformanceCounter();
        //         trinity_renderer(fb_id, vram, (SDL_GetTicks() / 14) % 360);
        Uint64 end = SDL_GetPerformanceCounter();
        float fps = SDL_GetPerformanceFrequency() / static_cast<float>(end - start);
        SDL_SetWindowTitle(window, ("2DRenderer, FPS: " + std::to_string(fps)).c_str());

        SDL_UpdateTexture(texture, nullptr, vram, FB_WIDTH * 4);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        //
        SDL_RenderPresent(renderer);
        //
        //         fb_id = (fb_id + 1) % 1;
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
