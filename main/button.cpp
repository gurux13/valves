#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <chrono>
#include <thread>
#include "esp_system.h"
#include "pinout.h"

const char* TAG = "button";

Button::Button(int pin) : pin(pin), callback(nullptr) {}

void Button::set_callback(std::function<void()> callback) {
    this->callback = callback;
}
void Button::init() {
    ESP_ERROR_CHECK(gpio_reset_pin((gpio_num_t)pin));
    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT));

    button_thread = std::thread([this]() {
        int last_state = 1;
        while (true) {
            int current_state = gpio_get_level((gpio_num_t)this->pin);
            if (current_state == 0 && last_state == 1) { // Button pressed
                ESP_LOGI(TAG, "Button on pin %d pressed", this->pin);
                if (this->callback) {
                    this->callback();
                }
            }
            last_state = current_state;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    button_thread.detach();
}

