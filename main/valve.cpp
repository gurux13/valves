#include "valve.h"
#include "pinout.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_rtc_time.h"
constexpr int VALVE_MOVE_TIMEOUT_MS = 10000; // Maximum time to wait for a valve to open/close

void make_output(gpio_num_t pin)
{
    ESP_ERROR_CHECK(gpio_reset_pin(pin));
    ESP_ERROR_CHECK(gpio_set_level(pin, 0));
    ESP_ERROR_CHECK(gpio_set_direction(pin, GPIO_MODE_OUTPUT));
}

void make_input(gpio_num_t pin)
{
    ESP_ERROR_CHECK(gpio_reset_pin(pin));
    ESP_ERROR_CHECK(gpio_set_direction(pin, GPIO_MODE_INPUT));
}

Valve::Valve(const ValvePins &pins, const std::string &name, Callback callback) : pins(pins), name(name), callback(callback)
{
    this->state = State::UNKNOWN;
}

void Valve::init()
{
    make_output(this->pins.drive_close);
    make_output(this->pins.drive_open);
    make_input(this->pins.sense_closed);
    make_input(this->pins.sense_open);
}

Valve::State Valve::get_state() const
{
    return this->state;
}

void Valve::step()
{
    bool should_callback = false;
    State new_state = State::UNKNOWN;
    {
        std::lock_guard<std::recursive_mutex> lock(this->state_mutex);
        bool is_closed = gpio_get_level(this->pins.sense_closed) == 0;
        bool is_open = gpio_get_level(this->pins.sense_open) == 0;
        new_state = this->state;
        if (is_open || is_closed)
        {
            bool stale_status = (is_open && state == State::CLOSING) || (is_closed && state == State::OPENING);
            if (!stale_status)
            {
                if (is_open)
                {
                    new_state = State::OPEN;
                }
                else if (is_closed)
                {
                    new_state = State::CLOSED;
                }
            }
        }

        if (new_state != this->state)
        {
            auto time_moving = (esp_rtc_get_time_us() - move_start_time) / 1000;
            ESP_LOGI("valve", "Valve %s move took %lld ms", this->name.c_str(), time_moving);

            this->state = new_state;
            this->stop();
            ESP_LOGI("valve", "Valve %s changed state to %s", this->name.c_str(), state_to_string(state).c_str());
            move_start_time = 0;
            should_callback = true;
        }
        if (this->state == State::OPENING || this->state == State::CLOSING)
        {
            int64_t now = esp_rtc_get_time_us();
            if (move_start_time != 0 && (now - move_start_time) > VALVE_MOVE_TIMEOUT_MS * 1000)
            {
                ESP_LOGE("valve", "Valve %s move timed out", this->name.c_str());
                this->state = State::ERROR;
                this->stop();
                should_callback = true;
            }
        }
    }
    if (should_callback && this->callback)
    {
        this->callback(this->state, *this);
    }
}

void Valve::move(bool open)
{
    if (this->state == State::ERROR)
    {
        ESP_LOGW("valve", "Cannot move valve %s because it is in ERROR state", this->name.c_str());
        return;
    }
    move_start_time = esp_rtc_get_time_us();
    {
        std::lock_guard<std::recursive_mutex> lock(this->state_mutex);
        if (open && (this->state == State::OPEN || this->state == State::OPENING))
        {
            ESP_LOGI("valve", "Valve %s already open or opening", this->name.c_str());
            return;
        }
        if (!open && (this->state == State::CLOSED || this->state == State::CLOSING))
        {
            ESP_LOGI("valve", "Valve %s already closed or closing", this->name.c_str());
            return;
        }

        ESP_LOGI("valve", "%s valve %s", open ? "Opening" : "Closing", this->name.c_str());
        gpio_set_level(this->pins.drive_close, open ? 0 : 1);
        gpio_set_level(this->pins.drive_open, open ? 1 : 0);
        this->state = open ? State::OPENING : State::CLOSING;
    }
    if (this->callback)
    {
        this->callback(this->state, *this);
    }
}

void Valve::open()
{
    this->move(true);
}
void Valve::close()
{
    this->move(false);
}
void Valve::stop()
{
    ESP_LOGI("valve", "Stopping valve %s", this->name.c_str());
    gpio_set_level(this->pins.drive_close, 0);
    gpio_set_level(this->pins.drive_open, 0);
    move_start_time = 0;

    bool should_callback = false;
    {
        std::lock_guard<std::recursive_mutex> lock(this->state_mutex);

        if (this->state == State::OPENING || this->state == State::CLOSING)
        {
            this->state = State::INTERMEDIATE;
            should_callback = true;
        }
    }
    if (this->callback && should_callback)
    {
        this->callback(this->state, *this);
    }
}

std::string Valve::get_name() const
{
    return this->name;
}