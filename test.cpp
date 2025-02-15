#include <cstddef>
#include <header.hpp>
#include <iostream>
#include <string>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <fstream>
// void trinity_renderer(fb_id_t fb_id, ap_uint<128> *vram, ap_uint<9> angle);
// 0x12c000 for 1 single frame buffer
ap_uint<64> vram[0x4B0000 / 8];
ap_uint<64> bullet_map[(256 * 128 * 2) / 8];
ap_uint<64> game_info_ram[0x4000 / 8];
uint16_t Bullet_sprite[128 * 256];
void render_2d(ap_uint<64> *vram, ap_uint<64> *game_info_ram, ap_uint<64> *bullet_map, ap_uint<1> fb1_alt);
uint64_t compose_entity(uint32_t X, uint32_t Y, uint32_t ROT, uint32_t TYPE, uint32_t VALID) {
    return ((X) | ((Y) << 9) | ((ROT) << 18) | ((TYPE) << 27) | ((VALID) << 31));
}
int main() {
    // load from file: Bullets.img
    std::ifstream file("/home/ztn/Embedded/ip_repo/2DRenderer/Bullets.img", std::ios::binary);
    file.read(reinterpret_cast<char *>(Bullet_sprite), 128 * 256 * 2);
    file.close();
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow("2DRenderer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, FB_WIDTH, FB_HEIGHT, 0);
    // load RGB555 bullet sprite called bullet.bmp
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, FB_WIDTH, FB_HEIGHT);
    for (int i = 0; i < 640 * 480 / 2; i++) {
        // populate with red
        vram[i] = 0xFF0000FFFF000000;
    }
    for (int i = 0; i < 0x4000 / 8; i++) {
        game_info_ram[i] = 0;
    }
    for (int i = 0; i < 256 * 128 / 4; i++) {
        bullet_map[i] = (ap_uint<64>)Bullet_sprite[4 * i] | ((ap_uint<64>)Bullet_sprite[4 * i + 1] << 16) | ((ap_uint<64>)Bullet_sprite[4 * i + 2] << 32) |
                        ((ap_uint<64>)Bullet_sprite[4 * i + 3] << 48);
    }
    // Enemy bullet test
    game_info_ram[0] = (compose_entity(0, 0, 0, 0, 1)) | (compose_entity(16, 16, 0, 1, 1) << 32);
    game_info_ram[1] = (compose_entity(16, 16, 0, 14, 1)) | (compose_entity(241, 126, 0, 15, 1) << 32);
    // Player bullet test(random x y)
    game_info_ram[1576/2] = (compose_entity(128, 0, 0, 0, 1)) | (compose_entity(128, 16, 0, 1, 1) << 32);
    bool quit = false;
    int fb_id = 0;
    uint32_t x=0,y=0;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            }
        }

        game_info_ram[1] = (compose_entity(x, y, 0, 14, 1));
        x++;y++;
        if(x>300)x=0;
        if(y>300)y=0;
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);
        //         SDL_RenderClear(renderer);
        //
        Uint64 start = SDL_GetPerformanceCounter();
        //         trinity_renderer(fb_id, vram, (SDL_GetTicks() / 14) % 360);
        Uint64 end = SDL_GetPerformanceCounter();
        float fps = SDL_GetPerformanceFrequency() / static_cast<float>(end - start);
        SDL_SetWindowTitle(window, ("2DRenderer, FPS: " + std::to_string(fps)).c_str());
        render_2d(vram, game_info_ram, bullet_map, 0);
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
