#pragma once

#include <vector>
#include <string>
#include "valve.h"
#include <thread>
class Valves {
    Valves(const Valves&) = delete;
    Valves& operator=(const Valves&) = delete;
    std::array<Valve, 3> valves;
    std::thread step_thread;
    Valves();
    void thread_proc();
public:
    

    
    static Valves& get_instance();
    void init();
    void step();
    Valve& get_valve(ValveId id);
    const std::array<Valve, 3>& get_all_valves();
};