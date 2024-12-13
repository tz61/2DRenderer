#include <ap_int.h>
#include <header.hpp>
#include <hls_burst_maxi.h>
#include <hls_math.h>
// just for vscode
// #define __SYNTHESIS__
// 16*(256*128)/1024 = 512KBit =  15BRAMs, size 0x10000 bytes
#ifndef __SYNTHESIS__
void render_2d(ap_uint<64> *vram, ap_uint<64> *game_info_ram, ap_uint<64> *bullet_map, ap_uint<1> fb1_alt) {
#else
void render_2d(hls::burst_maxi<ap_uint<64>> vram,
               ap_uint<1> fb1_alt) { // fb1_alt=1 then render to alt fb
// ap_ctrl mode: handshake
#endif

	ap_uint<64> bullet_sprite[BULLET_MAP_WIDTH * BULLET_MAP_HEIGHT / 4];
	game_info_t game_info;
	ap_uint<TILE_DEPTH> tile_fb[TILE_WIDTH * TILE_HEIGHT];
#pragma HLS INTERFACE mode = ap_ctrl_hs port = return
#pragma HLS INTERFACE mode = m_axi port = vram offset = off
#ifndef __SYNTHESIS__
    for (uint32_t i = 0; i < 32; i++) {
        for (uint32_t j = 0; j < READ_BURST_BEATS; j++) {
            ap_uint<AXI_WIDTH> tmp_read = *bullet_map;
            bullet_sprite[i * READ_BURST_BEATS + j] = tmp_read;
            bullet_map++;
        }
    }
#else
read_bullet_map:
    // 32 burst read of 256 beats
    for (uint32_t i = 0; i < 32; i++) {
        vram.read_request(BULLET_MAP_ADDR / 8, READ_BURST_BEATS);
        for (uint32_t j = 0; j < READ_BURST_BEATS; j++) {
#pragma HLS PIPELINE off
            ap_uint<AXI_WIDTH> tmp_read = vram.read();
            bullet_sprite[i * READ_BURST_BEATS + j] = tmp_read;
        }
        // no response for read
    }
#endif
#ifndef __SYNTHESIS__
    for (uint32_t i = 0; i < 8; i++) {
        for (uint32_t j = 0; j < READ_BURST_BEATS; j++) {
            uint64_t test_data = *game_info_ram;
            ap_uint<AXI_WIDTH> tmp_read = *game_info_ram;
            uint32_t tmp_addr = 2 * (i * READ_BURST_BEATS + j);
            if (tmp_addr < 2688) {
                game_info.enemy_bullets[tmp_addr] = tmp_read(31, 0);
                game_info.enemy_bullets[tmp_addr + 1] = tmp_read(63, 32);
            } else if (tmp_addr < 2688 + 1344) {
                game_info.player_bullets[tmp_addr - 2688] = tmp_read(31, 0);
                game_info.player_bullets[tmp_addr + 1 - 2688] = tmp_read(63, 32);
            } else {
                game_info.entities[tmp_addr - 4032] = tmp_read(31, 0);
                game_info.entities[tmp_addr + 1 - 4032] = tmp_read(63, 32);
            }
            game_info_ram++;
        }
    }
#else
read_game_info:
    // 8 burst read of 256 beats
    for (uint32_t i = 0; i < 8; i++) {
        vram.read_request(GAME_INFO_ADDR / 8, READ_BURST_BEATS);
        for (uint32_t j = 0; j < READ_BURST_BEATS; j++) {
#pragma HLS PIPELINE off
            ap_uint<AXI_WIDTH> tmp_read = vram.read();
            uint32_t tmp_addr = 2 * (i * READ_BURST_BEATS + j);
            if (tmp_addr < 2688) {
                game_info.enemy_bullets[tmp_addr] = tmp_read(31, 0);
                game_info.enemy_bullets[tmp_addr + 1] = tmp_read(63, 32);
            } else if (tmp_addr < 2688 + 1344) {
                game_info.player_bullets[tmp_addr - 2688] = tmp_read(31, 0);
                game_info.player_bullets[tmp_addr + 1 - 2688] = tmp_read(63, 32);
            } else {
                game_info.entities[tmp_addr - 4032] = tmp_read(31, 0);
                game_info.entities[tmp_addr + 1 - 4032] = tmp_read(63, 32);
            }
        }
        // no response for read
    }
#endif
render_frame_tile_y:
    for (int i = 0; i < TILE_Y_COUNT; i++) {
#pragma HLS PIPELINE off
        render_frame_tile_x:
        for (int j = 0; j < TILE_X_COUNT; j++) {
#pragma HLS PIPELINE off
            clear_tile_y:
            for (int k = 0; k < TILE_HEIGHT; k++) {
            clear_tile_x:
                for (int l = 0; l < TILE_WIDTH; l++) {
#pragma HLS PIPELINE off
                	// used for testing
                    //tile_fb[k * TILE_WIDTH + l] = k<<11|l<<6|(4<<1)|0;
                	tile_fb[k * TILE_WIDTH + l] = 0;
                }
            }
        render_enemy_bullets:
            for (int k = 0; k < TILE_HEIGHT; k++) {
#pragma HLS PIPELINE off
            render_enemy_bullets_x:
                for (int l = 0; l < TILE_WIDTH; l++) {
#pragma HLS PIPELINE off
                render_enemy_bullets_y:
                    ap_uint<TILE_DEPTH> tmp_pixel = tile_fb[k * TILE_WIDTH + l]; // not drawing
                    for (int m = 0; m < MAX_ENEMY_BULLETS_IN_TILE; m++) {
                    render_enemy_enum_bullets:
                        ap_uint<32> tmp_bullet = game_info.enemy_bullets[(i * TILE_X_COUNT + j) * MAX_ENEMY_BULLETS_IN_TILE + m];
                        uint32_t tmp_bullet_aa = tmp_bullet;
                        if (GET_VALID(tmp_bullet) == 1) { // if valid
                            sprite_t tmp_sprite = get_enemy_bullet_info(GET_TYPE(tmp_bullet));
                            // TODO rotation and then check whether on current pixel
                            // cur_pos_y: i*TILE_HEIGHT+k
                            // cur_pos_x: j*TILE_WIDTH+l
                            // if 0 <= cur_pos_x-GET_X(tmp_bullet) < tmp_sprite.width and
                            //    0 <= cur_pos_y-GET_Y(tmp_bullet) < tmp_sprite.height then
                            // offset_x <= cur_pos_x-GET_X(tmp_bullet)
                            // offset_y <= cur_pos_y-GET_Y(tmp_bullet)
                            // sprite_addr <= (tmp_sprite.y+offset_y) * 256 + (tmp_sprite.x+offset_x)
                            // tmp_color   <= bullet_sprite[sprite_addr]
                            // if tmp_color&0xFFFE != 0 then draw it(tmp_pixel = tmp_color)
                            // after 16 bullets are checked, take last one to write to tile_fb
                            int cur_pos_x = j * TILE_WIDTH + l;
                            int cur_pos_y = i * TILE_HEIGHT + k;
                            if (!(GET_X(tmp_bullet) <= cur_pos_x && cur_pos_x < GET_X(tmp_bullet) + tmp_sprite.width)) {
                                continue;
                            }
                            if (!(GET_Y(tmp_bullet) <= cur_pos_y && cur_pos_y < GET_Y(tmp_bullet) + tmp_sprite.height)) {
                                continue;
                            }
                            int offset_x = cur_pos_x - GET_X(tmp_bullet);
                            int offset_y = cur_pos_y - GET_Y(tmp_bullet);
                            int sprite_addr = (tmp_sprite.y + offset_y) * 256 + (tmp_sprite.x + offset_x);
                            ap_uint<16> tmp_color = ((bullet_sprite[sprite_addr / 4] >> ((sprite_addr & 0x3) * 16))) & 0xFFFF;
                            if ((tmp_color & 0xFFFE) != 0x1204) {
                                tmp_pixel = tmp_color;
                            }
                        } else {
                            continue;
                        }
                    } // end of render_enemy_enum_bullets
                    tile_fb[k * TILE_WIDTH + l] = tmp_pixel;
                }
            }
// 32 pixel, 32 * 4 = 128 Byte = 16 Beat * 8Byte flush the tile to DDR
#ifdef __SYNTHESIS__
        flush_tile_y:
            for (int k = 0; k < TILE_HEIGHT; k++) {
                vram.write_request((FB1_BASE + ((FB_START_Y + i * TILE_HEIGHT + k) * FB_WIDTH + (FB_START_X + j * TILE_WIDTH)) * PIX_DEPTH / 8) / 8, 16);
            flush_tile_x:
                for (int l = 0; l < 16; l++) {
#pragma HLS PIPELINE off
                    // lower 16bit for first 1 pixel, upper 16bit for second pixel
                    vram.write((RGB_DITHER(tile_fb[k * TILE_WIDTH + 2 * l])) | ((RGB_DITHER(tile_fb[k * TILE_WIDTH + 2 * l + 1]) << (32))));
                }
                vram.write_response();
            }
#else
            ap_uint<64> *dest_vram;
            for (int k = 0; k < TILE_HEIGHT; k++) {
                dest_vram = vram + (0 + ((FB_START_Y + i * TILE_HEIGHT + k) * FB_WIDTH + (FB_START_X + j * TILE_WIDTH)) * PIX_DEPTH / 8) / 8;
                for (int l = 0; l < 16; l++) {
                    *dest_vram = (RGB_DITHER((ap_uint<64>)tile_fb[k * TILE_WIDTH + 2 * l])) | ((RGB_DITHER((ap_uint<64>)tile_fb[k * TILE_WIDTH + 2 * l + 1]) << (32)));
                    dest_vram++;
                }
            }
#endif
        }
    }
}
