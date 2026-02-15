#pragma once
#include <string>
#include "pinout.h"
#include <functional>
#include <mutex>
#include "ids.h"
class Valve {
    Valve(const Valve&) = delete;
    Valve& operator=(const Valve&) = delete;
public:
    enum class State {
        UNKNOWN,
        CLOSED,
        OPENING,
        OPEN,
        CLOSING,
        INTERMEDIATE,
        ERROR
    };
    Valve(const ValvePins& pins, const std::string& name, ValveId id);
    void init();
    State get_state() const;
    void step();
    void open();
    void close();
    void stop();
    std::string get_name() const;
    inline ValveId get_id() const { return id; }
    inline static std::string state_to_string(State state) {
        switch (state) {
            case State::UNKNOWN: return "UNKNOWN";
            case State::CLOSED: return "CLOSED";
            case State::OPENING: return "OPENING";
            case State::OPEN: return "OPEN";
            case State::CLOSING: return "CLOSING";
            case State::INTERMEDIATE: return "INTERMEDIATE";
            case State::ERROR: return "ERROR";
            default: return "INVALID_STATE";
        }
    }
private:
    ValveId id;
    const ValvePins& pins;
    const std::string name;
    volatile State state = State::UNKNOWN;
    void move(bool open);
    std::recursive_mutex state_mutex;
    uint64_t move_start_time = 0;
};
