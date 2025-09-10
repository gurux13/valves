#pragma once
#include "driver/gpio.h"
#include <thread>
#include <atomic>
#include <mutex>
class Led {
    public:
    Led(const Led&) = delete;
    Led& operator=(const Led&) = delete;
    Led(gpio_num_t pin);
    void on();
    void off();
    void blink(int period_ms);
private:
    gpio_num_t pin;
    std::thread blink_thread;
    volatile int blink_period_ms{0};
    std::recursive_mutex blink_mutex;
};
