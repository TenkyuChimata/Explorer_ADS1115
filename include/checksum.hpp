#pragma once
#include <stdint.h>
#include <stddef.h>

// 对 data[2] 到 data[len-2] 的所有字节做 XOR 返回校验和
uint8_t getChecksum(const uint8_t *data, size_t len);
