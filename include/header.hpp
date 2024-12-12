#include <hls_math.h>
#include <ap_int.h>
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
#define TILE_DEPTH 16

#define AXI_WIDTH 64
#define TILE_SIZE TILE
#define BULLET_MAP_DEPTH 16
#define BULLET_MAP_WIDTH 256
#define BULLET_MAP_HEIGHT 128

#define FB_START_X (33 - 1)
#define FB_START_Y (17 - 1)
// direction存 链表里
typedef __attribute__((packed)) struct {
    // x 9 bit y 9bit rotation 9bit(512 degrees),type 4bit, valid 1bit
    uint32_t x : 9;        // 0-511
    uint32_t y : 9;        // 0-511
    uint32_t rotation : 9; // 0-511 mapped to 0-359
    uint32_t type : 4;     // 0-15 types
    uint32_t valid : 1;
} bullet_t; // in total 4B
#define MAX_ENEMY_BULLETS_IN_TILE 16
#define MAX_PLAYER_BULLETS 256
// max 2048 bullets of enemy 2048*4B = 8KiB
// max 512 bullets of host256*4B= 1KiB, intotal 9KiB, 2 bram
typedef struct {
    bullet_t enemy_bullets[TILE_X_COUNT*TILE_Y_COUNT*MAX_ENEMY_BULLETS_IN_TILE];
    bullet_t player_bullets[MAX_PLAYER_BULLETS];
} game_bullets_t;
#define MAX_ENEMYS 15
typedef __attribute__((packed)) struct {
    uint32_t x : 9;        // 0-511
    uint32_t y : 9;        // 0-511
    uint32_t rotation : 9; // 0-511 mapped to 0-359
    uint32_t type : 4;     // 0-2 types
    uint32_t valid : 1;
} entity_t; // 4B
typedef struct {
    uint32_t x : 9;        // 0-511
    uint32_t y : 9;        // 0-511
    uint32_t rotation : 9; // not used
    uint32_t type : 4;     // 0-2 types
    uint32_t valid : 1;
} dialog_t; // 4B
typedef struct {
    uint32_t x : 9;        // 0-511
    uint32_t y : 9;        // 0-511
    uint32_t rotation : 9; // not used
    uint32_t type : 4;     // 0-2 types
    uint32_t valid : 1;
} big_image_t; // 4B
// max 16 enemy 1 player
typedef struct {
    entity_t enemies[MAX_ENEMYS];
    entity_t player;
    dialog_t dialog_player;
    dialog_t dialog_enemy;
    big_image_t player_image;
    big_image_t enemy_image;
} game_misc_t; // 80 bytes
// dialog player, dialog enemy, player image, enemy image, 绘制时直接覆盖弹幕，反正不用判定

// 可以替换sprite
// 画自机还是基于tile,根据id hardcode ddr地址，然后固定好line stride.
// 遇到透明像素不绘制. 每个tile画之前清空
//#ifdef __SYNTHESIS__
//#define BULLET_MAP_ADDR 0x00800000
//#else
//#define BULLET_MAP_ADDR 0
//#endif
#define BULLET_MAP_ADDR 0x800000
#define FB0_BASE 0x01000000
#define FB1_BASE 0x0112c000
#define FB0_ALT_BASE 0x01258000
#define FB1_ALT_BASE 0x01384000
#define BULLET_INFO_ADDR BULLET_MAP_ADDR + BULLET_MAP_HEIGHT *BULLET_MAP_WIDTH *BULLET_MAP_DEPTH / 8
#define GAME_MISC_ADDR BULLET_INFO_ADDR + (TILE_X_COUNT*TILE_Y_COUNT*MAX_ENEMY_BULLETS_IN_TILE + MAX_PLAYER_BULLETS) * sizeof(bullet_t)
#define READ_BURST_BEATS 256
#define WRITE_BURST_BEATS 256
#define RGB_DITHER(x) ((((x>>(1+2*5))&0x1F)<<(24+3))|(((x>>(1+1*5))&0x1F)<<(16+3))|(((x>>1)&0x1F)<<(8+3))|0x0)
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
