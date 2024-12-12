#include <header.hpp>
// hardcode the sprite location
sprite_t get_enemy_bullet_info(ap_uint<4> type) {
    switch (type) {
    case 0:
        return {0, 0, 16, 16};
    case 1:
        return {16, 0, 16, 16};
    case 2:
        return {32, 0, 16, 16};
    case 3:
        return {48, 0, 8, 8};
    case 4:
        return {48, 8, 8, 8};
    case 5:
        return {56, 8, 8, 8};
    case 6:
        return {0, 16, 16, 16};
    case 7:
        return {16, 16, 16, 16};
    case 8:
        return {32, 16, 16, 16};
    case 9:
        return {48, 16, 16, 16};
    case 10:
        return {0, 32, 16, 16};
    case 11:
        return {16, 32, 16, 16};
    case 12:
        return {32, 32, 16, 16};
    case 13:
        return {48, 32, 16, 16};
    case 14:
        return {64, 32, 32, 32};
    case 15:
        return {96, 32, 32, 32};
    default: // case 0
        return {0, 0, 16, 16};
    }
}
sprite_t get_player_bullet_info(ap_uint<4> type) {
    switch (type) {
    case 0:
        return {32, 48, 16, 16};
    case 1:
        return {48, 48, 16, 16};
    case 2:
        return {64, 96, 64, 12};
    case 3:
        return {64, 108, 64, 20};
    default: // case 0
        return {32, 48, 16, 16};
    }
}
sprite_t get_enemy_info(ap_uint<4> type) {
    switch (type) {
    case 0:
        return {64, 64, 32, 32};
    case 1:
        return {96, 64, 32, 32};
    case 2:
        return {0, 64, 64, 64};
    default: // case 0
        return {64, 64, 32, 32};
    }
}

sprite_t get_player_info() { return {128, 0, 32, 48}; }
sprite_t get_boss_info() { return {160, 0, 96, 128}; }
sprite_t get_item_info(ap_uint<4> type) {
    switch (type) {
    case 0:
        return {0, 48, 16, 16};
    case 1:
        return {16, 48, 16, 16};
    case 2:
        return {64, 0, 32, 32};
    case 3:
        return {96, 0, 32, 32};
    case 4:
        return {128, 64, 32, 32};
    case 5:
        return {128, 96, 32, 32};
    default: // case 0
        return {0, 48, 16, 16};
    }
}