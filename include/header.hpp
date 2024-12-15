#include <ap_int.h>
#include <hls_math.h>
#define FB_WIDTH 640
#define FB_HEIGHT 480
#define RENDER_WIDTH 384
#define RENDER_HEIGHT 448
#define TILE_WIDTH 32
#define TILE_HEIGHT 32
#define TILE_X_COUNT RENDER_WIDTH / TILE_WIDTH
#define TILE_Y_COUNT RENDER_HEIGHT / TILE_HEIGHT
// TILE: 12 x 14
#define PIX_DEPTH 32
#define TILE_DEPTH 32

#define AXI_WIDTH 64
#define TILE_SIZE TILE
#define BULLET_MAP_DEPTH 16
#define BULLET_MAP_WIDTH 128
#define BULLET_MAP_HEIGHT 128

#define FB_START_X (33 - 1)
#define FB_START_Y (17 - 1)
// direction stored in chained list
// typedef __attribute__((packed)) struct {
//     // x 9 bit y 9bit rotation 9bit(512 degrees),type 4bit, valid 1bit
//     uint32_t x : 9;        // 0-511
//     uint32_t y : 9;        // 0-511
//     uint32_t rotation : 9; // 0-511 mapped to 0-359
//     uint32_t type : 4;     // 0-15 types
//     uint32_t valid : 1;
// } bullet_t; // in total 4B
#define GET_X(x) x(8, 0)
#define GET_Y(x) x(17, 9)
#define GET_ROTATION(x) x(26, 18)
#define GET_TYPE(x) x(30, 27)
#define GET_VALID(x) x(31, 31)

#define MAX_ENEMY_BULLETS_IN_TILE 16
#define MAX_PLAYER_BULLETS_IN_TILE 8
// max 2048 bullets of enemy 2048*4B = 8KiB
// max 512 bullets of host256*4B= 1KiB, intotal 9KiB, 2 bram
typedef struct {
    ap_uint<32> enemy_bullets[TILE_X_COUNT * TILE_Y_COUNT * MAX_ENEMY_BULLETS_IN_TILE];   // 2688
    ap_uint<32> player_bullets[TILE_X_COUNT * TILE_Y_COUNT * MAX_PLAYER_BULLETS_IN_TILE]; // 1344 max 8 in single tile
    ap_uint<32> entities[64];
    // first 14 enemy, 15th player, 16th boss, 17-24 item.
} game_info_t;
// (12*14*(16+8)+64)*4Byte = 0x4000 Byte
// 0x4000/8B=2048
// 2048/(256Beat)=8Burst
// dialog player, dialog enemy, player image, enemy image, 绘制时直接覆盖弹幕，反正不用判定

#define BULLET_MAP_ADDR 0x800000
#define FB0_BASE 0x01000000
#define FB1_BASE 0x0112c000
#define FB0_ALT_BASE 0x01258000
#define FB1_ALT_BASE 0x01384000
#define GAME_INFO_ADDR 0x810000
#define READ_BURST_BEATS 256
#define WRITE_BURST_BEATS 256
// take in RGBD5551
#define RGB_DITHER(x) ((((x >> (1 + 2 * 5)) & 0x1F) << (24 + 3)) | (((x >> (1 + 1 * 5)) & 0x1F) << (16 + 3)) | (((x >> 1) & 0x1F) << (8 + 3)) | 0x1)
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} sprite_t;
sprite_t get_enemy_bullet_info(ap_uint<4> type);
sprite_t get_player_bullet_info(ap_uint<4> type);
sprite_t get_enemy_info(ap_uint<4> type);
sprite_t get_player_info();
sprite_t get_boss_info();
sprite_t get_item_info(ap_uint<4> type);
