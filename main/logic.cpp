#include "logic.h"
#include "zigbee.h"
#include <esp_log.h>
#include <freertos/mpu_wrappers.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LOG_TAG "Logic"
Logic* the_logic = nullptr;
void logic_on_valve_state_change(ValveId valve, Valve::State new_state) {
    auto stack = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGW(LOG_TAG, "logic_on_valve_state_change stack high water mark: %u", stack);
    if (the_logic != nullptr) {
        ESP_LOGI(LOG_TAG, "Notifying logic of valve %d state change to %s", valve, Valve::state_to_string(new_state).c_str());
        the_logic->on_valve_state_change(valve, new_state);
    }
}
void logic_on_button_press(ButtonId button) {
    if (!the_logic) {
        ESP_LOGE(LOG_TAG, "Logic instance is null in button press handler!");
        return;
    }
    the_logic->button_pressed = true;
    the_logic->last_button_pressed = button;
}
Logic::Logic(Leds& leds, Buttons& buttons, Valves& valves) : leds(leds), buttons(buttons), valves(valves) {
    if (the_logic == nullptr) {
        the_logic = this;
    } else {
        while (true) {
            ESP_LOGE(LOG_TAG, "Multiple Logic instances created!");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

const ZigbeeState& Logic::get_state() const {
    return state;
}

void Logic::notify_zigbee() {
    zb_report_state(state);
}

void Logic::all_leds_off() {
    for (auto led : leds.get_all()) {
        led->off();
    }
}

void Logic::start() {
    all_leds_off();
    leds.get(Leds::Color::RED).blink(100);
    init_state();
    update_leds();
    if (this->buttons.get(ButtonId::CLOSE).is_pressed() || this->buttons.get(ButtonId::BYPASS).is_pressed() || this->buttons.get(ButtonId::NORMAL).is_pressed()) {
        should_factory_reset = true;
        ESP_LOGI(LOG_TAG, "Factory reset button held during startup, will factory reset Zigbee stack");
    }
}

void Logic::step() {
    if (button_pressed) {
        button_pressed = false;
        switch (last_button_pressed) {
            case ButtonId::NORMAL:
                on_operation(ZigbeeState::State::SOFTEN);
                break;
            case ButtonId::BYPASS:
                on_operation(ZigbeeState::State::BYPASS);
                break;
            case ButtonId::CLOSE:
                on_operation(ZigbeeState::State::FULL_CLOSE);
                break;
        }
    }
}

void Logic::update_leds() {
    all_leds_off();
    if (!zb_connected) {
        leds.get(Leds::Color::RED).blink(1000);
        return;
    }
    if (state.transitioning) {
        leds.get(Leds::Color::YELLOW).blink(200);
        return;
    }
    switch (state.state) {
        case ZigbeeState::State::UNDEFINED:
            leds.get(Leds::Color::YELLOW).blink(200);
            break;
        case ZigbeeState::State::SOFTEN:
            leds.get(Leds::Color::GREEN).on();
            break;
        case ZigbeeState::State::BYPASS:
            leds.get(Leds::Color::YELLOW).on();
            break;
        case ZigbeeState::State::FULL_CLOSE:
            leds.get(Leds::Color::RED).on();
            break;
        case ZigbeeState::State::ERROR:
            leds.get(Leds::Color::RED).blink(100);
            break;
        case ZigbeeState::State::RAW_CONTROL:
            leds.get(Leds::Color::YELLOW).blink(1000);
            break;
    }
}

void Logic::on_valve_state_change(ValveId valve, Valve::State new_state) {
    ESP_LOGI(LOG_TAG, "Logic received valve state change");
    bool state_changed = false;
    switch (valve) {
        case ValveId::INLET:
            if (state.rawValues.inlet != new_state) {
                state.rawValues.inlet = new_state;
                state_changed = true;
            }
            break;
        case ValveId::OUTLET:
            if (state.rawValues.outlet != new_state) {
                state.rawValues.outlet = new_state;
                state_changed = true;
            }
            break;
        case ValveId::BYPASS:
            if (state.rawValues.bypass != new_state) {
                state.rawValues.bypass = new_state;
                state_changed = true;
            }
            break;
    }
    if (new_state == Valve::State::ERROR) {
        state.state = ZigbeeState::State::ERROR;
        state.transitioning = false;
        ESP_LOGI(LOG_TAG, "A valve entered ERROR state, setting overall state to ERROR");
        state_changed = true;
    }
    if (state_changed) {
        ESP_LOGI(LOG_TAG, "Valve changed state to %s", Valve::state_to_string(new_state).c_str());
        if (state.transitioning) {
            // Check if we have reached the desired state
            bool in_desired_state = false;
            switch (state.state) {
                case ZigbeeState::State::SOFTEN:
                    in_desired_state = (state.rawValues.inlet == Valve::State::OPEN && state.rawValues.outlet == Valve::State::OPEN && state.rawValues.bypass == Valve::State::CLOSED);
                    break;
                case ZigbeeState::State::BYPASS:
                    in_desired_state = (state.rawValues.inlet == Valve::State::CLOSED && state.rawValues.outlet == Valve::State::CLOSED && state.rawValues.bypass == Valve::State::OPEN);
                    break;
                case ZigbeeState::State::FULL_CLOSE:
                    in_desired_state = (state.rawValues.inlet == Valve::State::CLOSED && state.rawValues.outlet == Valve::State::CLOSED && state.rawValues.bypass == Valve::State::CLOSED);
                    break;
                case ZigbeeState::State::RAW_CONTROL:
                    in_desired_state = true;
                    break;
                default:
                    break;
            }
            
            if (in_desired_state) {
                state.transitioning = false;
                ESP_LOGI(LOG_TAG, "Reached desired state %d", static_cast<int>(state.state));
            }
        }
        ESP_LOGI(LOG_TAG, "Updating LEDs and notifying Zigbee");
        notify_zigbee();
        update_leds();
    }
}

void Logic::init_state() {
    state.state = ZigbeeState::State::UNDEFINED;
    state.transitioning = false;
    auto & inlet = valves.get_valve(ValveId::INLET);
    auto & outlet = valves.get_valve(ValveId::OUTLET);
    auto & bypass = valves.get_valve(ValveId::BYPASS);
    state.rawValues.inlet = inlet.get_state();
    state.rawValues.outlet = outlet.get_state();
    state.rawValues.bypass = bypass.get_state();
    ESP_LOGI(LOG_TAG, "Initial state: inlet=%s, outlet=%s, bypass=%s",
             Valve::state_to_string(inlet.get_state()).c_str(),
             Valve::state_to_string(outlet.get_state()).c_str(),
             Valve::state_to_string(bypass.get_state()).c_str());
    if (inlet.get_state() == Valve::State::OPEN &&
        outlet.get_state() == Valve::State::OPEN &&
        bypass.get_state() == Valve::State::CLOSED) {
            state.state = ZigbeeState::State::SOFTEN;
    }
    else if (inlet.get_state() == Valve::State::CLOSED &&
             outlet.get_state() == Valve::State::CLOSED &&
             bypass.get_state() == Valve::State::OPEN) {
                state.state = ZigbeeState::State::BYPASS;
    }
    else if (inlet.get_state() == Valve::State::CLOSED &&
             outlet.get_state() == Valve::State::CLOSED &&
             bypass.get_state() == Valve::State::CLOSED) {
                state.state = ZigbeeState::State::FULL_CLOSE;
    }
    else if (inlet.get_state() == Valve::State::ERROR ||
             outlet.get_state() == Valve::State::ERROR ||
             bypass.get_state() == Valve::State::ERROR) {
                state.state = ZigbeeState::State::ERROR;
    } else {
        state.state = ZigbeeState::State::RAW_CONTROL;
    }
}

void Logic::on_operation(ZigbeeState::State new_state) {
    if (state.state == new_state) {
        ESP_LOGI(LOG_TAG, "Already in state %d, ignoring new operation", static_cast<int>(new_state));
        notify_zigbee();
        return;
    }
    if (state.state == ZigbeeState::State::ERROR) {
        ESP_LOGW(LOG_TAG, "Currently in ERROR state, cannot start new operation to state %d", static_cast<int>(new_state));
        notify_zigbee();
        return;
    }
    state.state = new_state;
    state.transitioning = true;
    switch (new_state) {
        case ZigbeeState::State::SOFTEN:
            valves.get_valve(ValveId::BYPASS).close();
            valves.get_valve(ValveId::INLET).open();
            valves.get_valve(ValveId::OUTLET).open();
            break;
        case ZigbeeState::State::BYPASS:
            valves.get_valve(ValveId::INLET).close();
            valves.get_valve(ValveId::OUTLET).close();
            valves.get_valve(ValveId::BYPASS).open();
            break;
        case ZigbeeState::State::FULL_CLOSE:
            valves.get_valve(ValveId::INLET).close();
            valves.get_valve(ValveId::OUTLET).close();
            valves.get_valve(ValveId::BYPASS).close();
            break;
        case ZigbeeState::State::ERROR:
            break;
        default:
            ESP_LOGW(LOG_TAG, "Unknown state %d", static_cast<int>(new_state));
            break;
    }
    update_leds();
    ESP_LOGI(LOG_TAG, "Started operation to state %d", static_cast<int>(new_state));
    notify_zigbee();
}

void Logic::on_zigbee_connected() {
    ESP_LOGI(LOG_TAG, "Zigbee connected");
    zb_connected = true;
    update_leds();
}

void Logic::on_zigbee_connecting() {
    ESP_LOGI(LOG_TAG, "Zigbee connecting");
    zb_connected = false;
    update_leds();
}

void Logic::on_zigbee_normal(ZigbeeState::State new_state) {
    on_operation(new_state);
}

void Logic::on_zigbee_raw(ValveId valve, bool open) {
    state.state = ZigbeeState::State::RAW_CONTROL;
    state.transitioning = true;
    Valve& v = valves.get_valve(valve);
    if (open) {
        v.open();
    } else {
        v.close();
    }
}