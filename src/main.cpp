#include <Arduino.h>
#include <Wire.h>
#include "config.hpp"
#include "checksum.hpp"
#include "ads1115/ADS1X15.h"

ADS1115 adc(0x48);
int32_t channelZ[PACKET_SIZE], channelE[PACKET_SIZE], channelN[PACKET_SIZE];

uint64_t getTimestamp() {
    return (uint64_t)millis();
}

// 根据 timeStamp % 4 决定 variable_data 字段
uint32_t getVariableData(uint64_t ts) {
    uint8_t which = ts & 0x3;
    switch (which) {
    case 0:
        // DEVICE_ID，最高位 GNSS_EN 这里假设 0
        return (0U << 31) | DEVICE_ID;
    default:
        return 0;
    }
}

void initADC() {
    Wire.begin();
    adc.begin();
    adc.setGain(GAIN_RATE);
    adc.setDataRate(SAMPLE_RATE);
}

void setup() {
    Serial.begin(SERIAL_BAUD);
    initADC();
}

void loop() {
    // —— 1. 读 5 个点 ——
    for (int i = 0; i < PACKET_SIZE; i++) {
        channelZ[i] = (int32_t)adc.readADC(0);
        channelE[i] = (int32_t)adc.readADC(1);
        channelN[i] = (int32_t)adc.readADC(2);
    }

    // —— 2. 按 v2 协议组包 ——
    uint8_t packet[PROTOCOL_V2_PACKET_LENGTH];

    // 2.1 包头 (0–1)
    packet[0] = PROTOCOL_V2_HEADER[0];
    packet[1] = PROTOCOL_V2_HEADER[1];

    // 2.2 时间戳 (2–9, 8 字节, 大端)
    uint64_t ts = getTimestamp();
    for (uint8_t i = 0; i < 8; i++) {
        packet[2 + i] = (uint8_t)(ts >> (56 - 8 * i));
    }

    // 2.3 可变字段 (10–13, 4 字节)
    uint32_t var = getVariableData(ts);
    for (uint8_t i = 0; i < 4; i++) {
        packet[10 + i] = (uint8_t)(var >> (24 - 8 * i));
    }

    // 2.4 三通道数据，每通道 5×4=20 字节
    // Z 通道 (14–33)
    for (uint8_t i = 0; i < PACKET_SIZE; i++) {
        int32_t v = channelZ[i];
        for (uint8_t j = 0; j < 4; j++) {
            packet[14 + i * 4 + j] = (uint8_t)(v >> (24 - 8 * j));
        }
    }
    // E 通道 (34–53)
    for (uint8_t i = 0; i < PACKET_SIZE; i++) {
        int32_t v = channelE[i];
        for (uint8_t j = 0; j < 4; j++) {
            packet[34 + i * 4 + j] = (uint8_t)(v >> (24 - 8 * j));
        }
    }
    // N 通道 (54–73)
    for (uint8_t i = 0; i < PACKET_SIZE; i++) {
        int32_t v = channelN[i];
        for (uint8_t j = 0; j < 4; j++) {
            packet[54 + i * 4 + j] = (uint8_t)(v >> (24 - 8 * j));
        }
    }

    // 2.5 校验和 (74)
    packet[74] = getChecksum(packet, PROTOCOL_V2_PACKET_LENGTH);

    // —— 3. 发送 ——
    Serial.write(packet, PROTOCOL_V2_PACKET_LENGTH);
    Serial.flush();

    // 按协议建议的发送间隔
    delay(5000 / SAMPLE_RATE);
}
