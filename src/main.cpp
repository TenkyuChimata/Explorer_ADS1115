#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <Wire.h>

#include "utils/delay.h"
#include "utils/gpio.h"
#include "utils/led.h"
#include "utils/uart.h"
#include "utils/uptime.h"

#include "array.h"
#include "packet.h"
#include "settings.h"
#include "ads1115/ADS1X15.h"

#ifndef FW_BUILD
#define FW_BUILD "unknownbuild"
#endif

#ifndef FW_REV
#define FW_REV "custombuild"
#endif

typedef struct {
    uint8_t sample_time_span;
    uint8_t channel_samples;

    int64_t timestamp;
    uint8_t sample_pos = 0;

    int32_t adc_readout_z_axis[MAINLINE_PACKET_CHANNEL_SAMPLES];
    int32_t adc_readout_e_axis[MAINLINE_PACKET_CHANNEL_SAMPLES];
    int32_t adc_readout_n_axis[MAINLINE_PACKET_CHANNEL_SAMPLES];

    uint8_array_t* uart_packet_buffer;
} explorer_states_t;

ADS1115 adc(0x48);
static explorer_states_t explorer_states;

void setup(void) {
    explorer_states.sample_time_span = 1000 / EXPLORER_SAMPLERATE;
    explorer_states.sample_pos = 0;
    explorer_states.timestamp = 0;

    uint8_t packet_size = get_data_packet_size(MAINLINE_PACKET_CHANNEL_SAMPLES);
    explorer_states.uart_packet_buffer = array_uint8_make(packet_size);

    mcu_utils_gpio_init(false);
    mcu_utils_gpio_mode(MCU_STATE_PIN, MCU_UTILS_GPIO_MODE_OUTPUT);
    mcu_utils_led_blink(MCU_STATE_PIN, 3, false);
    mcu_utils_delay_ms(1000, false);

    mcu_utils_uart_init(EXPLORER_BAUDRATE, false);
    mcu_utils_uart_flush();

    char fw_info_buf[43];
    snprintf(fw_info_buf, sizeof(fw_info_buf), "FW REV: %s\r\nBUILD: %s\r\n", FW_REV, FW_BUILD);
    mcu_utils_uart_write((uint8_t*)fw_info_buf, sizeof(fw_info_buf), true);

    mcu_utils_led_blink(MCU_STATE_PIN, 3, false);

    mcu_utils_delay_ms(1000, false);
    mcu_utils_gpio_high(MCU_STATE_PIN);

    Wire.begin();
    Wire.setClock(400000);
    adc.begin();
    adc.setGain(GAIN_RATE);
    adc.setDataRate(ADC_SAMPLE_RATE);
}

void loop(void) {
    static uint64_t next_sample_time = 0;
    uint64_t now = mcu_utils_uptime_ms();

    if (now < next_sample_time) {
        return;
    }
    next_sample_time = now + explorer_states.sample_time_span;

    if (explorer_states.sample_pos == 0) {
        explorer_states.timestamp = now;
    }

    explorer_states.adc_readout_z_axis[explorer_states.sample_pos] = (int32_t)adc.readADC(0);
    explorer_states.adc_readout_e_axis[explorer_states.sample_pos] = (int32_t)adc.readADC(1);
    explorer_states.adc_readout_n_axis[explorer_states.sample_pos] = (int32_t)adc.readADC(2);
    explorer_states.sample_pos++;

    if (explorer_states.sample_pos == MAINLINE_PACKET_CHANNEL_SAMPLES) {
        send_data_packet(explorer_states.timestamp, explorer_states.adc_readout_z_axis, explorer_states.adc_readout_e_axis, explorer_states.adc_readout_n_axis, MAINLINE_PACKET_CHANNEL_SAMPLES, explorer_states.uart_packet_buffer);
        explorer_states.sample_pos = 0;
    }
}
