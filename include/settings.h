#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <stdbool.h>

#include "utils/gpio.h"

#define GAIN_RATE 0
#define ADC_SAMPLE_RATE 5
#define EXPLORER_SAMPLERATE 25
#define EXPLORER_BAUDRATE 115200

static const mcu_utils_gpio_t MCU_STATE_PIN = {
    .pin = 2,
};

#endif
