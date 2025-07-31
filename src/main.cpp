#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include "config.hpp"
#include "ads1115/ADS1X15.h"

ADS1115 adc(0x48);
int32_t channelZ[PACKET_SIZE], channelE[PACKET_SIZE], channelN[PACKET_SIZE];

// 设备运行 uptime 毫秒（使用 millis() 组合处理 rollover）
uint64_t mcu_utils_uptime_ms(void) {
    static uint32_t low32 = 0;
    static uint32_t high32 = 0;

    uint32_t new_low32 = millis();
    if (new_low32 < low32) {
        high32++;
    }
    low32 = new_low32;

    return ((uint64_t)high32 << 32) | low32;
}

// 根据 timeStamp % 4 决定 variable_data 字段
uint32_t getVariableData(uint64_t ts) {
    uint8_t which = ts & 0x3;
    switch (which) {
    case 0:
        // DEVICE_ID，最高位 GNSS_EN
        return (0U << 31) | DEVICE_ID;
    default:
        return 0;
    }
}

// 小端写入辅助
static inline void write_le64(uint8_t *dst, uint64_t v) {
    memcpy(dst, &v, sizeof(v));
}

static inline void write_le32(uint8_t *dst, uint32_t v) {
    memcpy(dst, &v, sizeof(v));
}

static inline void write_le32_array(uint8_t *dst, const int32_t *src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        memcpy(dst + i * 4, &src[i], 4);
    }
}

// XOR checksum: 从 index 2 到倒数第二个字节（排除 header 和 checksum 本身）
uint8_t calc_xor(const uint8_t *buf, size_t len) {
    uint8_t x = 0;
    if (len < 3)
        return 0;
    for (size_t i = 2; i < len - 1; i++) {
        x ^= buf[i];
    }
    return x;
}

void initADC(void) {
    Wire.begin();
    adc.begin();
    adc.setGain(GAIN_RATE);
    adc.setDataRate(SAMPLE_RATE);
}

void setup(void) {
    Serial.begin(SERIAL_BAUD);
    initADC();
}

void loop(void) {
    // —— 1. 采样 PACKET_SIZE 个点 ——
    for (int i = 0; i < PACKET_SIZE; i++) {
        channelZ[i] = (int32_t)adc.readADC(0);
        channelE[i] = (int32_t)adc.readADC(1);
        channelN[i] = (int32_t)adc.readADC(2);
    }

    // —— 2. 构造数据包 ——
    uint8_t packet[PACKET_LENGTH];

    // 2.1 包头
    packet[0] = HEADER[0];
    packet[1] = HEADER[1];

    // 2.2 时间戳（小端）
    uint64_t ts = mcu_utils_uptime_ms();
    write_le64(&packet[2], ts);

    // 2.3 可变字段（小端）
    uint32_t var = getVariableData(ts);
    write_le32(&packet[10], var);

    // 2.4 三通道数据（小端）
    const size_t header_len = 2 + 8 + 4; // 14
    const size_t channel_block = PACKET_SIZE * 4;

    write_le32_array(&packet[header_len + 0 * channel_block], channelZ, PACKET_SIZE); // Z
    write_le32_array(&packet[header_len + 1 * channel_block], channelE, PACKET_SIZE); // E
    write_le32_array(&packet[header_len + 2 * channel_block], channelN, PACKET_SIZE); // N

    // 2.5 校验和（最后一个字节）
    packet[PACKET_LENGTH - 1] = calc_xor(packet, PACKET_LENGTH);

    // —— 3. 发送 ——
    Serial.write(packet, PACKET_LENGTH);
    Serial.flush();

    // —— 4. 精确间隔：用 busy-wait（while 空转）而不是 delay ——
    // 原意间隔为 5000ms / SAMPLE_RATE
    const uint32_t interval_us = (uint32_t)((5000000ULL + SAMPLE_RATE / 2) / SAMPLE_RATE); // 四舍五入
    static uint64_t next_wakeup = 0;
    uint64_t now = micros();
    if (next_wakeup == 0) {
        next_wakeup = now + interval_us;
    }
    else {
        next_wakeup += interval_us;
        // 如果 drift 过大（漏掉太多周期），重置基准避免无限追赶
        if ((int64_t)(now - next_wakeup) > (int64_t)interval_us * 5) {
            next_wakeup = now + interval_us;
        }
    }
    // busy-wait until target time
    while ((int64_t)(micros() - next_wakeup) < 0) {
        ;
    }
}
