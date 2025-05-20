#include "checksum.hpp"

uint8_t getChecksum(const uint8_t *data, size_t len) {
    uint8_t cs = 0;
    // 从索引 2 开始，到 len-2 (即 len-1 为 checksum 本身)
    for (size_t i = 2; i < len - 1; i++) {
        cs ^= data[i];
    }
    return cs;
}
