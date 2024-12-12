#include <ap_int.h>
#include <header.hpp>
#include <hls_burst_maxi.h>
#include <hls_math.h>
// 16*(256*128)/1024 = 512KBit =  15BRAMs, size 0x10000 bytes
ap_uint<BULLET_MAP_DEPTH> bullet_sprite[BULLET_MAP_WIDTH * BULLET_MAP_HEIGHT];
game_bullets_t game_bullets;
game_misc_t game_misc;
ap_uint<TILE_DEPTH> tile_fb[TILE_WIDTH * TILE_HEIGHT];
void render_2d(hls::burst_maxi<ap_uint<64>> vram) {
    // ap_ctrl mode: handshake
#pragma HLS INTERFACE mode = ap_ctrl_hs port = return
#pragma HLS INTERFACE mode = m_axi port = vram offset = off
//read_bullet_map:
//    // 32 burst read of 256 beats
//    for (uint32_t i = 0; i < 32; i++) {
//        vram.read_request(BULLET_MAP_ADDR, READ_BURST_BEATS);
//        for (uint32_t j = 0; j < READ_BURST_BEATS; j++) {
//#pragma HLS PIPELINE off
//            ap_uint<AXI_WIDTH> tmp_read = vram.read();
//            uint32_t tmp_addr = (i * READ_BURST_BEATS + j) * 4;
//            bullet_sprite[tmp_addr] = tmp_read(15, 0);
//            bullet_sprite[tmp_addr + 1] = tmp_read(31, 16);
//            bullet_sprite[tmp_addr + 2] = tmp_read(47, 32);
//            bullet_sprite[tmp_addr + 3] = tmp_read(63, 48);
//        }
//        // no response for read
//    }
//read_bullet_info:
//    // 9*1024B/8(AXI bus width)/128=9 BURST
//    for (uint32_t i = 0; i < 9; i++) {
//        vram.read_request(BULLET_INFO_ADDR, 128);
//        for (uint32_t j = 0; j < 128; j++) {
//            ap_uint<AXI_WIDTH> tmp_read = vram.read();
//            uint32_t tmp_addr = 2 * (i * 128 + j);
//            if (tmp_addr < 2048) {
//                game_bullets.enemy_bullets[tmp_addr].x = tmp_read(8, 0);
//                game_bullets.enemy_bullets[tmp_addr].y = tmp_read(17, 9);
//                game_bullets.enemy_bullets[tmp_addr].rotation = tmp_read(26, 18);
//                game_bullets.enemy_bullets[tmp_addr].type = tmp_read(30, 27);
//                game_bullets.enemy_bullets[tmp_addr].valid = tmp_read(31, 31);
//                game_bullets.enemy_bullets[tmp_addr + 1].x = tmp_read(40, 32);
//                game_bullets.enemy_bullets[tmp_addr + 1].y = tmp_read(49, 41);
//                game_bullets.enemy_bullets[tmp_addr + 1].rotation = tmp_read(58, 50);
//                game_bullets.enemy_bullets[tmp_addr + 1].type = tmp_read(62, 59);
//                game_bullets.enemy_bullets[tmp_addr + 1].valid = tmp_read(63, 63);
//            } else {
//                game_bullets.player_bullets[tmp_addr - 2048].x = tmp_read(8, 0);
//                game_bullets.player_bullets[tmp_addr - 2048].y = tmp_read(17, 9);
//                game_bullets.player_bullets[tmp_addr - 2048].rotation = tmp_read(26, 18);
//                game_bullets.player_bullets[tmp_addr - 2048].type = tmp_read(30, 27);
//                game_bullets.player_bullets[tmp_addr - 2048].valid = tmp_read(31, 31);
//                game_bullets.player_bullets[tmp_addr - 2048 + 1].x = tmp_read(40, 32);
//                game_bullets.player_bullets[tmp_addr - 2048 + 1].y = tmp_read(49, 41);
//                game_bullets.player_bullets[tmp_addr - 2048 + 1].rotation = tmp_read(58, 50);
//                game_bullets.player_bullets[tmp_addr - 2048 + 1].type = tmp_read(62, 59);
//                game_bullets.player_bullets[tmp_addr - 2048 + 1].valid = tmp_read(63, 63);
//            }
//        }
//        // no response for read
//    }
//read_game_misc:
//    ap_uint<AXI_WIDTH> tmp_buffer;
//    ap_uint<AXI_WIDTH> *read_game_misc = (ap_uint<AXI_WIDTH> *)(&game_misc);
//
//    for (uint32_t i = 0; i < 10; i++) {
//        vram.read_request(GAME_MISC_ADDR + AXI_WIDTH * i / 8, 1);
//        read_game_misc[i] = vram.read();
//    }
render_frame_tile_y:
    for (int i = 0; i < TILE_Y_COUNT; i++) {
    render_frame_tile_x:
        for (int j = 0; j < TILE_X_COUNT; j++) {
//        render_enemy_bullets:
//            for (int k = 0; k < TILE_HEIGHT; k++) {
//            render_enemy_bullets_x:
//                for (int l = 0; l < TILE_WIDTH; l++) {
//                render_enemy_bullets_y:
//                    ap_uint<TILE_DEPTH> tmp_pixel = 0xFFFF; // intermediate pixel
////                    for (int m = 0; m < MAX_ENEMY_BULLETS_IN_TILE; m++) {
////                    render_enemy_enum_bullets:
////                        if (game_bullets.enemy_bullets[m].valid) {
////                            sprite_t tmp_sprite = get_enemy_bullet_info(game_bullets.enemy_bullets[m].type);
////                            // TODO rotation and then check whether on current pixel
////                            if (game_bullets.enemy_bullets[m].x == j * TILE_WIDTH + l && game_bullets.enemy_bullets[m].y == i * TILE_HEIGHT + k) {
////                                tmp_pixel = bullet_sprite[tmp_sprite.y * BULLET_MAP_WIDTH + tmp_sprite.x];
////                            }
////                        }
////                    }
//                    tile_fb[k * TILE_HEIGHT + l] = k * TILE_HEIGHT + l;
//                }
//            }
            // 32 pixel, 32*4=128 Byte = 16 Beat*8Byte
            // flush the tile to DDR
        flush_tile_y:
            for (int k = 0; k < TILE_HEIGHT; k++) {
                vram.write_request((FB0_BASE + ((FB_START_Y+i * TILE_HEIGHT + k) * FB_WIDTH + (FB_START_X+j * TILE_WIDTH)) * PIX_DEPTH / 8)/8, 16);
//            	vram.write_request(0x01000000, 16);
            flush_tile_x:
                for (int l = 0; l < 16; l++) {
#pragma HLS PIPELINE off
                    // lower 16bit for first 1 pixel, upper 16bit for second pixel
//                    vram.write(RGB_DITHER(tile_fb[k * TILE_WIDTH + 2 * l]) | (RGB_DITHER(tile_fb[k * TILE_WIDTH + 2 * l + 1]) << 32));
                    vram.write(0xFF00000000FF0000);
                }
                vram.write_response();
            }
        }
    }
}
