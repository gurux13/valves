#pragma once
#include "led.h"
#include "buttons.h"
#include "leds.h"
#include "valves.h"
#include "zigbee.h"
#include "state.h"
#include <functional>
class Logic {
    Leds& leds;
    Buttons& buttons;
    Valves& valves;
    ZigbeeState state;
    std::function<void()> notify_zigbee_callback;
    void all_leds_off();
    void on_operation(ZigbeeState::State new_state);
    void notify_zigbee();
    void init_state();
    void on_valve_state_change(Valves::ValveId valve, Valve::State new_state);
    void update_raw_states();
    void update_leds();
public:
    Logic(Leds& leds, Buttons& buttons, Valves& valves, std::function<void()> notify_zigbee_callback);
    const ZigbeeState& get_state() const;
    void on_zigbee_raw(Valves::ValveId valve, bool open);
    void on_zigbee_normal(ZigbeeState::State new_state);
    void on_zigbee_connecting();
    void on_zigbee_connected();
    void on_zigbee_error();
    void start();
};