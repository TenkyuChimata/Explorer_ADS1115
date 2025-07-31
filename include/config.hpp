#pragma once
#include <stdint.h>
#include <stddef.h>

// ADC 采样率 (最大 7)
#define SAMPLE_RATE 4
// ADC 放大倍数 (最大 16)
#define GAIN_RATE 0
// 串口波特率 (最大 921600)
#define SERIAL_BAUD 115200
// 每个包每通道的样本数
#define PACKET_SIZE 5

// 整包长度
#define PACKET_LENGTH 75
// 设备 ID
#define DEVICE_ID 19890604

// 包头
static const uint8_t HEADER[2] = {0xFA, 0xDE};
