#include <vector>
#include <string>
#include "Valve.h"
#include <thread>
#pragma once

class Valves {
    Valves(const Valves&) = delete;
    Valves& operator=(const Valves&) = delete;
    
    std::array<Valve, 3> valves;

    void on_state_change(Valve::State new_state, Valve& valve);
    std::thread step_thread;
    Valves();
    void thread_proc();

public:
    enum ValveId {
        INLET = 0,
        OUTLET = 1,
        BYPASS = 2
    };
    
    static Valves& get_instance();
    void init();
    void step() ;
    Valve& get_valve(ValveId id);
    const std::array<Valve, 3>& get_all_valves();
};