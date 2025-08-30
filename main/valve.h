#pragma once
#include <string>
#include "pinout.h"
#include <functional>
#include <mutex>
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
    typedef std::function<void(State new_state, Valve& valve)> Callback;
    Valve(const ValvePins& pins, const std::string& name, Callback callback);
    void init();
    State get_state() const;
    void step();
    void open();
    void close();
    void stop();
    std::string get_name() const;
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
    const ValvePins& pins;
    const std::string name;
    volatile State state = State::UNKNOWN;
    Callback callback = nullptr;
    void move(bool open);
    std::recursive_mutex state_mutex;
    uint64_t move_start_time = 0;
};