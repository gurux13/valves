#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <chrono>
#include <thread>
#include "esp_system.h"
#include "pinout.h"
#include "logic.h"

const char* TAG = "button";

Button::Button(int pin, ButtonId id) : pin(pin), id(id) {}

void Button::init() {
    ESP_ERROR_CHECK(gpio_reset_pin((gpio_num_t)pin));
    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT));

    button_thread = std::thread([this]() {
        int last_state = 1;
        while (true) {
            int current_state = gpio_get_level((gpio_num_t)this->pin);
            pressed = (current_state == 0);
            if (current_state == 0 && last_state == 1) { // Button pressed
                
                ESP_LOGI(TAG, "Button on pin %d pressed", this->pin);
                logic_on_button_press(id);

            }
            last_state = current_state;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    button_thread.detach();
}

bool Button::is_pressed() {
    return pressed;
}
