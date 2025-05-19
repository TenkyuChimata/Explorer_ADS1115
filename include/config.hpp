#pragma once
#include <stdint.h>
#include <stddef.h>

// ADC 采样率 (最大 7)
#define SAMPLE_RATE 4
// ADC 放大倍数 (最大 16)
#define GAIN_RATE 1
// 串口波特率 (最大 921600)
#define SERIAL_BAUD 57600
// 每个包每通道的样本数
#define PACKET_SIZE 5

// —— 以下是 Protocol v2 定义 ——
// 整包长度
#define PROTOCOL_V2_PACKET_LENGTH 75
// 设备 ID (31 位)，最高位作 GNSS_EN
#define DEVICE_ID 19890604

// v2 包头 (2 字节)
static const uint8_t PROTOCOL_V2_HEADER[2] = {0xFA, 0xDE};
