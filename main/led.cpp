#include "led.h"
#include <thread>
#include <chrono>
#include "driver/gpio.h"
#include "esp_log.h"
static const char* TAG = "LED";

Led::Led(gpio_num_t pin) : pin(pin) {
    ESP_ERROR_CHECK(gpio_reset_pin(pin));
    ESP_ERROR_CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
    off();
    blink_thread = std::thread([this]() {
        while (true) {
            int blink_period_ms;
            {
                std::lock_guard<std::recursive_mutex> lock(this->blink_mutex);
                blink_period_ms = this->blink_period_ms;
            }
            if (blink_period_ms == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            ESP_ERROR_CHECK(gpio_set_level(this->pin, 1));

            std::this_thread::sleep_for(std::chrono::milliseconds(blink_period_ms));
            {
                std::lock_guard<std::recursive_mutex> lock(this->blink_mutex);
                blink_period_ms = this->blink_period_ms;
                if (blink_period_ms == 0) continue;
            }
            ESP_ERROR_CHECK(gpio_set_level(this->pin, 0));
            std::this_thread::sleep_for(std::chrono::milliseconds(blink_period_ms));
        }
    });
    blink_thread.detach();
}
void Led::on() {
    std::lock_guard<std::recursive_mutex> lock(this->blink_mutex);
    blink_period_ms = 0;
    ESP_ERROR_CHECK(gpio_set_level(pin, 1));
}
void Led::off() {
    std::lock_guard<std::recursive_mutex> lock(this->blink_mutex);
    blink_period_ms = 0;
    ESP_ERROR_CHECK(gpio_set_level(pin, 0));
}
void Led::blink(int period_ms) {
    if (period_ms <= 0) {
        off();
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(this->blink_mutex);
    blink_period_ms = period_ms;
}
