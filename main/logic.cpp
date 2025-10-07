#include "logic.h"
#include <esp_log.h>

#define LOG_TAG "Logic"

Logic::Logic(Leds& leds, Buttons& buttons, Valves& valves, std::function<void()> notify_zigbee_callback) : leds(leds), buttons(buttons), valves(valves), state(ZigbeeState::State::UNDEFINED), notify_zigbee_callback(notify_zigbee_callback) {
}

const ZigbeeState& Logic::get_state() const {
    return state;
}

void Logic::notify_zigbee() {
    if (notify_zigbee_callback) {
        notify_zigbee_callback();
    }
}

void Logic::all_leds_off() {
    for (auto led : leds.get_all()) {
        led->off();
    }
}

void Logic::start() {
    all_leds_off();
    leds.get(Leds::Color::RED).blink(100);
    buttons.get(Buttons::ButtonId::NORMAL).set_callback([this]() {
        on_operation(ZigbeeState::State::SOFTEN);
        notify_zigbee();
    });
    buttons.get(Buttons::ButtonId::BYPASS).set_callback([this]() {
        on_operation(ZigbeeState::State::BYPASS);
        notify_zigbee();
    });
    buttons.get(Buttons::ButtonId::CLOSE).set_callback([this]() {
        on_operation(ZigbeeState::State::FULL_CLOSE);
        notify_zigbee();
    });
    init_state();
    update_leds();
}

void Logic::update_leds() {
    all_leds_off();
    if (state.transitioning) {
        leds.get(Leds::Color::YELLOW).blink(500);
        return;
    }
    switch (state.state) {
        case ZigbeeState::State::UNDEFINED:
            leds.get(Leds::Color::YELLOW).blink(250);
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
            leds.get(Leds::Color::RED).blink(250);
            break;
        case ZigbeeState::State::RAW_CONTROL:
            leds.get(Leds::Color::YELLOW).blink(1000);
            break;
    }
}

void Logic::on_valve_state_change(Valves::ValveId valve, Valve::State new_state) {
    bool state_changed = false;
    switch (valve) {
        case Valves::ValveId::INLET:
            if (state.rawValues.inlet != new_state) {
                state.rawValues.inlet = new_state;
                state_changed = true;
            }
            break;
        case Valves::ValveId::OUTLET:
            if (state.rawValues.outlet != new_state) {
                state.rawValues.outlet = new_state;
                state_changed = true;
            }
            break;
        case Valves::ValveId::BYPASS:
            if (state.rawValues.bypass != new_state) {
                state.rawValues.bypass = new_state;
                state_changed = true;
            }
            break;
    }
    if (state_changed) {
        ESP_LOGI(LOG_TAG, "Valve %d changed state to %s", static_cast<int>(valve), Valve::state_to_string(new_state).c_str());
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
                case ZigbeeState::State::ERROR:
                    in_desired_state = (state.rawValues.inlet == Valve::State::ERROR || state.rawValues.outlet == Valve::State::ERROR || state.rawValues.bypass == Valve::State::ERROR);
                    break;
                default:
                    break;
            }
            if (in_desired_state) {
                state.transitioning = false;
                ESP_LOGI(LOG_TAG, "Reached desired state %d", static_cast<int>(state.state));
            }
        } else {
            update_raw_states();
        }
        notify_zigbee();
        update_leds();
    }
}

void Logic::init_state() {
    state.state = ZigbeeState::State::UNDEFINED;
    state.transitioning = false;
    auto & inlet = valves.get_valve(Valves::ValveId::INLET);
    auto & outlet = valves.get_valve(Valves::ValveId::OUTLET);
    auto & bypass = valves.get_valve(Valves::ValveId::BYPASS);
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
        state.state = ZigbeeState::State::UNDEFINED;
    }
}

void Logic::on_operation(ZigbeeState::State new_state) {
    if (state.state == new_state) {
        ESP_LOGI(LOG_TAG, "Already in state %d, ignoring new operation", static_cast<int>(new_state));
        return;
    }
    state.state = new_state;
    state.transitioning = true;
    switch (new_state) {
        case ZigbeeState::State::SOFTEN:
            valves.get_valve(Valves::ValveId::BYPASS).close();
            valves.get_valve(Valves::ValveId::INLET).open();
            valves.get_valve(Valves::ValveId::OUTLET).open();
            break;
        case ZigbeeState::State::BYPASS:
            valves.get_valve(Valves::ValveId::INLET).close();
            valves.get_valve(Valves::ValveId::OUTLET).close();
            valves.get_valve(Valves::ValveId::BYPASS).open();
            break;
        case ZigbeeState::State::FULL_CLOSE:
            valves.get_valve(Valves::ValveId::INLET).close();
            valves.get_valve(Valves::ValveId::OUTLET).close();
            valves.get_valve(Valves::ValveId::BYPASS).close();
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