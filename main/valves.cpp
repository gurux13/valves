#include "valves.h"
#include <thread>
#include <chrono>
#include <esp_pthread.h>
#include "esp_log.h"
Valves::Valves(): valves{Valve(INLET_VALVE, "Inlet", std::bind(&Valves::on_state_change, this, std::placeholders::_1, std::placeholders::_2)),
                   Valve(OUTLET_VALVE, "Outlet", std::bind(&Valves::on_state_change, this, std::placeholders::_1, std::placeholders::_2)),
                   Valve(BYPASS_VALVE, "Bypass", std::bind(&Valves::on_state_change, this, std::placeholders::_1, std::placeholders::_2))} {
    // valves.push_back(new Valve(INLET_VALVE, "Inlet", std::bind(&Valves::on_state_change, this, std::placeholders::_1, std::placeholders::_2)));
    // valves.push_back(new Valve(OUTLET_VALVE, "Outlet", std::bind(&Valves::on_state_change, this, std::placeholders::_1, std::placeholders::_2)));
    // valves.push_back(new Valve(BYPASS_VALVE, "Bypass", std::bind(&Valves::on_state_change, this, std::placeholders::_1, std::placeholders::_2)));
}

void Valves::on_state_change(Valve::State new_state, Valve& valve) {
    // Handle state change (e.g., logging, updating UI)
}

Valves& Valves::get_instance() {
    static Valves instance;
    return instance;
}

void Valves::thread_proc() {
    while (true) {
        step();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Valves::init() {
    for (auto& valve : valves) {
        valve.init();
    }
    this->step_thread = std::thread(&Valves::thread_proc, this);
    this->step_thread.detach();
}

void Valves::step() {
    for (auto& valve : valves) {
        valve.step();
    }
}
Valve& Valves::get_valve(ValveId id) {
    return valves.at(static_cast<size_t>(id));
}

const std::array<Valve, 3>& Valves::get_all_valves() {
    return valves;
}