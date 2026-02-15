#pragma once
#include "driver/gpio.h"
#define PIN_LED GPIO_NUM_11
#define PIN_BUTTON GPIO_NUM_6
struct ValvePins {
    const gpio_num_t drive_open;
    const gpio_num_t drive_close;
    const gpio_num_t sense_open;
    const gpio_num_t sense_closed;
};

constexpr ValvePins INLET_VALVE {GPIO_NUM_15, GPIO_NUM_14, GPIO_NUM_18, GPIO_NUM_19};
constexpr ValvePins OUTLET_VALVE {GPIO_NUM_7, GPIO_NUM_6, GPIO_NUM_12, GPIO_NUM_13};
constexpr ValvePins BYPASS_VALVE {GPIO_NUM_3, GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_5};


#define PIN_BTN_NORMAL GPIO_NUM_0
#define PIN_BTN_BYPASS GPIO_NUM_20
#define PIN_BTN_CLOSE GPIO_NUM_1

#define PIN_LED_G GPIO_NUM_23
#define PIN_LED_Y GPIO_NUM_22
#define PIN_LED_R GPIO_NUM_21