#include <ap_int.h>
#include <header.hpp>
#include <hls_burst_maxi.h>
#include <hls_math.h>
// just for vscode linter
// #define __SYNTHESIS__
// 16*(256*128)/1024 = 512KBit =  15BRAMs, size 0x10000 bytes
uint32_t compose_entity_32(uint32_t X, uint32_t Y, uint32_t ROT, uint32_t TYPE, uint32_t VALID) {
    return ((X) | ((Y) << 9) | ((ROT) << 18) | ((TYPE) << 27) | ((VALID) << 31));
}
#ifndef __SYNTHESIS__
void render_2d(ap_uint<64> *vram, ap_uint<64> *game_info_ram, ap_uint<64> *bullet_map, ap_uint<1> fb1_alt) {
#else
void render_2d(hls::burst_maxi<ap_uint<64>> vram,
               ap_uint<1> fb1_alt) { // fb1_alt=1 then render to alt fb
// ap_ctrl mode: handshake
#endif

    ap_uint<64> bullet_sprite[BULLET_MAP_WIDTH * BULLET_MAP_HEIGHT / 4];
    grid_info_t grid_info;
    ap_uint<32> enemy_bullet[1536];
    ap_uint<32> player_bullet[256];
    ap_uint<32> entities[256];
    ap_uint<TILE_DEPTH> tile_fb[TILE_WIDTH * TILE_HEIGHT];
#pragma HLS INTERFACE mode = ap_ctrl_hs port = return
#pragma HLS INTERFACE mode = m_axi port = vram offset = off
#ifndef __SYNTHESIS__
    for (uint32_t i = 0; i < 128; i++) {
        for (uint32_t j = 0; j < 32; j++) {
            ap_uint<AXI_WIDTH> tmp_read = *bullet_map;
            bullet_sprite[i * 32 + j] = tmp_read;
            bullet_map++;
        }
        bullet_map += 32;
    }
#else
read_bullet_map:
    // 32 burst read of 256 beats
    // new: 128 burst read of 32 beats(128 width)
    for (uint32_t i = 0; i < 128; i++) {
        vram.read_request(i * 64 + BULLET_MAP_ADDR / 8, 32);
        for (uint32_t j = 0; j < 32; j++) {
#pragma HLS PIPELINE off
            ap_uint<AXI_WIDTH> tmp_read = vram.read();
            bullet_sprite[i * 32 + j] = tmp_read;
        }
        // no response for read
    }
#endif
#ifndef __SYNTHESIS__
    for (uint32_t i = 0; i < 4; i++) {
        for (uint32_t j = 0; j < READ_BURST_BEATS; j++) {
            uint64_t test_data = *game_info_ram;
            ap_uint<AXI_WIDTH> tmp_read = *game_info_ram;
            uint32_t tmp_addr = 2 * (i * READ_BURST_BEATS + j);
            if (tmp_addr < 1536) {
                enemy_bullet[tmp_addr] = tmp_read(31, 0);
                enemy_bullet[tmp_addr + 1] = tmp_read(63, 32);
            } else if (tmp_addr < 1536 + 256) {
                player_bullet[tmp_addr - 1536] = tmp_read(31, 0);
                player_bullet[tmp_addr + 1 - 1536] = tmp_read(63, 32);
            } else {
                entities[tmp_addr - 1792] = tmp_read(31, 0);
                entities[tmp_addr + 1 - 1792] = tmp_read(63, 32);
            }
            game_info_ram++;
        }
    }
#else
read_game_info:
    // 4 burst read of 256 beats
    for (uint32_t i = 0; i < 4; i++) {
        vram.read_request(i * READ_BURST_BEATS + GAME_INFO_ADDR / 8, READ_BURST_BEATS);
        for (uint32_t j = 0; j < READ_BURST_BEATS; j++) {
#pragma HLS PIPELINE off
            ap_uint<AXI_WIDTH> tmp_read = vram.read();
            uint32_t tmp_addr = 2 * (i * READ_BURST_BEATS + j);
            if (tmp_addr < 1536) {
                enemy_bullet[tmp_addr] = tmp_read(31, 0);
                enemy_bullet[tmp_addr + 1] = tmp_read(63, 32);
            } else if (tmp_addr < 1536 + 256) {
                player_bullet[tmp_addr - 1536] = tmp_read(31, 0);
                player_bullet[tmp_addr + 1 - 1536] = tmp_read(63, 32);
            } else {
                entities[tmp_addr - 1792] = tmp_read(31, 0);
                entities[tmp_addr + 1 - 1792] = tmp_read(63, 32);
            }
        }
        // no response for read
    }
#endif
map_enemy_vram:
    static uint16_t bucket[12 * 14]; // bucket for each tile, max 16 bullets for each tile
    // clear bucket
    for (int i = 0; i < 12 * 14; i++) {
#pragma HLS PIPELINE off
    	bucket[i] = 0;
    }
    for (int i = 0; i < 2688; i++) {
#pragma HLS PIPELINE off
    	grid_info.enemy_bullets[i] = 0;
    }
    for (int i = 0; i < 1536; i++) {
#pragma HLS PIPELINE off
        ap_uint<32> tmp_b = enemy_bullet[i];
        if (GET_VALID(tmp_b)) {
            sprite_t info = get_enemy_bullet_info(GET_TYPE(tmp_b));
            int tile_x = GET_X(tmp_b) / 32;
            int tile_y = GET_Y(tmp_b) / 32;
            // right/bottom corner
            int tile_x_end = (GET_X(tmp_b) + info.width) / 32;
            int tile_y_end = (GET_Y(tmp_b) + info.height) / 32;
            for (int k = 0; k <= 11; k++) {
                for (int l = 0; l <= 13; l++) {
#pragma HLS PIPELINE off
                    int tile_idx = l * 12 + k;
                    if (tile_x <= k && k <= tile_x_end && tile_y <= l && l <= tile_y_end) {
                        // find a empty slot in this tile
                        if (bucket[tile_idx] < 16) {
                            grid_info.enemy_bullets[tile_idx * 16 + bucket[tile_idx]] = compose_entity_32(GET_X(tmp_b), GET_Y(tmp_b), 0, GET_TYPE(tmp_b), 1);
                            bucket[tile_idx]++;
                        } // else just ignore this bullet
                    }
                }
            }
        }
    }
map_player_vram:
    // clear bucket
    for (int i = 0; i < 12 * 14; i++) {
#pragma HLS PIPELINE off
    	bucket[i] = 0;
    }
    // clear the vram
    for (int i = 0; i < 1344; i++) {
#pragma HLS PIPELINE off
    	grid_info.player_bullets[i] = 0;
    }
    for (int i = 0; i < 256; i++) {
#pragma HLS PIPELINE off
        ap_uint<32> tmp_b = player_bullet[i];
        if (GET_VALID(tmp_b)) {
            sprite_t info = get_player_bullet_info(GET_TYPE(tmp_b));
            // left/top corner
            int tile_x = GET_X(tmp_b) / 32;
            int tile_y = GET_Y(tmp_b) / 32;
            // right/bottom corner
            int tile_x_end = (GET_X(tmp_b) + info.width) / 32;
            int tile_y_end = (GET_Y(tmp_b) + info.height) / 32;
            for (int k = 0; k <= 11; k++) {
                for (int l = 0; l <= 13; l++) {
#pragma HLS PIPELINE off
                    int tile_idx = l * 12 + k;
                    if (tile_x <= k && k <= tile_x_end && tile_y <= l && l <= tile_y_end) {
                        // find a empty slot in this tile
                        if (bucket[tile_idx] < 8) {
                            grid_info.player_bullets[tile_idx * 8 + bucket[tile_idx]] = compose_entity_32(GET_X(tmp_b), GET_Y(tmp_b), 0, GET_TYPE(tmp_b), 1);
                            bucket[tile_idx]++;
                        } // else just ignore this bullet
                    }
                }
            }
        }
    }
render_frame_tile_y:
    for (int i = 0; i < TILE_Y_COUNT; i++) {
        // #pragma HLS PIPELINE off
    render_frame_tile_x:
        for (int j = 0; j < TILE_X_COUNT; j++) {
            // #pragma HLS PIPELINE off
        clear_tile_y:
            for (int k = 0; k < TILE_HEIGHT; k++) {
            clear_tile_x:
                for (int l = 0; l < TILE_WIDTH; l++) {
                    // #pragma HLS PIPELINE off
                    //  used for testing
                    //  tile_fb[k * TILE_WIDTH + l] = (k&0xFF)<<24|(l&0xFF)<<16|(4<<8)|0;
                    tile_fb[k * TILE_WIDTH + l] = 0;
                }
            }
        render_enemy_bullets:
            for (int k = 0; k < TILE_HEIGHT; k++) {
                // #pragma HLS PIPELINE off
            render_enemy_bullets_x:
                for (int l = 0; l < TILE_WIDTH; l++) {
#pragma HLS PIPELINE
                    // #pragma HLS UNROLL factor = 4
                    // #pragma HLS ARRAY_PARTITION variable = tile_fb dim = 2 type = cyclic factor = 4
                render_enemy_bullets_y:
                    ap_uint<TILE_DEPTH> tmp_pixel = tile_fb[k * TILE_WIDTH + l]; // not drawing
                    ap_uint<8> alpha_ch = tmp_pixel(7, 0);
                    for (int m = 0; m < 4; m++) {
#pragma HLS PIPELINE
                        // if you need less drawn just change macro here MAX_ENEMY_BULLETS_IN_TILE
                    render_enemy_enum_bullets:
                        ap_uint<32> tmp_bullet = grid_info.enemy_bullets[(i * TILE_X_COUNT + j) * MAX_ENEMY_BULLETS_IN_TILE + m];
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
                            int sprite_addr = (tmp_sprite.y + offset_y) * BULLET_MAP_WIDTH + (tmp_sprite.x + offset_x);
                            ap_uint<16> tmp_color = ((bullet_sprite[sprite_addr / 4] >> ((sprite_addr & 0x3) * 16))) & 0xFFFF;
                            if ((tmp_color & 0xFFFE) != 0x1204) {
                                if (tmp_color & 0x1) {
                                    alpha_ch |= 0x2; // bit 1 indicate enemy bullet
                                }
                                tmp_pixel = (RGB_DITHER(((ap_uint<64>)tmp_color))) | (alpha_ch);
                                // RGB_DITHER got lowest bit to 1
                                // and lowest [7:1] bit reserved (alpha_ch)
                            }
                        } else {
                            continue;
                        }
                    } // end of render_enemy_enum_bullets
                    tile_fb[k * TILE_WIDTH + l] = tmp_pixel;
                }
            }
        render_player_bullets:
            for (int k = 0; k < TILE_HEIGHT; k++) {
                // #pragma HLS PIPELINE off
            render_player_bullets_x:
                for (int l = 0; l < TILE_WIDTH; l++) {
#pragma HLS PIPELINE
                    // #pragma HLS UNROLL factor = 2
                    // #pragma HLS ARRAY_PARTITION variable = tile_fb dim = 2 type = cyclic factor = 2
                render_player_bullets_y:
                    ap_uint<TILE_DEPTH> tmp_pixel = tile_fb[k * TILE_WIDTH + l]; // not drawing
                    ap_uint<8> alpha_ch = tmp_pixel(7, 0);
                    for (int m = 0; m < 2; m++) {
#pragma HLS PIPELINE
                        // if you need less just change macro here MAX_PLAYER_BULLETS_IN_TILE
                    render_player_enum_bullets:
                        ap_uint<32> tmp_bullet = grid_info.player_bullets[(i * TILE_X_COUNT + j) * MAX_PLAYER_BULLETS_IN_TILE + m];
                        if (GET_VALID(tmp_bullet) == 1) { // if valid
                            sprite_t tmp_sprite = get_player_bullet_info(GET_TYPE(tmp_bullet));
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
                            int sprite_addr = (tmp_sprite.y + offset_y) * BULLET_MAP_WIDTH + (tmp_sprite.x + offset_x);
                            ap_uint<16> tmp_color = ((bullet_sprite[sprite_addr / 4] >> ((sprite_addr & 0x3) * 16))) & 0xFFFF;
                            if ((tmp_color & 0xFFFE) != 0x1204) {
                                if (tmp_color & 0x1) {
                                    alpha_ch |= 0x4; // bit 2 indicate host bullet
                                }
                                tmp_pixel = (RGB_DITHER(((ap_uint<64>)tmp_color))) | (alpha_ch);
                            }
                        } else {
                            continue;
                        }
                    } // end of render_enemy_enum_bullets
                    tile_fb[k * TILE_WIDTH + l] = tmp_pixel;
                }
            }
// Final flush
// 32 pixel, 32 * 4 = 128 Byte = 16 Beat * 8Byte flush the tile to DDR
#ifdef __SYNTHESIS__
        flush_tile_y:
            for (int k = 0; k < TILE_HEIGHT; k++) {
                if (fb1_alt) {
                    vram.write_request((FB1_ALT_BASE + ((FB_START_Y + i * TILE_HEIGHT + k) * FB_WIDTH + (FB_START_X + j * TILE_WIDTH)) * PIX_DEPTH / 8) / 8,
                                       16);
                } else {
                    vram.write_request((FB1_BASE + ((FB_START_Y + i * TILE_HEIGHT + k) * FB_WIDTH + (FB_START_X + j * TILE_WIDTH)) * PIX_DEPTH / 8) / 8, 16);
                }
            flush_tile_x:
                for (int l = 0; l < 16; l++) {
#pragma HLS PIPELINE off
                    // lower 16bit for first 1 pixel, upper 16bit for second pixel
                    vram.write(ap_uint<64>(tile_fb[k * TILE_WIDTH + 2 * l]) | (ap_uint<64>(tile_fb[k * TILE_WIDTH + 2 * l + 1]) << 32));
                }
                vram.write_response();
            }
#else
            ap_uint<64> *dest_vram;
            for (int k = 0; k < TILE_HEIGHT; k++) {
                dest_vram = vram + (0 + ((FB_START_Y + i * TILE_HEIGHT + k) * FB_WIDTH + (FB_START_X + j * TILE_WIDTH)) * PIX_DEPTH / 8) / 8;
                for (int l = 0; l < 16; l++) {
                    *dest_vram = ap_uint<64>(tile_fb[k * TILE_WIDTH + 2 * l]) | (ap_uint<64>(tile_fb[k * TILE_WIDTH + 2 * l + 1]) << 32);
                    dest_vram++;
                }
            }
#endif
        }
    }
}
