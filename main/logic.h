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
    bool zb_connected;
    std::function<void()> notify_zigbee_callback;
    void all_leds_off();
    void on_operation(ZigbeeState::State new_state);
    void notify_zigbee();
    void init_state();
    void update_raw_states();
    void update_leds();
public:
    Logic(Leds& leds, Buttons& buttons, Valves& valves);
    const ZigbeeState& get_state() const;
    void on_zigbee_raw(ValveId valve, bool open);
    void on_zigbee_normal(ZigbeeState::State new_state);
    void on_zigbee_connecting();
    void on_zigbee_connected();
    void on_zigbee_error();
    void start();
    void on_valve_state_change(ValveId valve, Valve::State new_state);
    void step();
    ButtonId last_button_pressed;
    volatile bool button_pressed = false;
    bool should_factory_reset = false;
};
void logic_on_valve_state_change(ValveId valve, Valve::State new_state);
void logic_on_button_press(ButtonId button);